// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstring>
#include <span>
#include <string_view>
#include "common/game_fps.h"

namespace FileSys {

inline constexpr std::string_view WonderFpsBuildId = "EC47E6F8846D2A7B6892E2682845C7255F7117AE";

// Module offsets; the loader passes its decompressed NSO including the 0x100-byte header.
// Keep simulation FPS and NVN's present interval together: changing only presentation makes
// the game run at twice its intended speed. The fallback is used before timing samples exist.
inline bool ApplyWonderFpsPatch(std::span<u8> nso, u64 program_id, std::string_view build_id,
                                Settings::WonderFps target) {
    if (program_id != Settings::WonderSwitch2ProgramId || build_id != WonderFpsBuildId) {
        return false;
    }
    constexpr std::array<u32, 3> offsets{0x1bbfb0, 0x1bbfcc, 0x1bc04c};
    constexpr std::array<u32, 3> original{0x1aca0d01, 0xb9404281, 0xb9405281};
    // Also accept the standalone 60 FPS test mod installed before this built-in option.
    constexpr std::array<u32, 3> legacy{0x52800781, 0x52800021, 0x52800781};
    for (size_t i = 0; i < offsets.size(); ++i) {
        const size_t at = 0x100 + offsets[i];
        if (at + sizeof(u32) > nso.size())
            return false;
        u32 instruction{};
        std::memcpy(&instruction, nso.data() + at, sizeof(instruction));
        if (instruction != original[i] && instruction != legacy[i])
            return false;
    }
    const u32 fps = Settings::WonderFpsValue(target);
    const u32 interval = fps == 30 ? 2 : 1;
    const std::array<u32, 3> replacements{0x52800001U | (fps << 5), 0x52800001U | (interval << 5),
                                          0x52800001U | (fps << 5)};
    for (size_t i = 0; i < offsets.size(); ++i) {
        std::memcpy(nso.data() + 0x100 + offsets[i], &replacements[i], sizeof(u32));
    }
    return true;
}

} // namespace FileSys
