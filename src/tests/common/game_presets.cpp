// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "common/game_presets.h"

namespace {
void SetConflictingPreferences(Settings::Values& settings, bool per_game) {
    const auto set = [per_game](auto& setting, auto value) {
        setting.SetGlobal(!per_game);
        setting.SetValue(value);
    };
    set(settings.renderer_backend, Settings::RendererBackend::Null);
    set(settings.resolution_setup, Settings::ResolutionSetup::Res2X);
    set(settings.gpu_accuracy, Settings::GpuAccuracy::High);
    set(settings.use_asynchronous_shaders, false);
    set(settings.astc_recompression, Settings::AstcRecompression::Uncompressed);
    set(settings.vram_usage_mode, Settings::VramUsageMode::Aggressive);
    set(settings.use_reactive_flushing, true);
    set(settings.vsync_mode, Settings::VSyncMode::Immediate);
    set(settings.scaling_filter, Settings::ScalingFilter::NearestNeighbor);
    set(settings.anti_aliasing, Settings::AntiAliasing::Smaa);
}

void CheckPreset(const Settings::Values& settings) {
    CHECK(settings.renderer_backend.GetValue() == Settings::RendererBackend::Vulkan);
    CHECK(settings.resolution_setup.GetValue() == Settings::ResolutionSetup::Res1X);
    CHECK(settings.gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low);
    CHECK(settings.use_asynchronous_shaders.GetValue());
    CHECK(settings.astc_recompression.GetValue() == Settings::AstcRecompression::Bc3);
    CHECK(settings.vram_usage_mode.GetValue() == Settings::VramUsageMode::Conservative);
    CHECK_FALSE(settings.use_reactive_flushing.GetValue());
    CHECK(settings.vsync_mode.GetValue() == Settings::VSyncMode::FifoRelaxed);
    CHECK(settings.use_docked_mode.GetValue() == Settings::ConsoleMode::Handheld);
    CHECK(settings.scaling_filter.GetValue() == Settings::ScalingFilter::ScaleForce);
    CHECK(settings.anti_aliasing.GetValue() == Settings::AntiAliasing::Fxaa);
}

void CheckPreferences(const Settings::Values& settings, bool global = false) {
    CHECK(settings.renderer_backend.GetValue(global) == Settings::RendererBackend::Null);
    CHECK(settings.resolution_setup.GetValue(global) == Settings::ResolutionSetup::Res2X);
    CHECK(settings.gpu_accuracy.GetValue(global) == Settings::GpuAccuracy::High);
    CHECK_FALSE(settings.use_asynchronous_shaders.GetValue(global));
    CHECK(settings.astc_recompression.GetValue(global) ==
          Settings::AstcRecompression::Uncompressed);
    CHECK(settings.vram_usage_mode.GetValue(global) == Settings::VramUsageMode::Aggressive);
    CHECK(settings.use_reactive_flushing.GetValue(global));
    CHECK(settings.vsync_mode.GetValue(global) == Settings::VSyncMode::Immediate);
    CHECK(settings.scaling_filter.GetValue(global) == Settings::ScalingFilter::NearestNeighbor);
    CHECK(settings.anti_aliasing.GetValue(global) == Settings::AntiAliasing::Smaa);
}
} // namespace

TEST_CASE("Display filter defaults use ScaleForce and no extra AA", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    CHECK(settings->scaling_filter.GetDefault() == Settings::ScalingFilter::ScaleForce);
    CHECK(settings->anti_aliasing.GetDefault() == Settings::AntiAliasing::None);
}

TEST_CASE("Accepted MKW settings replace visible global preferences", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    Settings::SetMarioKartWorldRecommendedSettings(*settings, true);
    CheckPreset(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->gpu_accuracy.UsingGlobal());
    CHECK(settings->renderer_backend.GetValue(true) == Settings::RendererBackend::Vulkan);
    CHECK(settings->gpu_accuracy.GetValue(true) == Settings::GpuAccuracy::Low);
}

TEST_CASE("MKW preset overrides saved settings for the session", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    SetConflictingPreferences(*settings, true);
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::MarioKartWorldProgramId, true));
    CheckPreset(*settings);
    CheckPreferences(*settings, true);
    for (const auto& restore : settings->linkage.restore_functions) {
        restore();
    }
    CheckPreferences(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->resolution_setup.UsingGlobal());
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::MarioKartWorldProgramId, true));
    CheckPreset(*settings);
}

TEST_CASE("MKW preset does not change unrelated games", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    CHECK_FALSE(Settings::ApplyGamePreset(*settings, 0, true));
    CHECK_FALSE(Settings::ApplyGamePreset(*settings, Settings::MarioKartWorldProgramId + 1, true));
    CheckPreferences(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->resolution_setup.UsingGlobal());
}

TEST_CASE("Declining the MKW preset preserves global and per-game preferences", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    for (bool per_game : {false, true}) {
        SetConflictingPreferences(*settings, per_game);
        CHECK_FALSE(Settings::ApplyGamePreset(*settings, Settings::MarioKartWorldProgramId, false));
        CheckPreferences(*settings);
        CHECK(settings->renderer_backend.UsingGlobal() == !per_game);
        CHECK(settings->resolution_setup.UsingGlobal() == !per_game);
    }
}

TEST_CASE("Every Switch 2 preset applies to its own title only", "[game_presets]") {
    for (const auto& preset : Settings::GamePresets) {
        auto settings = std::make_unique<Settings::Values>();
        SetConflictingPreferences(*settings, false);
        CHECK_FALSE(Settings::ApplyGamePreset(*settings, preset.program_id, false));
        CheckPreferences(*settings);
        REQUIRE(Settings::ApplyGamePreset(*settings, preset.program_id, true));
        CHECK(settings->renderer_backend.GetValue() == Settings::RendererBackend::Vulkan);
        CHECK(settings->resolution_setup.GetValue() == Settings::ResolutionSetup::Res1X);
        CHECK(settings->gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low);
        // Ocarina of Time keeps reactive flushing on (checked below).
        if (preset.program_id != Settings::OcarinaOfTimeProgramId) {
            CHECK_FALSE(settings->use_reactive_flushing.GetValue());
        }
        CheckPreferences(*settings, true);
        CHECK(Settings::FindGamePreset(preset.program_id) == &preset);
    }
    CHECK(Settings::FindGamePreset(0) == nullptr);
}

TEST_CASE("Title presets set what the titles need", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::DonkeyKongBananzaProgramId, true));
    CHECK(settings->gpu_clock.GetValue() == Settings::GpuClock::Boost);
    CHECK(settings->fps30_patch.GetValue());
    settings = std::make_unique<Settings::Values>();
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::HyruleWarriorsAgeOfImprisonmentProgramId,
                                      true));
    CHECK(settings->cpu_clock.GetValue() == Settings::CpuClock::Normal);
    CHECK(settings->gpu_fence_behavior.GetValue() == Settings::GpuFenceBehavior::Balanced);
    settings = std::make_unique<Settings::Values>();
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::MarioTennisFeverProgramId, true));
    CHECK(settings->gpu_fence_behavior.GetValue() == Settings::GpuFenceBehavior::Balanced);
    CHECK(settings->async_presentation.GetValue());
    settings = std::make_unique<Settings::Values>();
    REQUIRE(Settings::ApplyGamePreset(*settings, Settings::OcarinaOfTimeProgramId, true));
    CHECK(settings->use_reactive_flushing.GetValue());
    CHECK(settings->fps30_patch.GetValue());
}

TEST_CASE("Graphics quality tokens are stable and legacy environment answers remain deterministic",
          "[game_presets]") {
    using Quality = Settings::GamePresetQuality;
    for (const auto quality : {Quality::Low, Quality::Standard, Quality::High}) {
        const auto token = Settings::GamePresetQualityToken(quality);
        REQUIRE(Settings::ParseGamePresetQuality(token) == quality);
        CHECK(Settings::GamePresetEnvironmentQuality(token) == quality);
    }
    CHECK(Settings::GamePresetEnvironmentQuality("1") == Quality::Standard);
    CHECK_FALSE(Settings::GamePresetEnvironmentQuality("0"));
    for (const auto token : {"", "2", "HIGH", "unknown", " low"}) {
        CHECK_FALSE(Settings::ParseGamePresetQuality(token));
        CHECK_FALSE(Settings::GamePresetEnvironmentQuality(token));
    }
}

TEST_CASE("Every visual quality retains each title's compatibility settings and saved globals",
          "[game_presets]") {
    using Quality = Settings::GamePresetQuality;
    for (const auto& preset : Settings::GamePresets) {
        for (const auto quality : {Quality::Low, Quality::Standard, Quality::High}) {
            auto settings = std::make_unique<Settings::Values>();
            SetConflictingPreferences(*settings, false);
            SetConflictingPreferences(*settings, true);
            settings->max_anisotropy.SetGlobal(true);
            settings->max_anisotropy.SetValue(Settings::AnisotropyMode::X8);
            REQUIRE(Settings::ApplyGamePreset(*settings, preset.program_id, quality));
            CHECK(settings->resolution_setup.GetValue() ==
                  (quality == Quality::Low    ? Settings::ResolutionSetup::Res3_4X
                   : quality == Quality::High ? Settings::ResolutionSetup::Res2X
                                              : Settings::ResolutionSetup::Res1X));
            CHECK(settings->anti_aliasing.GetValue() == (quality == Quality::High
                                                             ? Settings::AntiAliasing::Smaa
                                                             : Settings::AntiAliasing::Fxaa));
            CHECK(settings->astc_recompression.GetValue() ==
                  (quality == Quality::High ? Settings::AstcRecompression::Uncompressed
                                            : Settings::AstcRecompression::Bc3));
            CHECK(settings->max_anisotropy.GetValue() ==
                  (quality == Quality::Low    ? Settings::AnisotropyMode::X2
                   : quality == Quality::High ? Settings::AnisotropyMode::X16
                                              : Settings::AnisotropyMode::X4));
            CHECK(settings->renderer_backend.GetValue() == Settings::RendererBackend::Vulkan);
            CHECK(settings->gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low);
            CHECK(settings->use_asynchronous_shaders.GetValue());
            CHECK(settings->vram_usage_mode.GetValue() == Settings::VramUsageMode::Conservative);
            CHECK_FALSE(settings->use_reactive_flushing.GetValue());
            CHECK(settings->vsync_mode.GetValue() == Settings::VSyncMode::FifoRelaxed);
            CHECK(settings->scaling_filter.GetValue() == Settings::ScalingFilter::ScaleForce);
            switch (preset.program_id) {
            case Settings::MarioKartWorldProgramId:
                CHECK(settings->use_docked_mode.GetValue() == Settings::ConsoleMode::Handheld);
                break;
            case Settings::DonkeyKongBananzaProgramId:
                CHECK(settings->gpu_clock.GetValue() == Settings::GpuClock::Boost);
                CHECK(settings->fps30_patch.GetValue());
                break;
            case Settings::HyruleWarriorsAgeOfImprisonmentProgramId:
                CHECK(settings->cpu_clock.GetValue() == Settings::CpuClock::Normal);
                CHECK(settings->gpu_fence_behavior.GetValue() ==
                      Settings::GpuFenceBehavior::Balanced);
                break;
            case Settings::MarioTennisFeverProgramId:
                CHECK(settings->gpu_fence_behavior.GetValue() ==
                      Settings::GpuFenceBehavior::Balanced);
                CHECK(settings->async_presentation.GetValue());
                break;
            }
            CheckPreferences(*settings, true);
            CHECK(settings->max_anisotropy.GetValue(true) == Settings::AnisotropyMode::X8);
            for (const auto& restore : settings->linkage.restore_functions) {
                restore();
            }
            CheckPreferences(*settings);
            CHECK(settings->max_anisotropy.GetValue() == Settings::AnisotropyMode::X8);
        }
    }
}

TEST_CASE("Changing quality replaces every visual override and ignores unsupported titles",
          "[game_presets]") {
    using Quality = Settings::GamePresetQuality;
    auto settings = std::make_unique<Settings::Values>();
    const u64 title = Settings::MarioTennisFeverProgramId;
    REQUIRE(Settings::ApplyGamePreset(*settings, title, Quality::High));
    REQUIRE(Settings::ApplyGamePreset(*settings, title, Quality::Low));
    CHECK(settings->resolution_setup.GetValue() == Settings::ResolutionSetup::Res3_4X);
    CHECK(settings->anti_aliasing.GetValue() == Settings::AntiAliasing::Fxaa);
    CHECK(settings->astc_recompression.GetValue() == Settings::AstcRecompression::Bc3);
    CHECK(settings->max_anisotropy.GetValue() == Settings::AnisotropyMode::X2);
    REQUIRE(Settings::ApplyGamePreset(*settings, title, true));
    CHECK(settings->resolution_setup.GetValue() == Settings::ResolutionSetup::Res1X);
    CHECK(settings->max_anisotropy.GetValue() == Settings::AnisotropyMode::X4);
    CHECK_FALSE(Settings::ApplyGamePreset(*settings, 0, Quality::High));
    CHECK_FALSE(
        Settings::ApplyGamePreset(*settings, Settings::MarioKartWorldProgramId + 1, Quality::Low));
    CHECK(settings->resolution_setup.GetValue() == Settings::ResolutionSetup::Res1X);
}
