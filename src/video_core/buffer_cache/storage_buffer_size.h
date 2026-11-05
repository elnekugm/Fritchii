// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include "common/common_types.h"

namespace VideoCommon {

// Native compute pointers have no following size word. A pointer-table entry is also bare;
// read-only tables may address larger pools, while written bindings keep the smaller limit.
[[nodiscard]] constexpr u32 StorageBufferSizeLimit(u64 kind_extent, u64 next_word,
                                                   bool table_pointer, bool native_compute,
                                                   bool is_written) {
    constexpr u32 mib = 1024 * 1024;
    if (table_pointer) {
        return static_cast<u32>((std::min)(kind_extent, u64{(is_written ? 8U : 64U) * mib}));
    }
    if (native_compute) {
        return static_cast<u32>((std::min)(kind_extent, u64{8U * mib}));
    }
    const u32 packed_size = static_cast<u32>(next_word);
    if ((next_word >> 32) == 0 && packed_size != 0 && packed_size <= kind_extent) {
        return packed_size;
    }
    return static_cast<u32>((std::min)(kind_extent, u64{8U * mib}));
}

} // namespace VideoCommon
