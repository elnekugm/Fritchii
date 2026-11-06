// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "core/file_sys/wonder_fps_patch.h"

namespace {
constexpr std::array<u32, 3> Offsets{0x1bbfb0, 0x1bbfcc, 0x1bc04c};
constexpr std::array<u32, 3> Original{0x1aca0d01, 0xb9404281, 0xb9405281};
std::vector<u8> MakeModule() {
    std::vector<u8> module(0x100 + Offsets.back() + 4);
    for (size_t i = 0; i < Offsets.size(); ++i) {
        std::memcpy(module.data() + 0x100 + Offsets[i], &Original[i], 4);
    }
    return module;
}
u32 ReadWord(const std::vector<u8>& module, size_t index) {
    u32 word{};
    std::memcpy(&word, module.data() + 0x100 + Offsets[index], 4);
    return word;
}
} // namespace

TEST_CASE("Wonder FPS patch keeps simulation and presentation together", "[wonder_fps]") {
    constexpr std::array targets{Settings::WonderFps::Fps30,  Settings::WonderFps::Fps60,
                                 Settings::WonderFps::Fps90,  Settings::WonderFps::Fps120,
                                 Settings::WonderFps::Fps180, Settings::WonderFps::Fps240};
    constexpr std::array<u32, 6> fps_values{30, 60, 90, 120, 180, 240};
    for (size_t i = 0; i < targets.size(); ++i) {
        auto module = MakeModule();
        REQUIRE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                             FileSys::WonderFpsBuildId, targets[i]));
        // Decode MOVZ W1 immediates, independently checking simulation Hz and NVN interval.
        for (size_t site = 0; site < 3; ++site) {
            CHECK((ReadWord(module, site) & 0xffe0001fU) == 0x52800001U);
        }
        CHECK(((ReadWord(module, 0) >> 5) & 0xffff) == fps_values[i]);
        CHECK(((ReadWord(module, 2) >> 5) & 0xffff) == fps_values[i]);
        CHECK(((ReadWord(module, 1) >> 5) & 0xffff) == (fps_values[i] == 30 ? 2 : 1));
    }
}

TEST_CASE("Wonder FPS patch rejects unknown games builds and altered or truncated code atomically",
          "[wonder_fps]") {
    auto module = MakeModule();
    const auto original = module;
    CHECK_FALSE(FileSys::ApplyWonderFpsPatch(module, Settings::BananzaFpsProgramId,
                                             FileSys::WonderFpsBuildId,
                                             Settings::WonderFps::Fps60));
    CHECK_FALSE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                             "other build", Settings::WonderFps::Fps60));
    CHECK(module == original);
    module.back() ^= 1;
    const auto altered = module;
    CHECK_FALSE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                             FileSys::WonderFpsBuildId,
                                             Settings::WonderFps::Fps60));
    CHECK(module == altered);
    module = original;
    module.pop_back();
    const auto truncated = module;
    CHECK_FALSE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                             FileSys::WonderFpsBuildId,
                                             Settings::WonderFps::Fps60));
    CHECK(module == truncated);
}

TEST_CASE("Wonder FPS selector supersedes standalone 60 FPS mod", "[wonder_fps]") {
    auto module = MakeModule();
    REQUIRE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                         FileSys::WonderFpsBuildId, Settings::WonderFps::Fps60));
    REQUIRE(FileSys::ApplyWonderFpsPatch(module, Settings::WonderSwitch2ProgramId,
                                         FileSys::WonderFpsBuildId, Settings::WonderFps::Fps240));
    CHECK(((ReadWord(module, 0) >> 5) & 0xffff) == 240);
}

TEST_CASE("Game FPS controls are visible only in their own game's properties", "[wonder_fps]") {
    for (const auto id : {u64{0}, Settings::WonderSwitch2ProgramId, Settings::BananzaFpsProgramId,
                          u64{0x010015100B514000}}) {
        CHECK_FALSE(Settings::ShowWonderFps(id, true));
        CHECK_FALSE(Settings::ShowBananzaFps(id, true));
        CHECK(Settings::ShowWonderFps(id, false) == (id == Settings::WonderSwitch2ProgramId));
        CHECK(Settings::ShowBananzaFps(id, false) == (id == Settings::BananzaFpsProgramId));
    }
}
