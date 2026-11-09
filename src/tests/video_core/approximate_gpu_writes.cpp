// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include "common/range_sets.h"
#include "common/range_sets.inc"
#include "video_core/buffer_cache/approximate_gpu_writes.h"

namespace {
using Set = Common::RangeSet<u64>;

// Mirrors BufferCache::MarkWrittenBuffer's bookkeeping.
void Mark(Set& approx, Set& modified, u64 addr, u64 size, bool approximate) {
    if (approximate) {
        VideoCommon::NoteApproximateGpuWrite(approx, modified, addr, size);
    } else {
        approx.Subtract(addr, size);
    }
    modified.Add(addr, size);
}

bool Exact(const Set& approx, const Set& modified, u64 addr, u64 size) {
    return VideoCommon::IsRegionExactlyGpuModified(approx, modified, addr, size);
}
} // namespace

TEST_CASE("Guessed native compute extents do not count as exact GPU writes",
          "[video_core][approximate_gpu_writes]") {
    Set approx, modified;
    // Orbitals: a 2 MiB guessed storage extent covers shader code 64 KiB further on.
    Mark(approx, modified, 0x58fc1000, 0x200000, true);
    REQUIRE(modified.Intersects(0x58fd1000, 0xa00));
    REQUIRE_FALSE(Exact(approx, modified, 0x58fd1000, 0xa00));
    REQUIRE_FALSE(Exact(approx, modified, 0x58fc1000, 0x200000));
    // Repeated guessed writes stay approximate.
    Mark(approx, modified, 0x58fc1000, 0x200000, true);
    REQUIRE_FALSE(Exact(approx, modified, 0x58fd1000, 0xa00));
}

TEST_CASE("Exact GPU writes stay exact under later guessed extents",
          "[video_core][approximate_gpu_writes]") {
    Set approx, modified;
    Mark(approx, modified, 0x2000, 0x100, false);
    Mark(approx, modified, 0x1000, 0x4000, true);
    REQUIRE(Exact(approx, modified, 0x2000, 0x100));
    REQUIRE(Exact(approx, modified, 0x20ff, 1));
    REQUIRE(Exact(approx, modified, 0x1000, 0x4000));
    REQUIRE_FALSE(Exact(approx, modified, 0x1000, 0x1000));
    REQUIRE_FALSE(Exact(approx, modified, 0x2100, 0x100));
    REQUIRE_FALSE(Exact(approx, modified, 0x1fff, 1));
}

TEST_CASE("An exact write over a guessed extent makes those bytes exact",
          "[video_core][approximate_gpu_writes]") {
    Set approx, modified;
    Mark(approx, modified, 0x1000, 0x4000, true);
    Mark(approx, modified, 0x3000, 0x10, false);
    REQUIRE(Exact(approx, modified, 0x3000, 0x10));
    REQUIRE(Exact(approx, modified, 0x2ff0, 0x20));
    REQUIRE_FALSE(Exact(approx, modified, 0x1000, 0x2000));
    REQUIRE_FALSE(Exact(approx, modified, 0x3010, 0x100));
}

TEST_CASE("Only GPU-modified bytes can be exact", "[video_core][approximate_gpu_writes]") {
    Set approx, modified;
    REQUIRE_FALSE(Exact(approx, modified, 0x1000, 0x1000));
    Mark(approx, modified, 0x1000, 0x100, false);
    modified.Subtract(0x1000, 0x100); // downloaded
    REQUIRE_FALSE(Exact(approx, modified, 0x1000, 0x100));
    // A guessed extent over bytes no longer pending is approximate as a whole.
    Mark(approx, modified, 0x800, 0x1000, true);
    REQUIRE_FALSE(Exact(approx, modified, 0x800, 0x1000));
}

TEST_CASE("A guessed extent keeps exact bytes around earlier guessed ones",
          "[video_core][approximate_gpu_writes]") {
    Set approx, modified;
    // One pending run: exact bytes, then a guessed piece; a farther guessed piece after a gap.
    Mark(approx, modified, 0x1000, 0x100, false);
    Mark(approx, modified, 0x1100, 0x100, true);
    Mark(approx, modified, 0x3000, 0x1000, true);
    Mark(approx, modified, 0x1000, 0x1000, true);
    REQUIRE(Exact(approx, modified, 0x1000, 0x100));
    REQUIRE_FALSE(Exact(approx, modified, 0x1100, 0x100));
    REQUIRE_FALSE(Exact(approx, modified, 0x1800, 0x10));
    REQUIRE_FALSE(Exact(approx, modified, 0x3000, 0x1000));
}
