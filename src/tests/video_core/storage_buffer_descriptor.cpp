// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/paged_scalar.h"
#include "video_core/buffer_cache/storage_buffer_descriptor.h"

namespace {
struct DescriptorMemory {
    std::array<u8, 0x4000> backing{};
    std::array<bool, 2> mapped{true, true};
    mutable std::vector<u64> translations;
    mutable std::vector<u64> scalar_reads;

    const u8* Pointer(u64 address) const {
        if (address >= 0x2000 || !mapped[address / 0x1000]) return nullptr;
        return backing.data() + (address < 0x1000 ? 0x2000 : 0) + (address & 0xfff);
    }
    const u8* GetPointer(u64 address) const {
        translations.push_back(address);
        return Pointer(address);
    }
    template <typename T>
    T Read(u64 address) const {
        scalar_reads.push_back(address);
        return Common::ReadPagedScalar<T>(address, [&](u64 current) { return GetPointer(current); });
    }
    void Store(u64 address, const std::array<u64, 2>& data) {
        std::array<u8, sizeof(data)> bytes{};
        std::memcpy(bytes.data(), data.data(), bytes.size());
        for (size_t i = 0; i < bytes.size(); ++i) {
            backing[Pointer(address + i) - backing.data()] = bytes[i];
        }
        translations.clear();
        scalar_reads.clear();
    }
};
} // namespace

TEST_CASE("Storage descriptors share a translation without crossing fragmented host pages",
          "[video_core][storage_descriptor]") {
    DescriptorMemory memory;
    constexpr std::array<u64, 2> expected{0x1122334455667788ULL, 0xabcdef1234567890ULL};
    for (u64 address = 0; address < 0x1000; ++address) {
        memory.Store(address, expected);
        CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, address, false) == expected);
        if (address <= 0xff0) {
            CHECK(memory.translations == std::vector<u64>{address});
            CHECK(memory.scalar_reads.empty());
        } else {
            CHECK(memory.scalar_reads == std::vector<u64>{address, address + 8});
        }
    }
}

TEST_CASE("Storage descriptor reads keep missing pages and bare pointers distinct",
          "[video_core][storage_descriptor]") {
    DescriptorMemory memory;
    constexpr std::array<u64, 2> data{0x1122334455667788ULL, 0xabcdef1234567890ULL};
    memory.Store(0xff8, data);
    memory.mapped[1] = false;
    CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, 0xff8, false) ==
          std::array<u64, 2>{data[0], 0});
    memory.mapped = {false, true};
    memory.scalar_reads.clear();
    CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, 0xff8, false) ==
          std::array<u64, 2>{0, 0});
    CHECK(memory.scalar_reads == std::vector<u64>{0xff8});

    memory.mapped = {true, true};
    for (const u64 address : {u64{0x100}, u64{0xffc}}) {
        memory.Store(address, data);
        CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, address, true) ==
              std::array<u64, 2>{data[0], 0});
        CHECK(memory.scalar_reads == std::vector<u64>{address});
    }
}

TEST_CASE("Storage descriptors read changed backing instead of reusing previous contents",
          "[video_core][storage_descriptor]") {
    DescriptorMemory memory;
    memory.Store(0x100, {1, 2});
    CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, 0x100, false) ==
          std::array<u64, 2>{1, 2});
    memory.Store(0x100, {3, 4});
    CHECK(VideoCommon::ReadStorageBufferDescriptor(memory, 0x100, false) ==
          std::array<u64, 2>{3, 4});
}
