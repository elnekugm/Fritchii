// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "hid_core/resources/npad/npad.h"

using Core::HID::NpadStyleSet;
using Core::HID::NpadStyleTag;
using Service::HID::SyncJoycon2DualStyle;

namespace {
NpadStyleTag Dual() {
    NpadStyleTag style{NpadStyleSet::None};
    style.joycon_dual.Assign(1);
    return style;
}
constexpr auto Joycon2 = static_cast<NpadStyleSet>(0x1000);
} // namespace

TEST_CASE("Joy-Con 2 bit is hidden from games that do not support it", "[hid]") {
    // Kirby Air Riders: 0x3f, aborts in GetVibrationDeviceHandles when bit 12 shows up.
    auto style = Dual();
    style.joycon2_dual.Assign(1);
    REQUIRE(SyncJoycon2DualStyle(style, static_cast<NpadStyleSet>(0x3f)));
    REQUIRE(style.raw == Dual().raw);
    REQUIRE_FALSE(SyncJoycon2DualStyle(style, static_cast<NpadStyleSet>(0x3f)));
}

TEST_CASE("Joy-Con 2 bit is shown to games that support it", "[hid]") {
    // Drag x Drive narrows its supported set to 0x1000.
    auto style = Dual();
    REQUIRE(SyncJoycon2DualStyle(style, Joycon2));
    REQUIRE(style.joycon_dual == 1);
    REQUIRE(style.joycon2_dual == 1);
    REQUIRE_FALSE(SyncJoycon2DualStyle(style, Joycon2));
}

TEST_CASE("Joy-Con 2 bit never lands on a non-dual pad", "[hid]") {
    NpadStyleTag style{NpadStyleSet::None};
    style.fullkey.Assign(1);
    const auto before = style.raw;
    REQUIRE_FALSE(SyncJoycon2DualStyle(style, Joycon2));
    REQUIRE(style.raw == before);
}
