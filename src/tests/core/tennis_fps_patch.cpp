// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cstring>
#include <catch2/catch_test_macros.hpp>
#include "common/game_fps.h"
#include "core/file_sys/tennis_fps_patch.h"

TEST_CASE("Tennis patch checks module and instruction before writing", "[tennis_fps]") {
    // Bytes captured from the unpacked local game module, independent of patch constants.
    constexpr std::array<u8, 32> module_id{
        0x25, 0x0a, 0x51, 0x0a, 0x93, 0x41, 0x29, 0xcb, 0x38, 0xe9, 0x30, 0xc9,
        0xa2, 0x90, 0x82, 0xb6, 0x97, 0xfd, 0x76, 0xc4};
    std::array<u8, 4> code{0x88, 0x02, 0x00, 0x39};
    const auto original = code;
    auto id = module_id;
    id[0] ^= 1;
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(code, id));
    REQUIRE(code == original);
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(std::span{code}.first(3), module_id));
    REQUIRE(code == original);
    REQUIRE(FileSys::ApplyTennisFpsPatch(code, module_id));
    u32 word{};
    std::memcpy(&word, code.data(), code.size());
    REQUIRE(word == 0xd41e0061U);
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(code, module_id));
}

TEST_CASE("Tennis render selection leaves simulation ticks alone", "[tennis_fps]") {
    for (u32 fps : {1U, 24U, 30U, 40U, 45U, 60U}) {
        u32 rendered = 0;
        for (u32 tick = 1; tick <= 600; ++tick) {
            rendered += FileSys::TennisRenderTick(tick, fps);
        }
        REQUIRE(rendered == fps * 10);
    }
    REQUIRE(Settings::ShowTennisFps(Settings::TennisFpsProgramId, false));
    REQUIRE_FALSE(Settings::ShowTennisFps(Settings::WonderSwitch2ProgramId, false));
    REQUIRE_FALSE(Settings::ShowTennisFps(Settings::TennisFpsProgramId, true));
}
