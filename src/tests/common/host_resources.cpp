// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "common/common_types.h"
#include "common/literals.h"
#include "common/memory_detect.h"

using namespace Common::Literals;

TEST_CASE("DescribeHostResources names what a host can run out of", "[common][memory]") {
    using Common::DescribeHostResources;
    // A platform that reports only the process leaves the host and handle parts out.
    REQUIRE(DescribeHostResources({.process_resident = 5_GiB}) == "process 5120 MiB resident");
    REQUIRE(DescribeHostResources({.process_resident = 9_GiB,
                                   .process_commit = 18_GiB,
                                   .host_commit_free = 100_MiB,
                                   .host_commit_limit = 37_GiB,
                                   .host_physical_free = 512_MiB,
                                   .gdi_objects = 9998,
                                   .user_objects = 312,
                                   .handles = 2301}) ==
            "process 9216 MiB resident, 18432 MiB committed; host 100 of 37888 MiB commit free, "
            "512 MiB RAM free; 9998 GDI objects, 312 USER objects, 2301 handles");
}

#if defined(__linux__) || defined(_WIN32)
TEST_CASE("QueryHostResources follows the memory the process takes", "[common][memory]") {
    const Common::HostResources before = Common::QueryHostResources();
    REQUIRE(before.process_resident > 1_MiB);

    const std::vector<u8> block(64_MiB, u8{1}); // written, so it is resident too
    const Common::HostResources after = Common::QueryHostResources();
    REQUIRE(block.back() == 1);
    REQUIRE(after.process_resident >= before.process_resident + 48_MiB);
    REQUIRE(after.process_resident <= before.process_resident + 256_MiB);
#ifdef _WIN32
    REQUIRE(after.process_commit >= before.process_commit + 48_MiB);
    REQUIRE(after.process_commit <= before.process_commit + 256_MiB);
    REQUIRE(after.host_commit_limit > 1_GiB);
    REQUIRE(after.host_commit_free < after.host_commit_limit);
    REQUIRE(after.host_physical_free > 0);
    REQUIRE(after.handles > 0);
#endif
}
#endif
