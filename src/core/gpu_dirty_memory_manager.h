// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "core/device_memory_manager.h"

namespace Core {

class GPUDirtyMemoryManager {
public:
    GPUDirtyMemoryManager() : current{default_transform} {
        back_buffer.reserve(256);
        front_buffer.reserve(256);
    }

    ~GPUDirtyMemoryManager() = default;

    void Collect(PAddr address, size_t size) {
        TransformAddress t = BuildTransform(address, size);
        TransformAddress tmp, original;
        do {
            tmp = current.load(std::memory_order_acquire);
            original = tmp;
            if (tmp.address != t.address) {
                if (IsValid(tmp.address)) {
                    std::scoped_lock lk(guard);
                    back_buffer.emplace_back(tmp);
                    has_back.store(true, std::memory_order_release);
                    current.exchange(t, std::memory_order_relaxed);
                    return;
                }
                tmp.address = t.address;
                tmp.mask = 0;
            }
            if ((tmp.mask | t.mask) == tmp.mask) {
                return;
            }
            tmp.mask |= t.mask;
        } while (!current.compare_exchange_weak(original, tmp, std::memory_order_acq_rel,
                                                std::memory_order_relaxed));
    }

    /// Calls `callback` for each write collected and not yet applied, without consuming them: those
    /// still collecting and those a Gather is handing out.
    void ForEachPending(const std::function<void(PAddr, size_t)>& callback) {
        std::vector<TransformAddress> pending;
        {
            std::scoped_lock gather_lk(gather_guard);
            std::scoped_lock lk(guard);
            pending = back_buffer;
            pending.insert(pending.end(), front_buffer.begin(), front_buffer.end());
            const TransformAddress t = current.load(std::memory_order_acquire);
            if (IsValid(t.address)) {
                pending.push_back(t);
            }
        }
        for (const auto& transform : pending) {
            ForEachRun(transform, callback);
        }
    }

    /// Moves on with each Gather that took writes. A page the core reported to the GPU before
    /// must be reported again on its next write: the write held back for it has been applied.
    /// Read after Collect, it tells whether the write went into the list after that Gather.
    [[nodiscard]] u64 Generation() const {
        return generation.load(std::memory_order_acquire);
    }

    /// The page a core last reported to the GPU, and in which generation.
    struct ReportedPage {
        PAddr page{~PAddr{}};
        u64 generation{};
    };

    /// Collects a CPU write to GPU-cached memory at `page`. `report` (the GPU's OnCPUWrite, which
    /// holds the page back) runs for the first write to a page in each generation; when it returns
    /// false the write is not collected.
    template <typename Report>
    void CollectWrite(PAddr address, size_t size, PAddr page, ReportedPage& last, Report&& report) {
        const u64 seen = Generation();
        if (last.page != page || last.generation != seen) {
            if (!report()) {
                return;
            }
            last = {page, seen};
        }
        Collect(address, size);
        // A Gather took the list since the generation was read: the write may have gone with it
        // (applied before the caller stores it) or into the next list (while the round releases
        // the page). Report and collect it again for the next round.
        // shortcut: a Gather after this check, before the caller stores the bytes, goes unseen;
        // closing that needs collecting after the store in every write path.
        if (const u64 now = Generation(); now != seen) {
            report();
            Collect(address, size);
            last.generation = now;
        }
    }

    void Gather(std::function<void(PAddr, size_t)>& callback) {
        // FlushRegion also gathers off the GPU thread (host1x video decode reads); serialize
        // gatherers so one cannot swap or clear front_buffer while another iterates it.
        std::scoped_lock gather_lk(gather_guard);
        {
            std::scoped_lock lk(guard);
            // Before the exchange: a Collect that lands in the next list sees the new generation.
            generation.fetch_add(1, std::memory_order_relaxed);
            TransformAddress t = current.exchange(default_transform, std::memory_order_acq_rel);
            front_buffer.swap(back_buffer);
            has_back.store(false, std::memory_order_relaxed);
            if (IsValid(t.address)) {
                front_buffer.emplace_back(t);
            }
        }
        for (auto& transform : front_buffer) {
            ForEachRun(transform, callback);
        }
        front_buffer.clear();
    }

    /// Returns true when collected writes not yet gathered overlap [address, address + size).
    bool Overlaps(PAddr address, size_t size) {
        const PAddr end = address + size;
        const auto overlaps = [&](TransformAddress t) {
            if (!IsValid(t.address)) {
                return false;
            }
            const PAddr begin = static_cast<PAddr>(t.address) << page_bits;
            if (address >= begin + page_size || end <= begin) {
                return false;
            }
            const size_t minor_bit = address > begin ? (address - begin) >> align_bits : 0;
            const size_t top_bit = ((std::min)(end - begin, PAddr{page_size}) + align_mask) >> align_bits;
            return (t.mask & CreateMask<u32>(top_bit, minor_bit)) != 0;
        };
        std::scoped_lock lk(guard);
        if (overlaps(current.load(std::memory_order_acquire))) {
            return true;
        }
        for (const TransformAddress& t : back_buffer) {
            if (overlaps(t)) {
                return true;
            }
        }
        return false;
    }

private:
    struct alignas(8) TransformAddress {
        u32 address;
        u32 mask;
    };

    template <typename Func>
    static void ForEachRun(const TransformAddress& transform, Func&& callback) {
        size_t offset = 0;
        u64 mask = transform.mask;
        while (mask != 0) {
            const size_t empty_bits = std::countr_zero(mask);
            offset += empty_bits << align_bits;
            mask = mask >> empty_bits;
            const size_t continuous_bits = std::countr_one(mask);
            callback((static_cast<PAddr>(transform.address) << page_bits) + offset,
                     continuous_bits << align_bits);
            mask = continuous_bits < align_size ? (mask >> continuous_bits) : 0;
            offset += continuous_bits << align_bits;
        }
    }

    constexpr static size_t page_bits = DEVICE_PAGEBITS - 1;
    constexpr static size_t page_size = 1ULL << page_bits;
    constexpr static size_t page_mask = page_size - 1;

    constexpr static size_t align_bits = 6U;
    constexpr static size_t align_size = 1U << align_bits;
    constexpr static size_t align_mask = align_size - 1;
    constexpr static TransformAddress default_transform = {.address = ~0U, .mask = 0U};

    bool IsValid(PAddr address) {
        return address < (1ULL << 39);
    }

    template <typename T>
    T CreateMask(size_t top_bit, size_t minor_bit) {
        T mask = ~T(0);
        mask <<= (sizeof(T) * 8 - top_bit);
        mask >>= (sizeof(T) * 8 - top_bit);
        mask >>= minor_bit;
        mask <<= minor_bit;
        return mask;
    }

    TransformAddress BuildTransform(PAddr address, size_t size) {
        const size_t minor_address = address & page_mask;
        const size_t minor_bit = minor_address >> align_bits;
        const size_t top_bit = (minor_address + size + align_mask) >> align_bits;
        TransformAddress result{};
        result.address = static_cast<u32>(address >> page_bits);
        result.mask = CreateMask<u32>(top_bit, minor_bit);
        return result;
    }

    std::atomic<TransformAddress> current{};
    std::atomic<bool> has_back{}; ///< back_buffer holds entries (set and cleared under guard)
    std::atomic<u64> generation{};
    std::mutex guard;
    std::mutex gather_guard;
    std::vector<TransformAddress> back_buffer;
    std::vector<TransformAddress> front_buffer;
};

/// One bit per device page of the 36-bit device address space (2 MiB), set by writers and taken
/// by the native uniform bank check (BufferCache, EDEN_NX2_EXP9).
class DevicePageFlags {
public:
    static constexpr u64 AddressBits = 36;

    void Mark(u64 address, u64 size) noexcept {
        constexpr u64 limit = u64{1} << AddressBits;
        if (size == 0 || address >= limit) {
            return;
        }
        u64 page = address >> DEVICE_PAGEBITS;
        const u64 last = ((std::min)(address + size, limit) - 1) >> DEVICE_PAGEBITS;
        while (page <= last) {
            const u64 first_bit = page % 64;
            const u64 bits = (std::min<u64>)(64 - first_bit, last - page + 1);
            const u64 mask = (bits == 64 ? ~u64{0} : (u64{1} << bits) - 1) << first_bit;
            std::atomic<u64>& word = words[page / 64];
            if ((word.load(std::memory_order_relaxed) & mask) != mask) {
                word.fetch_or(mask, std::memory_order_seq_cst);
            }
            page += bits;
        }
    }

    /// Clears the bit of the page holding `address`, returning whether it was set.
    bool Take(u64 address) noexcept {
        if (address >= (u64{1} << AddressBits)) {
            return false;
        }
        const u64 page = address >> DEVICE_PAGEBITS;
        const u64 bit = u64{1} << (page % 64);
        std::atomic<u64>& word = words[page / 64];
        if ((word.load(std::memory_order_relaxed) & bit) == 0) {
            return false;
        }
        return (word.fetch_and(~bit, std::memory_order_seq_cst) & bit) != 0;
    }

private:
    std::array<std::atomic<u64>, (u64{1} << (AddressBits - DEVICE_PAGEBITS)) / 64> words{};
};

/// Pages guest CPU writes went to through the dirty memory managers: the buffer cache sees them
/// only at the next InvalidateGPUCache.
inline DevicePageFlags g_collected_write_pages;
/// Pages whose guest memory the buffer cache saw change (WriteMemory, uploads in place,
/// downloads).
inline DevicePageFlags g_buffer_write_pages;

} // namespace Core
