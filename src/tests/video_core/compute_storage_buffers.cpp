// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>
#include <limits>
#include <tuple>
#include <catch2/catch_test_macros.hpp>

#include "video_core/buffer_cache/buffer_cache_base.h"
#include "video_core/buffer_cache/storage_buffer_size.h"

TEST_CASE("Native compute pointers use their mapped extent, not the next parameter",
          "[video_core][compute_storage]") {
    constexpr u32 mib = 1024 * 1024;
    using VideoCommon::StorageBufferSizeLimit;
    // The following value can be a texture handle, as in Fortune's Weave.
    CHECK(StorageBufferSizeLimit(4 * mib, 0x10a, false, true, true) == 4 * mib);
    CHECK(StorageBufferSizeLimit(16 * mib, 0x10a, false, true, false) == 8 * mib);
    // NVN and graphics descriptors still use their explicit valid size.
    CHECK(StorageBufferSizeLimit(4 * mib, 0x10a, false, false, true) == 0x10a);
    CHECK(StorageBufferSizeLimit(4 * mib, 0, false, false, false) == 4 * mib);
    CHECK(StorageBufferSizeLimit(4 * mib, 0x10000010aULL, false, false, false) == 4 * mib);
    // Indirect read-only pools retain their larger cap; written pools do not.
    CHECK(StorageBufferSizeLimit(80 * mib, 0x10a, true, true, false) == 64 * mib);
    CHECK(StorageBufferSizeLimit(80 * mib, 0x10a, true, true, true) == 8 * mib);
    // Cap before narrowing: a kind range of 4 GiB must not wrap to zero.
    constexpr u64 four_gib = 1ULL << 32;
    CHECK(StorageBufferSizeLimit(four_gib, 0x10a, false, true, true) == 8 * mib);
    CHECK(StorageBufferSizeLimit(four_gib + 0x2000, 0x10a, true, true, false) == 64 * mib);
    CHECK(StorageBufferSizeLimit(0, 0x10a, false, true, false) == 0);
    CHECK(StorageBufferSizeLimit(0x1000, 0x10a, false, true, false) == 0x1000);
}

TEST_CASE("Compute storage bindings cover every shader descriptor and mask bit",
          "[video_core][compute_storage]") {
    using State = VideoCommon::BufferCacheChannelInfo;
    constexpr size_t count = std::tuple_size_v<decltype(State::compute_storage_buffers)>;
    STATIC_REQUIRE(count == Shader::Info::MAX_SSBOS);
    STATIC_REQUIRE(count <= std::numeric_limits<decltype(State::enabled_compute_storage_buffers)>::digits);
    STATIC_REQUIRE(count <= std::numeric_limits<decltype(State::written_compute_storage_buffers)>::digits);
    std::array<VideoCommon::Binding, count> bindings{};
    u32 enabled{};
    u32 written{};
    for (u32 index = 0; index < count; ++index) {
        bindings[index].size = index + 1;
        enabled |= 1U << index;
        written |= (index % 2) << index;
    }
    size_t visited{};
    while (enabled != 0) {
        const u32 index = std::countr_zero(enabled);
        REQUIRE(bindings[index].size == index + 1);
        REQUIRE(((written >> index) & 1) == index % 2);
        enabled &= enabled - 1;
        ++visited;
    }
    REQUIRE(visited == count);
}
