// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <barrier>
#include <chrono>
#include <cstring>
#include <future>
#include <memory>
#include <thread>
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
#include "video_core/memory_manager.h"
#include "video_core/nx2.h"

namespace {
struct FixtureRuntime;
struct FixtureBuffer : VideoCommon::BufferBase {
    std::vector<u8> bytes;
    FixtureBuffer(FixtureRuntime&, VideoCommon::NullBufferParams tag) : BufferBase(tag) {}
    FixtureBuffer(FixtureRuntime&, DAddr addr, u32 size, bool)
        : BufferBase(addr, size), bytes(size) {}
    explicit FixtureBuffer(size_t size)
        : BufferBase(VideoCommon::NullBufferParams{}), bytes(size) {}
    // (offset, size) of every MarkUsage call: what the cache told the runtime its GPU work uses.
    std::vector<std::pair<u64, u64>> marked;
    void MarkUsage(u64 offset, u64 size) {
        marked.emplace_back(offset, size);
    }
};
struct FixtureStage {
    // Shared: the cache keeps a copy of a fence download's stage until its fence pops.
    std::shared_ptr<FixtureBuffer> buffer;
    size_t offset{};
    std::span<u8> mapped_span;
    explicit FixtureStage(size_t size)
        : buffer(std::make_shared<FixtureBuffer>(size)), mapped_span(buffer->bytes) {}
    explicit FixtureStage(std::shared_ptr<FixtureBuffer> buffer_)
        : buffer(std::move(buffer_)), mapped_span(buffer->bytes) {}
};
struct FixtureRuntime {
    bool out_of_bounds{};
    std::vector<std::pair<u64, u64>> copied;
    struct DeferredStage {
        std::shared_ptr<FixtureBuffer> buffer;
        bool active{};
        u64 last_use_tick{};
    };
    u64 current_tick{};
    u64 gpu_tick{};
    std::vector<DeferredStage> deferred_stages;
    std::vector<size_t> allocation_ids;
    std::vector<size_t> freed_ids;
    FixtureStage DownloadStagingBuffer(size_t size, bool deferred = false) {
        if (!deferred) {
            return FixtureStage(size);
        }
        for (size_t i = 0; i < deferred_stages.size(); ++i) {
            auto& stage = deferred_stages[i];
            if (!stage.active && IsFree(stage.last_use_tick) && stage.buffer->bytes.size() >= size) {
                stage.active = true;
                allocation_ids.push_back(i);
                return FixtureStage(stage.buffer);
            }
        }
        FixtureStage stage(size);
        allocation_ids.push_back(deferred_stages.size());
        deferred_stages.push_back({stage.buffer, true});
        return stage;
    }
    void FreeDeferredStagingBuffer(FixtureStage& ref) {
        ReleaseDeferredStage(ref, current_tick);
    }
    void FreeCompletedDeferredStagingBuffer(FixtureStage& ref) {
        ReleaseDeferredStage(ref, 0);
    }
    void ReleaseDeferredStage(FixtureStage& ref, u64 last_use_tick) {
        const auto it = std::ranges::find_if(deferred_stages, [&](const auto& stage) {
            return stage.buffer == ref.buffer;
        });
        REQUIRE(it != deferred_stages.end());
        REQUIRE(it->active);
        it->active = false;
        it->last_use_tick = last_use_tick;
        freed_ids.push_back(static_cast<size_t>(it - deferred_stages.begin()));
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
        return current_tick;
    }
    bool IsFree(u64 tick) {
        return tick <= gpu_tick;
    }
    void Wait(u64) {}
    template <class... T>
    bool CanReorderUpload(T&&...) {
        return false;
    }
    void ClearBuffer(FixtureBuffer& b, u64 off, u64 size, u32) {
        std::fill_n(b.bytes.begin() + off, size, 0);
    }
    void PreCopyBarrier() {}
    void PostCopyBarrier() {}
    template <class C>
    void CopyBuffer(std::shared_ptr<FixtureBuffer>& dst, FixtureBuffer& src, const C& copies,
                    bool sync, bool reorder = false) {
        CopyBuffer(*dst, src, copies, sync, reorder);
    }
    template <class C>
    void CopyBuffer(FixtureBuffer& dst, std::shared_ptr<FixtureBuffer>& src, const C& copies,
                    bool sync, bool reorder = false) {
        CopyBuffer(dst, *src, copies, sync, reorder);
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
// Bind just the real address-space policy needed by these memory-only cache tests. Engines
// and channel execution are not involved in ObtainCPUBuffer/GetFlushArea/CommitAsyncFlushes.
struct NativeFixtureCache : VideoCommon::BufferCache<FixtureParams> {
    using VideoCommon::BufferCache<FixtureParams>::BufferCache;
    void SetAddressSpace(Tegra::MemoryManager& memory) {
        gpu_memory = &memory;
    }
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

TEST_CASE("BufferCache: downloads keep held-back CPU writes", "[video_core][buffer_cache]") {
    // Three buffers the GPU wrote (host bytes 0x31), three pages each, one per case below. In
    // each, the guest CPU writes code into the middle page and the core holds the write back
    // (OnCPUWrite) until the GPU thread applies it (a round of CachedWriteMemory).
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
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, size = 0x3000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, 3 * size, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 3 * size,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, 3 * size, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);
    const auto gpu_write = [&](u64 base, u8 value) {
        auto* buffer = cache
                           ->ObtainCPUBuffer(base, static_cast<u32>(size),
                                             VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                                             VideoCommon::ObtainBufferOperation::MarkAsWritten)
                           .first;
        std::fill(buffer->bytes.begin(), buffer->bytes.end(), value);
    };
    const auto cpu_write = [&](u64 base) {
        const std::vector<u8> code(0x100, 0xab);
        manager.WriteBlockUnsafe(base + 0x1080, code.data(), code.size());
        REQUIRE(cache->OnCPUWrite(base + 0x1080, code.size()));
    };
    const auto guest_byte = [&](u64 addr) {
        u8 value{};
        manager.ReadBlockUnsafe(addr, &value, 1);
        return value;
    };
    cache->NoteCpuRead(dev, 3 * size); // GPU writes there are copied back at fences

    // A readback of the whole buffer leaves the CPU's newer bytes alone.
    const u64 sync = dev;
    gpu_write(sync, 0x31);
    cpu_write(sync);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync) == 0x31);
    CHECK(guest_byte(sync + 0x1080) == 0xab);
    CHECK(guest_byte(sync + 0x2000) == 0x31);
    // The round that applies the write releases the page: what the GPU writes there later is
    // read back.
    const auto apply_round = [&](auto&& during) {
        cache->BeginCachedWrites();
        during();
        cache->CachedWriteMemory(sync + 0x1080, 0x100);
        cache->EndCachedWrites();
    };
    apply_round([] {});
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync + 0x1080) == 0x5c);
    // Unless the core held the page again during the round: that write waits for the next one.
    gpu_write(sync, 0x31);
    apply_round([&] { cpu_write(sync); });
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync) == 0x5c);
    CHECK(guest_byte(sync + 0x1080) == 0xab);
    apply_round([] {});
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync + 0x1080) == 0x5c);
    cache->CommitAsyncFlushes();
    cache->PopAsyncFlushes();

    // A fence download taken before the CPU write does not write over it when it pops.
    const u64 before = dev + size;
    gpu_write(before, 0x31);
    REQUIRE(cache->HasUncommittedFlushes());
    cache->CommitAsyncFlushes();
    cpu_write(before);
    cache->PopAsyncFlushes();
    CHECK(guest_byte(before) == 0x31);
    CHECK(guest_byte(before + 0x1080) == 0xab);

    // Nor does one committed while the write is still held back.
    const u64 held = dev + 2 * size;
    gpu_write(held, 0x31);
    cpu_write(held);
    cache->CommitAsyncFlushes();
    cache->PopAsyncFlushes();
    CHECK(guest_byte(held + 0x2000) == 0x31);
    CHECK(guest_byte(held + 0x1080) == 0xab);
    cache.reset();
}

TEST_CASE("BufferCache: obtained buffers count as used by this command buffer",
          "[video_core][buffer_cache]") {
    // DMA buffer/image copies, indirect dispatches and query writes get their buffer through
    // ObtainCPUBuffer. Unless the bytes are marked used, CanReorderUpload lets a later upload to
    // them in the same command buffer run in front of that GPU work (Yoshi and the Mysterious
    // Book: a texture copied from a buffer took the next upload's bytes for one frame).
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
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, size = 0x3000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, size, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, size,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, size, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);

    constexpr u64 addr = dev + 0x1100;
    constexpr u32 bytes = 0x340;
    const auto check = [&](VideoCommon::ObtainBufferSynchronize sync,
                           VideoCommon::ObtainBufferOperation op) {
        const auto [buffer, offset] = cache->ObtainCPUBuffer(addr, bytes, sync, op);
        REQUIRE(buffer->CpuAddr() + offset == addr);
        REQUIRE_FALSE(buffer->marked.empty());
        CHECK(buffer->marked.back() == std::pair<u64, u64>{offset, bytes});
    };
    // An upload source (the copy reads it) and a download target (the copy writes it).
    check(VideoCommon::ObtainBufferSynchronize::FullSynchronize,
          VideoCommon::ObtainBufferOperation::DoNothing);
    check(VideoCommon::ObtainBufferSynchronize::FullSynchronize,
          VideoCommon::ObtainBufferOperation::MarkAsWritten);
    check(VideoCommon::ObtainBufferSynchronize::NoSynchronize,
          VideoCommon::ObtainBufferOperation::DiscardWrite);

    // Uploads mark their bytes too. One that stays in this command buffer is otherwise overtaken
    // by a later upload to the same bytes, and its older data wins.
    const auto covered = [](const FixtureBuffer& buffer, u64 offset, u64 length) {
        return std::ranges::any_of(buffer.marked, [&](const std::pair<u64, u64>& mark) {
            return mark.first <= offset && offset + length <= mark.first + mark.second;
        });
    };
    constexpr u64 inline_addr = dev + 0x2040;
    const auto [written, written_offset] =
        cache->ObtainCPUBuffer(inline_addr, 0x80, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                               VideoCommon::ObtainBufferOperation::MarkAsWritten);
    written->marked.clear();
    const std::array<u8, 0x20> words{};
    REQUIRE(cache->InlineMemory(inline_addr + 0x10, words.size(), words));
    CHECK(covered(*written, written_offset + 0x10, words.size()));

    constexpr u64 cpu_addr = dev + 0x400;
    cache->WriteMemory(cpu_addr, 0x40);
    const auto [synced, synced_offset] =
        cache->ObtainCPUBuffer(cpu_addr, 0x40, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                               VideoCommon::ObtainBufferOperation::DoNothing);
    REQUIRE(synced->marked.size() >= 2);
    synced->marked.pop_back(); // ObtainCPUBuffer's own mark
    CHECK(covered(*synced, synced_offset, 0x40));
    cache.reset();
}

TEST_CASE("BufferCache: completed downloads are reusable without a frame",
          "[video_core][buffer_cache][download_lifetime]") {
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT { Settings::values.use_multi_core.SetValue(old_multi_core); };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT { system.Kernel().UnregisterKernelObject(&process); };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, page = 0x1000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, 3 * page, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 3 * page,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, 3 * page, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);
    const auto write = [&](DAddr address, u8 value) {
        const auto [buffer, offset] = cache->ObtainCPUBuffer(
            address, page, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
            VideoCommon::ObtainBufferOperation::MarkAsWritten);
        std::fill_n(buffer->bytes.begin() + offset, page, value);
    };
    const auto check_guest = [&](DAddr address, u8 expected) {
        std::array<u8, page> bytes{};
        manager.ReadBlockUnsafe(address, bytes.data(), bytes.size());
        CHECK(std::ranges::all_of(bytes, [expected](u8 value) { return value == expected; }));
    };

    // Two fences are outstanding. A later allocation must not reuse either one's storage.
    write(dev, 0x31);
    cache->CommitAsyncFlushes();
    write(dev + page, 0x72);
    cache->CommitAsyncFlushes();
    REQUIRE(runtime.allocation_ids.size() == 2);
    CHECK(runtime.allocation_ids[0] != runtime.allocation_ids[1]);
    CHECK(runtime.freed_ids.empty());
    // These readbacks' fences completed, but unrelated newer work is still outstanding.
    // Retagging completed buffers with the current tick would prevent their immediate reuse.
    runtime.current_tick = 20;
    runtime.gpu_tick = 10;
    cache->PopAsyncFlushes();
    check_guest(dev, 0x31);
    // Popping happens on the fencing thread: release remains a backend operation.
    CHECK(runtime.freed_ids.empty());
    write(dev + 2 * page, 0x53);
    cache->CommitAsyncFlushes();
    REQUIRE(runtime.allocation_ids.size() == 3);
    REQUIRE(runtime.freed_ids.size() == 1);
    CHECK(runtime.freed_ids[0] == runtime.allocation_ids[0]);
    CHECK(runtime.allocation_ids[2] == runtime.allocation_ids[0]);
    CHECK(runtime.deferred_stages[runtime.allocation_ids[1]].active);
    cache->PopAsyncFlushes();
    check_guest(dev + page, 0x72);
    cache->PopAsyncFlushes();
    check_guest(dev + 2 * page, 0x53);

    // More compute-release fences in the same frame reuse completed storage indefinitely.
    for (u8 value : std::array<u8, 4>{0x64, 0x75, 0x86, 0x97}) {
        write(dev + 2 * page, value);
        cache->CommitAsyncFlushes();
        CHECK(runtime.deferred_stages.size() == 2);
        cache->PopAsyncFlushes();
        check_guest(dev + 2 * page, value);
    }
    CHECK_FALSE(runtime.out_of_bounds);
}

TEST_CASE("DeviceMemoryManager: translations distinguish reused manager lifetimes on another thread",
          "[video_core][buffer_cache][translation_lifetime]") {
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT { Settings::values.use_multi_core.SetValue(old_multi_core); };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT { system.Kernel().UnregisterKernelObject(&process); };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, page = 0x1000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, page, 0x31);
    system.DeviceMemory().buffer.ClearBackingRegion(phys + page, page, 0x72);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 2 * page,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);

    using Manager = Tegra::MaxwellDeviceMemoryManager;
    alignas(Manager) std::array<std::byte, sizeof(Manager)> storage;
    auto* manager = std::construct_at(reinterpret_cast<Manager*>(storage.data()),
                                      system.DeviceMemory());
    manager->Map(dev, guest, page, manager->RegisterProcess(&cpu));
    const u64 first_generation = manager->MappingGeneration();
    std::array<u8, 6> observed{};
    std::barrier gate{2};
    std::thread reader([&] {
        observed[0] = manager->Read<u8>(dev);
        manager->ReadBlockUnsafe(dev + 1, &observed[1], 1);
        gate.arrive_and_wait(); // The worker's thread-local cache now holds the first mapping.
        gate.arrive_and_wait(); // Main has reconstructed a manager at the identical address.
        observed[2] = manager->Read<u8>(dev);
        manager->ReadBlockUnsafe(dev + 1, &observed[3], 1);
        gate.arrive_and_wait(); // Retain the second instance's cached mapping.
        gate.arrive_and_wait(); // The same instance now maps different backing.
        observed[4] = manager->Read<u8>(dev);
        manager->ReadBlockUnsafe(dev + 1, &observed[5], 1);
    });
    gate.arrive_and_wait();
    std::destroy_at(manager);
    manager = std::construct_at(reinterpret_cast<Manager*>(storage.data()), system.DeviceMemory());
    const auto second_asid = manager->RegisterProcess(&cpu);
    manager->Map(dev, guest + page, page, second_asid);
    const u64 second_generation = manager->MappingGeneration();
    gate.arrive_and_wait();
    gate.arrive_and_wait();
    // Map replaces the backing and increments the generation. Unmap also calls the GPU
    // interface, which this memory-only fixture deliberately does not create.
    manager->Map(dev, guest, page, second_asid);
    const u64 remapped_generation = manager->MappingGeneration();
    gate.arrive_and_wait();
    reader.join();
    std::destroy_at(manager);

    CHECK(first_generation == second_generation);
    CHECK(remapped_generation != second_generation);
    CHECK(observed == std::array<u8, 6>{0x31, 0x31, 0x72, 0x72, 0x31, 0x31});
}

TEST_CASE("BufferCache: pending download budget waits for delivery and supports cancellation",
          "[video_core][buffer_cache][download_budget]") {
    using namespace std::chrono_literals;
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT { Settings::values.use_multi_core.SetValue(old_multi_core); };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT { system.Kernel().UnregisterKernelObject(&process); };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, page = 0x1000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, 2 * page, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 2 * page,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, 2 * page, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);
    const auto commit = [&](u32 bytes, u8 value) {
        const auto [buffer, offset] = cache->ObtainCPUBuffer(
            dev, bytes, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
            VideoCommon::ObtainBufferOperation::MarkAsWritten);
        std::fill_n(buffer->bytes.begin() + offset, bytes, value);
        cache->CommitAsyncFlushes();
    };
    const auto check_guest = [&](u32 bytes, u8 expected) {
        std::vector<u8> data(bytes);
        manager.ReadBlockUnsafe(dev, data.data(), data.size());
        CHECK(std::ranges::all_of(data, [expected](u8 value) { return value == expected; }));
    };
    const auto wait = [&](bool should_block, bool cancel) {
        std::promise<void> entered;
        auto started = entered.get_future();
        std::promise<bool> completed;
        auto result = completed.get_future();
        std::stop_source stop;
        std::jthread waiter([&] {
            entered.set_value();
            completed.set_value(cache->WaitAsyncFlushBudget(stop.get_token(), page));
        });
        // No Catch assertions until the worker has been cancelled and joined: even a failed
        // readiness/wakeup check must leave neither a blocked worker nor a dangling cache.
        const bool did_start = started.wait_for(2s) == std::future_status::ready;
        const auto initial = result.wait_for(should_block ? 30ms : 2s);
        if (should_block) {
            if (cancel) {
                stop.request_stop();
            } else {
                std::scoped_lock lock{cache->mutex};
                cache->PopAsyncFlushes();
            }
        }
        const bool ready = result.wait_for(2s) == std::future_status::ready;
        stop.request_stop();
        waiter.join();
        const bool success = result.get();
        REQUIRE(did_start);
        CHECK(initial == (should_block ? std::future_status::timeout : std::future_status::ready));
        REQUIRE(ready);
        CHECK(success == !cancel);
    };

    SECTION("reaching the budget blocks until the completed bytes reach guest memory") {
        commit(page, 0x31);
        check_guest(page, 0);
        wait(true, false);
        check_guest(page, 0x31);
        CHECK(runtime.freed_ids.empty()); // Delivery does not release staging on this thread.
    }
    SECTION("completion before waiting cannot lose the wakeup") {
        commit(page, 0x72);
        cache->PopAsyncFlushes();
        check_guest(page, 0x72);
        wait(false, false);
    }
    SECTION("cancellation leaves the pending copy available for later delivery") {
        commit(page, 0x53);
        wait(true, true);
        CHECK(runtime.freed_ids.empty());
        REQUIRE(cache->ShouldWaitAsyncFlushes());
        check_guest(page, 0);
        // A new wait still sees the pending bytes: cancellation neither drops the copy nor
        // forgets its budget. Its eventual pop delivers the original staging contents.
        wait(true, false);
        check_guest(page, 0x53);
    }
    SECTION("one oversized batch is admitted but stalls the following producer wait") {
        wait(false, false);
        commit(2 * page, 0x64);
        REQUIRE(runtime.allocation_ids.size() == 1);
        REQUIRE(runtime.deferred_stages.front().buffer->bytes.size() >= 2 * page);
        wait(true, false);
        check_guest(2 * page, 0x64);
        wait(false, false);
    }
    CHECK_FALSE(runtime.out_of_bounds);
}

TEST_CASE("BufferCache: native fence downloads copy only CPU-visible pages of a large write",
          "[video_core][buffer_cache][selective_download]") {
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    const u64 old_title = Tegra::g_nx2_program_id.exchange(0x04003F40232DE000ULL);
    SCOPE_EXIT {
        Settings::values.use_multi_core.SetValue(old_multi_core);
        Tegra::g_nx2_program_id.store(old_title);
    };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT { system.Kernel().UnregisterKernelObject(&process); };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000;
    constexpr u32 page = 0x1000, extent = 8 * 1024 * 1024;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, extent, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, extent,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, extent, manager.RegisterProcess(&cpu));
    Tegra::MemoryManager address_space(system, manager);
    address_space.EnableNativeTiling();
    FixtureRuntime runtime;
    auto cache = std::make_unique<NativeFixtureCache>(manager, runtime);
    cache->SetAddressSpace(address_space);
    const auto write = [&](u8 value) {
        const auto [buffer, offset] = cache->ObtainCPUBuffer(
            dev, extent, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
            VideoCommon::ObtainBufferOperation::MarkAsWritten);
        std::fill_n(buffer->bytes.begin() + offset, extent, value);
    };
    const auto check_page = [&](DAddr address, u8 expected) {
        std::array<u8, page> bytes{};
        manager.ReadBlockUnsafe(address, bytes.data(), bytes.size());
        CHECK(std::ranges::all_of(bytes, [expected](u8 value) { return value == expected; }));
    };

    write(0x31);
    // The first ordered CPU read downloads one dirty page and marks it Preflushable.
    const DAddr read_page = dev + 3 * page;
    const auto first_read = cache->GetFlushArea(read_page, page);
    REQUIRE(first_read.has_value());
    REQUIRE_FALSE(first_read->preemtive);
    cache->DownloadMemory(first_read->start_address,
                          first_read->end_address - first_read->start_address);
    check_page(read_page, 0x31);

    SECTION("one preflushable page does not expand to the full guessed compute extent") {
        write(0x72);
        cache->CommitAsyncFlushes();
        REQUIRE(runtime.allocation_ids.size() == 1);
        CHECK(runtime.deferred_stages.front().buffer->bytes.size() == page);
        cache->PopAsyncFlushes();
        check_page(read_page, 0x72);
        const DAddr unread_page = dev + 7 * page;
        check_page(unread_page, 0);
        REQUIRE(cache->IsRegionGpuModified(unread_page, page));
        // Selective fence downloads must not lose a later first CPU read elsewhere.
        const auto later_read = cache->GetFlushArea(unread_page, page);
        REQUIRE(later_read.has_value());
        REQUIRE_FALSE(later_read->preemtive);
        cache->DownloadMemory(later_read->start_address,
                              later_read->end_address - later_read->start_address);
        check_page(unread_page, 0x72);
        // Enumeration must retain both persistent read marks for subsequent GPU writes.
        write(0x86);
        cache->CommitAsyncFlushes();
        REQUIRE(runtime.allocation_ids.size() == 2);
        const auto& repeated = runtime.deferred_stages[runtime.allocation_ids.back()];
        CHECK(repeated.buffer->bytes.size() == 2 * page);
        cache->PopAsyncFlushes();
        check_page(read_page, 0x86);
        check_page(unread_page, 0x86);
    }
    SECTION("preflushable and explicit CPU-read pages form a union without duplicate copies") {
        cache->NoteCpuRead(read_page, page); // Overlaps the preflushable mark.
        cache->NoteCpuRead(dev + 5 * page, page); // Separate explicit CPU-read page.
        write(0x53);
        cache->CommitAsyncFlushes();
        REQUIRE(runtime.allocation_ids.size() == 1);
        CHECK(runtime.deferred_stages.front().buffer->bytes.size() == 2 * page);
        cache->PopAsyncFlushes();
        check_page(read_page, 0x53);
        check_page(dev + 5 * page, 0x53);
        check_page(dev + 7 * page, 0);
    }
    SECTION("titles requiring full asynchronous downloads still copy the whole write") {
        Tegra::g_nx2_program_id.store(0x0400A3A01EE4C000ULL); // Pokopia's full-download policy.
        write(0x64);
        cache->CommitAsyncFlushes();
        REQUIRE(runtime.allocation_ids.size() == 1);
        CHECK(runtime.deferred_stages.front().buffer->bytes.size() == extent);
        cache->PopAsyncFlushes();
        check_page(dev, 0x64);
        check_page(dev + extent - page, 0x64);
    }
    CHECK_FALSE(runtime.out_of_bounds);
}
