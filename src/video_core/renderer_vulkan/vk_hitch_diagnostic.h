// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>

#include "common/hitch_diagnostic.h"
#include "common/logging.h"

namespace Vulkan::HitchTrace {

enum class Event : size_t { WorkerChunk, WorkerWait, Submit, GpuWait, Count };

// Each category has a separate cap so a burst of queued chunks cannot consume all GPU-wait logs.
inline bool Claim(Event event, u64 duration_ns) {
    constexpr u64 threshold_ns = 20'000'000;
    if (duration_ns < threshold_ns) {
        return false;
    }
    static std::array<std::atomic<u32>, static_cast<size_t>(Event::Count)> events{};
    const u32 count = events[static_cast<size_t>(event)].fetch_add(1, std::memory_order_relaxed);
    if (count == 128) {
        static constexpr std::array names{"worker_chunk", "worker_wait", "submit", "gpu_wait"};
        LOG_INFO(Render_Vulkan, "VulkanHitch truncated category={} max_events=128 at_ns={}",
                 names[static_cast<size_t>(event)], Common::HitchDiagnostic::NowNs());
    }
    return count < 128;
}

} // namespace Vulkan::HitchTrace
