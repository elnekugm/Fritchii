// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include "common/common_types.h"

namespace FileSys {
// Tennis' build-checked unpacked game NRO, rather than the small nxboot main NSO.
inline constexpr std::array<u8, 32> TennisFpsModuleId{
    0x25, 0x0a, 0x51, 0x0a, 0x93, 0x41, 0x29, 0xcb, 0x38, 0xe9, 0x30, 0xc9,
    0xa2, 0x90, 0x82, 0xb6, 0x97, 0xfd, 0x76, 0xc4};
inline constexpr u32 TennisFpsHookOffset = 0x11f773c;
inline constexpr u32 TennisFpsOriginal = 0x39000288; // strb w8, [x20]
inline constexpr u32 TennisFpsHook = 0xd4000001U | (0xf003U << 5);

inline bool ApplyTennisFpsPatch(std::span<u8> instruction, std::span<const u8> module_id) {
    if (module_id.size() != TennisFpsModuleId.size() ||
        !std::equal(module_id.begin(), module_id.end(), TennisFpsModuleId.begin()) ||
        instruction.size() != sizeof(u32)) {
        return false;
    }
    u32 original{};
    std::memcpy(&original, instruction.data(), sizeof(original));
    if (original != TennisFpsOriginal) {
        return false;
    }
    std::memcpy(instruction.data(), &TennisFpsHook, sizeof(TennisFpsHook));
    return true;
}

// Keep the game's 60 Hz simulation; select which ticks submit rendering work.
inline constexpr bool TennisRenderTick(u32 frame, u32 fps) {
    return fps >= 60 || static_cast<u64>(frame % 60) * fps % 60 < fps;
}
} // namespace FileSys
