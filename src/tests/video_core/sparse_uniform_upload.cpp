// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/buffer_cache/sparse_uniform_upload.h"
#include "video_core/buffer_cache/uniform_stream_reuse.h"

namespace {
struct SparseMemory {
    static constexpr size_t AS_BITS = 16;
    static constexpr size_t PageSize = 0x1000;
    std::array<u8, 1 << AS_BITS> bytes{};
    std::array<int, 16> pages{};
    u64 generation{1};
    size_t pointer_reads{};
    size_t direct_reads{};

    SparseMemory() {
        pages.fill(-1);
        for (size_t index = 0; index < bytes.size(); ++index) {
            bytes[index] = static_cast<u8>((index / PageSize) * 13 + index % 251 + 1);
        }
    }
    void Map(size_t page, int backing) {
        pages.at(page) = backing;
        ++generation;
    }
    template <class T>
    T* GetPointer(u64 address) {
        REQUIRE(address < bytes.size());
        ++pointer_reads;
        const int backing = pages[address / PageSize];
        return backing < 0
                   ? nullptr
                   : reinterpret_cast<T*>(bytes.data() + backing * PageSize + address % PageSize);
    }
    const u8* GetSpan(u64 address, size_t size) {
        REQUIRE(address < bytes.size());
        REQUIRE(size <= bytes.size() - address);
        const int backing = pages[address / PageSize];
        if (backing < 0)
            return nullptr;
        const u8* start = bytes.data() + backing * PageSize + address % PageSize;
        for (size_t offset = 0; offset < size;) {
            const size_t current = address + offset;
            const int page = pages[current / PageSize];
            if (page < 0 || bytes.data() + page * PageSize + current % PageSize != start + offset) {
                return nullptr;
            }
            offset += (std::min)(size - offset, PageSize - current % PageSize);
        }
        ++direct_reads;
        return start;
    }
};

void CheckBytes(const SparseMemory& memory, u64 address, std::span<const u8> actual) {
    for (size_t index = 0; index < actual.size(); ++index) {
        u8 expected = 0;
        if (address < memory.bytes.size() && index < memory.bytes.size() - address) {
            const size_t current = address + index;
            const int backing = memory.pages[current / SparseMemory::PageSize];
            if (backing >= 0) {
                expected =
                    memory
                        .bytes[backing * SparseMemory::PageSize + current % SparseMemory::PageSize];
            }
        }
        REQUIRE(actual[index] == expected);
    }
}

struct StreamRuntime {
    struct StreamUniformRef {
        size_t allocation;
    };
    std::vector<std::vector<u8>> allocations;
    StreamUniformRef bound{};
    u64 CurrentTick() const {
        return 1;
    }
    StreamUniformRef LastStreamUniformRef() const {
        return bound;
    }
    std::span<u8> BindMappedUniformBuffer(size_t, u32, u32 size) {
        allocations.emplace_back(size);
        bound = {allocations.size() - 1};
        return allocations.back();
    }
    void RebindStreamUniform(StreamUniformRef ref, u32 size) {
        REQUIRE(allocations.at(ref.allocation).size() == size);
        bound = ref;
    }
};
} // namespace

TEST_CASE("Sparse native uniforms preserve holes, suffixes and unaligned page edges",
          "[video_core][sparse_uniform]") {
    SparseMemory memory;
    memory.Map(0, 8);
    memory.Map(2, 3);  // An interior hole must not discard this mapped suffix.
    memory.Map(3, 11); // Host backing is deliberately not contiguous.
    std::vector<u8> destination(0x3009, 0xa5);
    VideoCommon::ReadSparseUniformMemory(memory, 0xffb, destination);
    CheckBytes(memory, 0xffb, destination);
    CHECK(memory.pointer_reads == 5);
    CHECK(memory.direct_reads == 0);
    CHECK(std::all_of(destination.begin() + 5, destination.begin() + 0x1005,
                      [](u8 value) { return value == 0; }));
}

TEST_CASE("Sparse native uniform reads keep a contiguous fast path and bound address arithmetic",
          "[video_core][sparse_uniform]") {
    SparseMemory memory;
    memory.Map(1, 4);
    memory.Map(2, 5);
    std::vector<u8> destination(0x1001, 0xa5);
    VideoCommon::ReadSparseUniformMemory(memory, 0x1fff, destination);
    CheckBytes(memory, 0x1fff, destination);
    CHECK(memory.direct_reads == 1);
    CHECK(memory.pointer_reads == 0);
    memory.Map(15, 1);
    destination.assign(32, 0xa5);
    VideoCommon::ReadSparseUniformMemory(memory, 0xfff8, destination);
    CheckBytes(memory, 0xfff8, destination);
    const size_t reads = memory.pointer_reads;
    VideoCommon::ReadSparseUniformMemory(memory, (std::numeric_limits<u64>::max)() - 3,
                                         destination);
    CHECK(memory.pointer_reads == reads);
    CHECK(std::all_of(destination.begin(), destination.end(), [](u8 value) { return value == 0; }));
    VideoCommon::ReadSparseUniformMemory(memory, (std::numeric_limits<u64>::max)(),
                                         std::span<u8>{});
}

TEST_CASE("Retained native uniform uploads refresh across holes, remaps and frames",
          "[video_core][sparse_uniform]") {
    SparseMemory memory;
    memory.Map(0, 0);
    memory.Map(1, 1);
    memory.Map(2, 2);
    VideoCommon::SparseUniformRefreshTracker refresh;
    // Model the retained host descriptor allocation: skipping an upload leaves its old bytes.
    std::vector<u8> retained(0x3000, 0xa5);
    size_t uploads = 0;
    const auto bind = [&](u64 frame) {
        if (refresh.NeedsRefresh(0, static_cast<u32>(retained.size()), frame, memory.generation)) {
            VideoCommon::ReadSparseUniformMemory(memory, 0, retained);
            ++uploads;
        }
        CheckBytes(memory, 0, retained);
    };
    bind(1);
    bind(1);
    REQUIRE(uploads == 1);
    memory.Map(1, -1);
    bind(1); // Same frame: previous nonzero host bytes must become zero.
    REQUIRE(uploads == 2);
    memory.Map(1, 7);
    bind(1); // Same frame, no CPU write: zero bytes must become the new backing's bytes.
    REQUIRE(uploads == 3);
    memory.bytes[2] ^= 0xff;
    bind(2); // The existing per-frame refresh still covers untracked CPU changes.
    CHECK(uploads == 4);
    CHECK(retained.size() == 0x3000);
    CHECK(refresh.NeedsRefresh(0, 0x2000, 2, memory.generation));
}

TEST_CASE("Small sparse uniform stream reuse observes remapped backing in the same command",
          "[video_core][sparse_uniform]") {
    SparseMemory memory;
    memory.Map(0, 0);
    memory.Map(1, 1);
    VideoCommon::SparseUniformReadView view{memory};
    StreamRuntime runtime;
    VideoCommon::ResetUniformStreamReuse<StreamRuntime>();
    const auto bind = [&] {
        REQUIRE(VideoCommon::BindSmallUniformReusing(runtime, view, 0, 0, 0, 0xff0, 64));
        CheckBytes(memory, 0xff0, runtime.allocations.at(runtime.bound.allocation));
    };
    bind();
    bind();
    REQUIRE(runtime.allocations.size() == 1);
    memory.Map(1, -1);
    bind();
    REQUIRE(runtime.allocations.size() == 2);
    memory.Map(1, 9);
    bind();
    REQUIRE(runtime.allocations.size() == 3);
    bind();
    CHECK(runtime.allocations.size() == 3);
}
