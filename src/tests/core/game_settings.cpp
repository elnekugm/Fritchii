// SPDX-FileCopyrightText: Copyright 2026 Orchid Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Per-title overrides in Core::GameSettings::LoadOverrides. Only the renderer's vendor string
// is read there, so minimal fakes stand in for the window and renderer.

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "common/settings.h"
#include "common/scope_exit.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/graphics_context.h"
#include "core/game_settings.h"
#include "video_core/renderer_base.h"

namespace {

template <typename T, bool ranged>
struct SettingSnapshot {
    Settings::SwitchableSetting<T, ranged>& setting;
    bool global;
    T global_value, custom_value;
    explicit SettingSnapshot(Settings::SwitchableSetting<T, ranged>& value)
        : setting{value}, global{value.UsingGlobal()}, global_value{value.GetValue(true)} {
        setting.SetGlobal(false);
        custom_value = setting.GetValue();
        setting.SetGlobal(global);
    }
    ~SettingSnapshot() {
        setting.SetGlobal(true);
        setting.SetValue(global_value);
        setting.SetGlobal(false);
        setting.SetValue(custom_value);
        setting.SetGlobal(global);
    }
};

class FakeWindow final : public Core::Frontend::EmuWindow {
public:
    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override {
        return nullptr;
    }
    bool IsShown() const override {
        return true;
    }
};

class FakeRenderer final : public VideoCore::RendererBase {
public:
    explicit FakeRenderer(Core::Frontend::EmuWindow& window) : RendererBase(window, nullptr) {}
    void Composite(std::span<const Tegra::FramebufferConfig>) override {}
    std::vector<u8> GetAppletCaptureBuffer() override {
        return {};
    }
    VideoCore::RasterizerInterface* ReadRasterizer() override {
        return nullptr;
    }
    std::string GetDeviceVendor() const override {
        return "AMD Radeon RX 7600 (RADV NAVI33)";
    }
};

bool PreemptiveReadsFor(u64 program_id) {
    FakeWindow window;
    FakeRenderer renderer{window};
    Core::GameSettings::LoadOverrides(program_id, renderer);
    return Settings::values.preemptive_cpu_reads;
}

} // namespace

TEST_CASE("GameSettings: Xenoblade DE Switch 2 Edition skips ordered CPU reads", "[game_settings]") {
    // Title ID as the game reports it (not the enum), so a wrong ID in the table fails here.
    // The game reads a GPU-written texture every frame; ordered reads drained the host GPU.
    REQUIRE(PreemptiveReadsFor(0x0400FF500E34A001ULL));
    // The Switch 1 release keeps ordered reads.
    REQUIRE_FALSE(PreemptiveReadsFor(0x0100FF500E34A000ULL));
}

TEST_CASE("Orbitals uses coherent native defaults while preserving custom settings", "[game_settings]") {
    auto& accuracy = Settings::values.gpu_accuracy;
    auto& fences = Settings::values.gpu_fence_behavior;
    auto& reactive = Settings::values.use_reactive_flushing;
    SettingSnapshot saved_accuracy{accuracy};
    SettingSnapshot saved_fences{fences};
    SettingSnapshot saved_reactive{reactive};
    const bool old_preemptive = Settings::values.preemptive_cpu_reads;
    const auto old_runtime_accuracy = Settings::values.current_gpu_accuracy;
    SCOPE_EXIT {
        Settings::values.preemptive_cpu_reads = old_preemptive;
        Settings::values.current_gpu_accuracy = old_runtime_accuracy;
    };
    Settings::values.current_gpu_accuracy = Settings::GpuAccuracy::High;
    accuracy.SetGlobal(true);
    accuracy.SetValue(Settings::GpuAccuracy::High);
    fences.SetGlobal(true);
    fences.SetValue(Settings::GpuFenceBehavior::Default);
    reactive.SetGlobal(true);
    reactive.SetValue(true);
    FakeWindow window;
    FakeRenderer renderer{window};

    SECTION("Inherited defaults select read traps and delayed native fences") {
        Core::GameSettings::LoadEarlyOverrides(0x040004F027560000ULL);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::Low);
        CHECK(Settings::values.current_gpu_accuracy == Settings::GpuAccuracy::Low);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Balanced);
        CHECK_FALSE(reactive.GetValue());
        Core::GameSettings::LoadOverrides(0x040004F027560000ULL, renderer);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::Low);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Balanced);
        CHECK_FALSE(reactive.GetValue());
        CHECK_FALSE(accuracy.UsingGlobal());
        CHECK_FALSE(fences.UsingGlobal());
        CHECK_FALSE(reactive.UsingGlobal());
        CHECK(accuracy.GetValue(true) == Settings::GpuAccuracy::High);
        CHECK(fences.GetValue(true) == Settings::GpuFenceBehavior::Default);
        CHECK(reactive.GetValue(true));
        CHECK_FALSE(Settings::values.preemptive_cpu_reads);
    }
    SECTION("Explicit per-game choices remain authoritative") {
        accuracy.SetGlobal(false);
        accuracy.SetValue(Settings::GpuAccuracy::High);
        fences.SetGlobal(false);
        fences.SetValue(Settings::GpuFenceBehavior::Accurate);
        reactive.SetGlobal(false);
        reactive.SetValue(true);
        Core::GameSettings::LoadEarlyOverrides(0x040004F027560000ULL);
        Core::GameSettings::LoadOverrides(0x040004F027560000ULL, renderer);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::High);
        CHECK(Settings::values.current_gpu_accuracy == Settings::GpuAccuracy::High);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Accurate);
        CHECK(reactive.GetValue());
        CHECK_FALSE(Settings::values.preemptive_cpu_reads);
    }
    SECTION("A different title retains its inherited settings") {
        Core::GameSettings::LoadEarlyOverrides(0x040004F027570000ULL);
        Core::GameSettings::LoadOverrides(0x040004F027570000ULL, renderer);
        CHECK(accuracy.GetValue() == Settings::GpuAccuracy::High);
        CHECK(fences.GetValue() == Settings::GpuFenceBehavior::Default);
        CHECK(reactive.GetValue());
        CHECK(accuracy.UsingGlobal());
        CHECK(fences.UsingGlobal());
        CHECK(reactive.UsingGlobal());
    }
}
