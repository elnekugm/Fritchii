// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <cstdlib>

#include "common/common_types.h"

namespace VideoCommon::Nx2FastDraw {

enum class Reason : u8 {
    Invalidation,
    IndirectReset,
    Frame,
    Count,
};

inline constexpr std::array<const char*, static_cast<size_t>(Reason::Count)> reason_names{
    "invalidation", "indirect reset", "frame"};

struct Delta {
    std::array<u32, 8> binds{};
    u32 num_writes{};
    std::array<u64, 32> write_begin{};
    std::array<u64, 32> write_end{};
    bool vertex_buffers{};

    [[nodiscard]] bool Empty() const noexcept {
        if (num_writes != 0 || vertex_buffers) {
            return false;
        }
        for (const u32 bind : binds) {
            if (bind != 0) {
                return false;
            }
        }
        return true;
    }
};

inline Delta g_delta{};
inline std::atomic<u32> g_last_break{};
inline std::atomic<u64> g_serial{};

inline const bool g_enabled = [] {
    const char* value = std::getenv("EDEN_NX2_FAST_DRAW");
    return value != nullptr && value[0] != '0';
}();
inline const bool g_delta_enabled = [] {
    const char* value = std::getenv("EDEN_NX2_FAST_DRAW");
    return value != nullptr && value[0] == '2' && value[1] == '\0';
}();
inline const bool g_verify = std::getenv("EDEN_NX2_FAST_DRAW_VERIFY") != nullptr;
inline const bool g_buffer_skip = std::getenv("EDEN_NX2_BUFFER_SKIP") != nullptr;
inline const bool g_inline_invalidate = std::getenv("EDEN_NX2_INLINE_INVALIDATE") != nullptr;
inline const bool g_dynamic_skip = std::getenv("EDEN_NX2_DYNAMIC_SKIP") != nullptr;

inline u64 Serial() noexcept {
    return g_serial.load(std::memory_order_relaxed);
}

inline void Break(Reason reason, u32 method = 0) noexcept {
    g_last_break.store((method << 8) | static_cast<u32>(reason), std::memory_order_relaxed);
    g_serial.fetch_add(1, std::memory_order_relaxed);
}

} // namespace VideoCommon::Nx2FastDraw
