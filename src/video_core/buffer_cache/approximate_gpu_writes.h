// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>
#include <boost/container/small_vector.hpp>
#include "common/common_types.h"

namespace VideoCommon {

// Native compute storage pointers carry no size, so a written one is marked GPU-modified over a
// guessed extent (the mapped prefix, up to 8 MiB). Shader code often follows such a buffer: in
// Orbitals every instruction-cache invalidate then downloaded the code with a full GPU wait,
// ~50 times a second. `approx_only` holds the GPU-modified bytes that only guessed extents cover.

/// Records a guessed-extent write. Call before adding the range to `gpu_modified`: bytes an
/// exact write left pending inside it stay exact.
template <typename Set>
void NoteApproximateGpuWrite(Set& approx_only, const Set& gpu_modified, u64 addr, u64 size) {
    if (approx_only.Contains(addr, size)) {
        return;
    }
    boost::container::small_vector<std::pair<u64, u64>, 8> exact;
    gpu_modified.ForEachInRange(addr, size, [&](u64 start, u64 end) {
        u64 cursor = start;
        approx_only.ForEachInRange(start, end - start, [&](u64 covered, u64 covered_end) {
            if (covered > cursor) {
                exact.emplace_back(cursor, covered);
            }
            cursor = covered_end;
        });
        if (cursor < end) {
            exact.emplace_back(cursor, end);
        }
    });
    approx_only.Add(addr, size);
    for (const auto& [start, end] : exact) {
        approx_only.Subtract(start, end - start);
    }
}

/// Whether part of the region is GPU-modified by a write of known extent.
template <typename Set>
[[nodiscard]] bool IsRegionExactlyGpuModified(const Set& approx_only, const Set& gpu_modified,
                                              u64 addr, u64 size) {
    bool found = false;
    gpu_modified.ForEachInRange(addr, size, [&](u64 start, u64 end) {
        found = found || !approx_only.Contains(start, end - start);
    });
    return found;
}

} // namespace VideoCommon
