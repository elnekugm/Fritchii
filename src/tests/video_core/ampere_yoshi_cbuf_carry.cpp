// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <catch2/catch_test_macros.hpp>
#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
namespace {
using namespace Shader::Ampere;
constexpr Instruction Exit{{0x794d, 0, 0x03800000, 0}};
constexpr Instruction Cases[] = {
    {{0x89937a10, 0x00008500, 0x027e6480, 0x000fe400}},
    {{0x59637a10, 0x00008500, 0x027e6454, 0x040fe400}},
};
TEST_CASE("Yoshi CBuf IADD3.X consumes two predicate carries", "[video_core][yoshi_cbuf_carry]") {
    for (auto word : Cases) {
        const auto result = LowerToMaxwell(std::array{word, Exit});
        INFO(result.error);
        REQUIRE(result);
    }
}
} // namespace
