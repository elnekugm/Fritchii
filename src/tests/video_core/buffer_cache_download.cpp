// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstring>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/buffer_cache/memory_tracker_base.h"
#include "video_core/host1x/gpu_device_memory_manager.h"

namespace {
struct FixtureRuntime;
struct FixtureBuffer : VideoCommon::BufferBase {
    std::vector<u8> bytes;
    FixtureBuffer(FixtureRuntime&, VideoCommon::NullBufferParams tag) : BufferBase(tag) {}
    FixtureBuffer(FixtureRuntime&, DAddr addr, u32 size, bool)
        : BufferBase(addr, size), bytes(size) {}
    explicit FixtureBuffer(size_t size)
        : BufferBase(VideoCommon::NullBufferParams{}), bytes(size) {}
    void MarkUsage(u64, u64) {}
};
struct FixtureStage {
    FixtureBuffer buffer;
    size_t offset{};
    std::span<u8> mapped_span;
    explicit FixtureStage(size_t size) : buffer(size), mapped_span(buffer.bytes) {}
};
struct FixtureRuntime {
    bool out_of_bounds{};
    std::vector<std::pair<u64, u64>> copied;
    FixtureStage DownloadStagingBuffer(size_t size, bool = false) {
        return FixtureStage(size);
    }
    FixtureStage UploadStagingBuffer(size_t size) {
        return FixtureStage(size);
    }
    bool CanReportMemoryUsage() {
        return false;
    }
    u64 GetDeviceLocalMemory() {
        return 0;
    }
    void Finish() {}
    u64 CurrentTick() {
        return 0;
    }
    bool IsFree(u64) {
        return true;
    }
    void Wait(u64) {}
    template <class... T>
    bool CanReorderUpload(T&&...) {
        return false;
    }
    void ClearBuffer(FixtureBuffer& b, u64 off, u64 size, u32) {
        std::fill_n(b.bytes.begin() + off, size, 0);
    }
    template <class C>
    void CopyBuffer(FixtureBuffer& dst, FixtureBuffer& src, const C& copies, bool, bool = false) {
        for (const auto& c : copies) {
            copied.emplace_back(src.CpuAddr(), c.src_offset);
            if (c.src_offset > src.bytes.size() || c.size > src.bytes.size() - c.src_offset ||
                c.dst_offset > dst.bytes.size() || c.size > dst.bytes.size() - c.dst_offset) {
                out_of_bounds = true;
                continue;
            }
            std::memcpy(dst.bytes.data() + c.dst_offset, src.bytes.data() + c.src_offset, c.size);
        }
    }
};
struct FixtureParams {
    using Buffer = FixtureBuffer;
    using Runtime = FixtureRuntime;
    using Async_Buffer = FixtureStage;
    using MemoryTracker = VideoCommon::MemoryTrackerBase<Tegra::MaxwellDeviceMemoryManager>;
    static constexpr bool IS_OPENGL = false, HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS = false,
                          HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT = false,
                          NEEDS_BIND_UNIFORM_INDEX = false, NEEDS_BIND_STORAGE_INDEX = false,
                          USE_MEMORY_MAPS = true, SEPARATE_IMAGE_BUFFERS_BINDINGS = false,
                          USE_MEMORY_MAPS_FOR_UPLOADS = true;
};
} // namespace
TEST_CASE("BufferCache: readback spanning separate buffers", "[video_core][buffer_cache]") {
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT {
        Settings::values.use_multi_core.SetValue(old_multi_core);
    };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT {
        system.Kernel().UnregisterKernelObject(&process);
    };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, gap = 0x10000;
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 3 * gap,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    auto asid = manager.RegisterProcess(&cpu);
    manager.Map(dev, guest, 3 * gap, asid);
    FixtureRuntime runtime;
    VideoCommon::BufferCache<FixtureParams> cache(manager, runtime);
    auto* first =
        cache
            .ObtainCPUBuffer(dev, gap, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                             VideoCommon::ObtainBufferOperation::MarkAsWritten)
            .first;
    std::fill(first->bytes.begin(), first->bytes.end(), 0x31);
    auto* second = cache
                       .ObtainCPUBuffer(dev + 2 * gap, gap,
                                        VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                                        VideoCommon::ObtainBufferOperation::MarkAsWritten)
                       .first;
    std::fill(second->bytes.begin(), second->bytes.end(), 0x72);
    runtime.copied.clear();
    REQUIRE(cache.IsRegionGpuModified(dev, gap));
    REQUIRE(cache.IsRegionGpuModified(dev + 2 * gap, gap));
    cache.DownloadMemory(dev + gap, gap);
    REQUIRE(runtime.copied.empty());
    REQUIRE(cache.IsRegionGpuModified(dev, gap));
    REQUIRE(cache.IsRegionGpuModified(dev + 2 * gap, gap));
    cache.DownloadMemory(dev, 3 * gap);
    CHECK_FALSE(runtime.out_of_bounds);
    const std::vector<std::pair<u64, u64>> expected{{dev, 0}, {dev + 2 * gap, 0}};
    CHECK(runtime.copied == expected);
    std::vector<u8> out1(gap), out2(gap);
    manager.ReadBlockUnsafe(dev, out1.data(), out1.size());
    manager.ReadBlockUnsafe(dev + 2 * gap, out2.data(), out2.size());
    CHECK(std::ranges::all_of(out1, [](u8 x) { return x == 0x31; }));
    CHECK(std::ranges::all_of(out2, [](u8 x) { return x == 0x72; }));
    CHECK_FALSE(cache.IsRegionGpuModified(dev, gap));
    CHECK_FALSE(cache.IsRegionGpuModified(dev + 2 * gap, gap));
}
