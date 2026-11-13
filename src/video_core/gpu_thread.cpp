// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <thread>
#if defined(ARCHITECTURE_x86_64)
#include <immintrin.h>
#endif

#include "common/assert.h"
#include "common/dkb_cpu_trace.h"
#include "common/nx2_timeline.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/frontend/graphics_context.h"
#include "video_core/nx2_debug_control.h"
#include "video_core/control/scheduler.h"
#include "video_core/dma_pusher.h"
#include "video_core/gpu.h"
#include "video_core/gpu_thread.h"
#include "video_core/gpu_split.h"
#include "video_core/nx2_profile.h"
#include "video_core/readback_diagnostics.h"
#include "video_core/host1x/host1x.h"
#include "video_core/nx2.h"
#include "video_core/renderer_base.h"

namespace VideoCommon::GPUThread {

namespace {

VideoCore::RasterizerDownloadArea ReadMemory(VideoCore::RasterizerInterface& rasterizer,
                                           const ThreadManager& manager, const SynchState& state,
                                           DAddr addr, u64 size, u64 published_work_sequence,
                                           ReadbackDependencyDiagnostics::Record* diagnostic) {
    if (diagnostic) {
        diagnostic->backend_start = ReadbackDependencyDiagnostics::Now();
    }
    const u64 generation = manager.ReadbackGeneration();
    auto area = rasterizer.GetFlushArea(addr, size);
    if (diagnostic) {
        diagnostic->backend_generation = generation;
        diagnostic->area_end = ReadbackDependencyDiagnostics::Now();
        diagnostic->preemptive_flag = area.preemtive;
    }
    rasterizer.FlushRegion(area.start_address, area.end_address - area.start_address);
    area.preemtive = true;
    if (diagnostic) {
        diagnostic->flush_end = ReadbackDependencyDiagnostics::Now();
        diagnostic->checked_generation = manager.ReadbackGeneration();
        diagnostic->checked_sequence = state.published_work_sequence.load(std::memory_order_acquire);
        area.completed_generation =
            generation != 0 && diagnostic->checked_generation == generation &&
                    diagnostic->checked_sequence == published_work_sequence
                ? generation
                : 0;
    } else {
        area.completed_generation =
            generation != 0 && manager.ReadbackGeneration() == generation &&
                    state.published_work_sequence.load(std::memory_order_acquire) == published_work_sequence
                ? generation
                : 0;
    }
    return area;
}

// Tells a CPU thread waiting in PushCommand that its command ran.
void SignalCommand(SynchState& state, u64 fence, bool block) {
    state.signaled_fence.store(fence, std::memory_order_release);
    if (block) {
        state.signaled_fence.notify_all();
    }
}

} // Anonymous namespace

ThreadManager::ThreadManager(Core::System& system_)
    : system{system_}
{}

ThreadManager::~ThreadManager() = default;

void ThreadManager::StartThread(VideoCore::RendererBase& renderer, Core::Frontend::GraphicsContext& context, Tegra::Control::Scheduler& scheduler) {
    rasterizer = renderer.ReadRasterizer();
    thread = std::jthread([&](std::stop_token stop_token) {
        const Common::DkbCpuTrace::Registration cpu_trace{Common::DkbCpuTrace::Role::GpuCommand};
        Common::SetCurrentThreadName("GPU");
        Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
        Common::PinCurrentThread(Common::PinnedRole::GpuFront);
        Common::SetCurrentThreadToPerformanceCores();
        VideoCommon::GpuSplit::PinCurrentThread("EDEN_NX2_PIN_FRONT");
        system.RegisterHostThread();
        Tegra::Nx2SemaphoreWaiters::Get().SetWakeEvent(&state.pending_wake);
        SCOPE_EXIT {
            Tegra::Nx2SemaphoreWaiters::Get().SetWakeEvent(nullptr);
        };

        auto current_context = context.Acquire();
        CommandDataContainer next;
        // When a pending channel last moved or a command arrived (see the polling below).
        auto last_progress = std::chrono::steady_clock::now();
        u32 idle_polls = 0;
        namespace Profile = VideoCommon::Nx2Profile;
        // Before the split's back thread starts, which reads it too.
        Profile::Initialize();
        auto& split = VideoCommon::GpuSplit::Get();
        while (true) {
            next = {};
            Profile::Report();
            split.MaybeStart(system);
            if (stop_token.stop_requested()) {
                if (!state.queue.TryPop(next)) {
                    break;
                }
            } else if (scheduler.HasPendingChannels()) {
                // Suspended channels wait on semaphores the CPU releases: poll them meanwhile.
                bool progressed = false;
                {
                    Profile::Scope scope{Profile::Zone::ResumePending};
                    progressed = scheduler.ResumePending(system.GPU());
                }
                if (progressed) {
                    state.idle_on_pending.store(false, std::memory_order_release);
                    last_progress = std::chrono::steady_clock::now();
                    idle_polls = 0;
                }
                if (!state.queue.TryPop(next)) {
                    if (!scheduler.HasPendingChannels()) {
                        state.idle_on_pending.store(false, std::memory_order_release);
                        continue;
                    }
                    if (!state.idle_on_pending.load(std::memory_order_relaxed) &&
                        state.queue.Empty()) {
                        state.idle_on_pending.store(true, std::memory_order_seq_cst);
                        if (!state.queue.Empty()) {
                            state.idle_on_pending.store(false, std::memory_order_release);
                            continue;
                        }
                        std::shared_ptr<Common::Event> to_wake;
                        {
                            std::scoped_lock lk{state.pending_idle_mutex};
                            to_wake = state.pending_idle_wake.lock();
                        }
                        if (to_wake) {
                            to_wake->Set();
                        }
                    }
                    Profile::Scope scope{Profile::Zone::PendingSleep};
                    const auto idle_for = std::chrono::steady_clock::now() - last_progress;
                    if (idle_polls == 0) {
                        ++idle_polls;
#if defined(ARCHITECTURE_x86_64)
                        for (u32 i = 0; i < 256; ++i) {
                            _mm_pause();
                        }
#else
                        std::this_thread::yield();
#endif
                    } else if (idle_for < std::chrono::milliseconds{2}) {
                        ++idle_polls;
                        Common::SetCurrentThreadPriority(Common::ThreadPriority::Normal);
#if defined(ARCHITECTURE_x86_64)
                        for (u32 i = 0; i < 256; ++i) {
                            _mm_pause();
                        }
#endif
                        std::this_thread::yield();
                        Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
                    } else {
                        state.pending_wake.WaitFor(std::chrono::milliseconds{1});
                    }
                }
                idle_polls = 0;
                state.idle_on_pending.store(false, std::memory_order_release);
            } else {
                idle_polls = 0;
                state.idle_on_pending.store(false, std::memory_order_release);
                Profile::Scope scope{Profile::Zone::Idle};
                state.queue.PopWait(next, stop_token);
            }
            if (std::holds_alternative<std::monostate>(next.data)) {
                continue;
            }
            if (const auto* read = std::get_if<CpuReadCommand>(&next.data);
                read && read->diagnostic) {
                read->diagnostic->front_popped = ReadbackDependencyDiagnostics::Now();
                read->diagnostic->split = split.Active();
            }
            last_progress = std::chrono::steady_clock::now();
            if (split.Active()) {
                // Only command lists are parsed here; everything else runs on the back thread
                // in order, and so does the signal that the command ran.
                auto* const rasterizer_ptr = renderer.ReadRasterizer();
                if (auto* submit_list = std::get_if<SubmitListCommand>(&next.data)) {
                    const bool timeline = Common::Nx2Timeline::Active();
                    if (timeline) {
                        // The list's sequence number (PushCommand's "submit list") on each
                        // stage: here, and from a one-entry marker where the back reaches it.
                        Common::Nx2Timeline::Instant("front: list begin", {"seq", next.fence},
                                                     {"channel", submit_list->channel});
                        split.Defer(
                            [seq = next.fence] {
                                Common::Nx2Timeline::Instant("back: list", {"seq", seq});
                            },
                            VideoCommon::GpuSplit::Effect::None);
                    }
                    {
                        Profile::Scope scope{Profile::Zone::Push};
                        scheduler.Push(system.GPU(), submit_list->channel,
                                       std::move(submit_list->entries));
                    }
                    if (timeline) {
                        Common::Nx2Timeline::Instant("front: list end", {"seq", next.fence});
                    }
                } else if (std::holds_alternative<GPUTickCommand>(next.data)) {
                    // TickWork ends fast draw reuse itself when it runs a request.
                    split.Defer([gpu = &system.GPU()] {
                        Profile::Scope scope{Profile::Zone::Tick};
                        gpu->TickWork();
                    }, VideoCommon::GpuSplit::FastDrawNeutral);
                } else if (auto* run = std::get_if<RunCommand>(&next.data)) {
                    auto wrapped = [fn = std::move(run->func)]() mutable {
                        Profile::Scope scope{Profile::Zone::Run};
                        fn();
                    };
                    if (run->before_later_reads) {
                        // GPU memory maps unmap caches (UnmapMemory downloads): the back's state.
                        split.DeferFunction(std::move(wrapped));
                        split.Drain(VideoCommon::DrainSite::Map);
                    } else {
                        split.DeferFunction(std::move(wrapped));
                        split.NoteMemoryWrite();
                    }
                } else if (const auto* flush = std::get_if<FlushRegionCommand>(&next.data)) {
                    split.Defer([rasterizer_ptr, addr = flush->addr, size = flush->size] {
                        Profile::Scope scope{Profile::Zone::FlushInvalidate};
                        rasterizer_ptr->FlushRegion(addr, size);
                    });
                } else if (const auto* invalidate = std::get_if<InvalidateRegionCommand>(&next.data)) {
                    split.Defer([rasterizer_ptr, addr = invalidate->addr, size = invalidate->size] {
                        Profile::Scope scope{Profile::Zone::FlushInvalidate};
                        rasterizer_ptr->OnCacheInvalidation(addr, size);
                    });
                } else {
                    ASSERT(false);
                }
                split.Defer(
                    [state_ptr = &state, fence = next.fence, block = next.block] {
                        SignalCommand(*state_ptr, fence, block);
                    },
                    VideoCommon::GpuSplit::Effect::None);
                split.Publish();
                last_progress = std::chrono::steady_clock::now();
                continue;
            }
            if (auto* submit_list = std::get_if<SubmitListCommand>(&next.data)) {
                const bool timeline = Common::Nx2Timeline::Active();
                if (timeline) {
                    Common::Nx2Timeline::Instant("front: list begin", {"seq", next.fence},
                                                 {"channel", submit_list->channel});
                }
                {
                    Profile::Scope scope{Profile::Zone::Push};
                    scheduler.Push(system.GPU(), submit_list->channel,
                                   std::move(submit_list->entries));
                }
                if (timeline) {
                    Common::Nx2Timeline::Instant("front: list end", {"seq", next.fence});
                }
            } else if (std::holds_alternative<GPUTickCommand>(next.data)) {
                Profile::Scope scope{Profile::Zone::Tick};
                system.GPU().TickWork();
            } else if (auto* run = std::get_if<RunCommand>(&next.data)) {
                Profile::Scope scope{Profile::Zone::Run};
                run->func();
            } else if (const auto* read = std::get_if<CpuReadCommand>(&next.data)) {
                *read->result = GPUThread::ReadMemory(*renderer.ReadRasterizer(), *this, state,
                                                    read->addr, read->size,
                                                    next.published_work_sequence, read->diagnostic);
            } else if (const auto* flush = std::get_if<FlushRegionCommand>(&next.data)) {
                Profile::Scope scope{Profile::Zone::FlushInvalidate};
                renderer.ReadRasterizer()->FlushRegion(flush->addr, flush->size);
            } else if (const auto* invalidate = std::get_if<InvalidateRegionCommand>(&next.data)) {
                Profile::Scope scope{Profile::Zone::FlushInvalidate};
                renderer.ReadRasterizer()->OnCacheInvalidation(invalidate->addr, invalidate->size);
            } else {
                ASSERT(false);
            }
            SignalCommand(state, next.fence, next.block);
        }
        if (split.Active()) {
            split.Drain();
        }
    });
}

void ThreadManager::SubmitList(s32 channel, Tegra::CommandList&& entries, bool is_async) {
    PushCommand(SubmitListCommand(channel, std::move(entries)), false, is_async);
}

void ThreadManager::FlushRegion(DAddr addr, u64 size, bool is_async) {
    if (!is_async) {
        // Always flush with synchronous GPU mode
        PushCommand(FlushRegionCommand(addr, size), false, is_async);
    }
}

void ThreadManager::TickGPU(bool is_async) {
    PushCommand(GPUTickCommand(), false, is_async);
}

void ThreadManager::Run(std::function<void()>&& func, bool is_async, bool before_later_reads) {
    PushCommand(RunCommand{std::move(func), before_later_reads}, false, is_async);
}

void ThreadManager::InvalidateRegion(DAddr addr, u64 size) {
    BeginAsyncGuestWrite();
    SCOPE_EXIT { EndAsyncGuestWrite(); };
    rasterizer->OnCacheInvalidation(addr, size);
}

void ThreadManager::FlushAndInvalidateRegion(DAddr addr, u64 size, bool is_async) {
    if (VideoCore::Nx2Debug::AccuratePart(8)) {
        if (!is_async) {
            PushCommand(FlushRegionCommand(addr, size), false, is_async);
        } else {
            auto& gpu = system.GPU();
            const u64 fence = gpu.RequestFlush(addr, size);
            TickGPU(is_async);
            gpu.WaitForSyncOperation(fence);
        }
    }
    rasterizer->OnCacheInvalidation(addr, size);
}

u64 ThreadManager::PushCommand(CommandData&& command_data, bool block, bool is_async) {
    if (!is_async) {
        // In synchronous GPU mode, block the caller until the command has executed
        block = true;
    }

    if (const auto* read = std::get_if<CpuReadCommand>(&command_data);
        read && read->diagnostic) {
        read->diagnostic->before_lock = ReadbackDependencyDiagnostics::Now();
    }
    std::unique_lock lk(state.write_lock);
    if (state.shutting_down) {
        return 0;
    }
    if (!std::holds_alternative<CpuReadCommand>(command_data)) {
        state.published_work_sequence.fetch_add(1, std::memory_order_acq_rel);
        InvalidateReadbackCache();
    }
    const u64 fence{++state.last_fence};
    if (Common::Nx2Timeline::Active()) {
        // The command's sequence number, which the GPU threads' list markers repeat
        // (tools/nx2/critical_path.py follows a list through the stages with it). This is also
        // what wakes the front from "idle".
        if (const auto* list = std::get_if<SubmitListCommand>(&command_data)) {
            Common::Nx2Timeline::Instant("submit list", {"seq", fence}, {"channel", list->channel},
                                         {"entries", list->entries.command_lists.size()});
        } else {
            Common::Nx2Timeline::Instant("submit command", {"seq", fence});
        }
    }
    state.queue.EmplaceWait(std::move(command_data), fence, block,
                            state.published_work_sequence.load(std::memory_order_acquire));
    state.pending_wake.Set();

    lk.unlock();
    if (block) {
        // With the split, SignalCommand runs on the back.
        Common::Nx2Timeline::ArgSpan timeline_span{"guest: wait for GPU command", {"seq", fence}};
        state.cv.wait(lk, thread.get_stop_token(), [this, fence] {
            return fence <= state.signaled_fence.load(std::memory_order_relaxed);
        });
    }

    return fence;
}

void ThreadManager::NotifyShutdown() {
    if (thread.joinable()) {
        thread.request_stop();
        state.pending_wake.Set();
        thread.join();
    }
    VideoCommon::GpuSplit::Get().Stop();
}

} // namespace VideoCommon::GPUThread
