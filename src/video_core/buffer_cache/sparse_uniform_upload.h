// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstring>
#include <span>
#include <unordered_map>

#include "common/common_types.h"

namespace VideoCommon {

// Native indirect uniform banks describe an upper bound, including device pages without backing.
// Preserve that whole descriptor range: holes read zero, and mapped pages after holes still count.
// Other buffer reads retain their strict unmapped-memory diagnostics.
template <class Memory>
void ReadSparseUniformMemory(Memory& memory, u64 address, std::span<u8> destination) {
    static_assert(Memory::AS_BITS < 64);
    constexpr u64 address_limit = u64{1} << Memory::AS_BITS;
    constexpr u64 page_size = 0x1000;
    if (destination.empty()) {
        return;
    }
    if (address >= address_limit) {
        std::memset(destination.data(), 0, destination.size());
        return;
    }
    const size_t readable =
        static_cast<size_t>((std::min<u64>)(destination.size(), address_limit - address));
    if (readable == destination.size()) {
        if (const u8* pointer = memory.GetSpan(address, readable)) {
            std::memcpy(destination.data(), pointer, readable);
            return;
        }
    }
    size_t offset = 0;
    while (offset < readable) {
        const size_t count =
            (std::min)(readable - offset, size_t{page_size - (address & (page_size - 1))});
        if (const u8* pointer = memory.template GetPointer<u8>(address)) {
            std::memcpy(destination.data() + offset, pointer, count);
        } else {
            std::memset(destination.data() + offset, 0, count);
        }
        offset += count;
        address += count;
    }
    if (readable < destination.size()) {
        std::memset(destination.data() + readable, 0, destination.size() - readable);
    }
}

// Adapter for the existing uniform stream reuse reader. Comparing freshly read zero-filled
// bytes also prevents reusing an allocation whose source page was unmapped or remapped.
template <class Memory>
struct SparseUniformReadView {
    Memory& memory;
    void ReadBlockUnsafe(u64 address, void* destination, size_t size) {
        ReadSparseUniformMemory(memory, address, std::span{static_cast<u8*>(destination), size});
    }
};

// Each bank is refreshed once per frame, and again if device mappings change in that frame.
// Map() does not mark buffer-cache pages CPU modified, so remembering only the frame can retain
// zeros uploaded for a hole after that hole acquires real backing.
class SparseUniformRefreshTracker {
public:
    bool NeedsRefresh(u64 address, u32 size, u64 frame, u64 mapping_generation) {
        if (frame != current_frame) {
            banks.clear();
            current_frame = frame;
        }
        const auto [it, inserted] = banks.try_emplace(address, Entry{size, mapping_generation});
        if (!inserted && it->second.size == size &&
            it->second.mapping_generation == mapping_generation) {
            return false;
        }
        it->second = {size, mapping_generation};
        return true;
    }

private:
    struct Entry {
        u32 size;
        u64 mapping_generation;
    };
    u64 current_frame{~u64{0}};
    std::unordered_map<u64, Entry> banks;
};

} // namespace VideoCommon
