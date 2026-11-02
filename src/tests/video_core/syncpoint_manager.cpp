// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include <catch2/catch_test_macros.hpp>

#include "video_core/host1x/syncpoint_manager.h"

using Tegra::Host1x::SyncpointManager;

TEST_CASE("Syncpoint ids past the table are dropped, valid ones still run their actions",
          "[video_core][syncpoint_manager]") {
    const auto manager = std::make_unique<SyncpointManager>();
    constexpr u32 Last = SyncpointManager::NUM_MAX_SYNCPOINTS - 1;
    bool fired = false;
    manager->RegisterGuestAction(Last, 1, [&fired] { fired = true; });

    // 24-bit ids as the puller's fence action can carry them.
    manager->IncrementGuest(SyncpointManager::NUM_MAX_SYNCPOINTS);
    manager->IncrementHost(0xFFFFFF);
    REQUIRE_FALSE(fired);

    manager->IncrementGuest(Last);
    REQUIRE(fired);
    REQUIRE(manager->GetGuestSyncpointValue(Last) == 1);
}
