// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include "common/logging.h"

TEST_CASE("DescribeException names what ends the process", "[common][logging]") {
    using Common::Log::DescribeException;
    REQUIRE(DescribeException(std::make_exception_ptr(
                std::runtime_error("out of device memory"))) == "out of device memory");
    REQUIRE(DescribeException(std::make_exception_ptr(42)) == "non-standard exception");
    REQUIRE(DescribeException(nullptr) == "no active exception");
}
