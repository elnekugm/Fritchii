// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

TEST_CASE("Ampere F2I round up preserves conversion mode", "[video_core][ampere]") {
    using Shader::Ampere::Instruction;
    using Shader::Ampere::LowerToMaxwell;
    const Instruction exit{{0x0000794d, 0, 0x03800000, 0}};

    for (const bool is_signed : {false, true}) {
        const Instruction convert{{0x00067305, 0x00000009,
                                   is_signed ? 0x0021b100U : 0x0020b000U, 0}};
        const auto lowered = LowerToMaxwell(std::array{convert, exit});
        REQUIRE(static_cast<bool>(lowered));
        const auto instruction = std::ranges::find_if(lowered.instructions, [](u64 word) {
            return (word & 0xfff0000000000000ULL) == 0x5cb0000000000000ULL;
        });
        REQUIRE(instruction != lowered.instructions.end());
        REQUIRE((*instruction & 0xff) == 6);
        REQUIRE(((*instruction >> 8) & 3) == 2);
        REQUIRE(((*instruction >> 10) & 3) == 2);
        REQUIRE(((*instruction >> 12) & 1) == static_cast<u64>(is_signed));
        REQUIRE(((*instruction >> 39) & 3) == 2);
        REQUIRE(((*instruction >> 44) & 1) == static_cast<u64>(is_signed));
    }

    const Instruction unsupported{{0x00067305, 0x00000009, 0x0020b001, 0}};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{unsupported, exit})));
}

TEST_CASE("Ampere byte F2I rejects other widths, rounding, modifiers and register aliases",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0x00157305, 3, 0x0021e100, 0x0001e400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    for (const u32 mode : {0x0021c100U, 0x0021a100U, 0x00216100U, 0x0021e300U,
                           0x0021e900U, 0x0061e100U, 0x0821e100U}) {
        auto invalid = code;
        invalid[0].words[2] = mode;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 source : {246U, 254U, 0x10003U}) {
        auto invalid = code;
        invalid[0].words[1] = source;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 source : {28U, 62U, 64U, 216U}) {
        auto invalid = code;
        invalid[0].words = {0x00157d05, source, 0x0821e100, 0x0001e400};
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 source : {0x00400440U, 0x04800400U, 0x40400400U, 0x80400400U}) {
        auto invalid = code;
        invalid[0].words = {0x00157b05, source, 0x0021e100, 0x0001e400};
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto missing_uniform_mode = code;
    missing_uniform_mode[0].words[0] = 0x00157d05;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(missing_uniform_mode)));
    auto invalid = code;
    invalid[0].words[0] |= 1U << 24;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[3] |= 1;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[0] = 0x00f67305;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}
TEST_CASE("Ampere ceiling F2I preserves rounding, signedness and unsigned saturation",
          "[video_core][ampere]") {
    for (const bool is_signed : {false, true}) {
        for (const u32 ftz : {0U, 0x10000U}) {
            for (const u32 predicate : {2U, 7U, 10U}) {
                const std::array<Instruction, 2> code{{
                    {{0x00020305U | (predicate << 12), 2,
                      0x0020b000U | ftz | (is_signed ? 0x100U : 0U), 0x000e3000}},
                    {{0x0000794d, 0, 0x03800000, 0}},
                }};
                const auto lowered = LowerToMaxwell(code);
                REQUIRE(static_cast<bool>(lowered));
                REQUIRE(lowered.instructions.size() == (is_signed ? 2U : 9U));
                const u64 guard = u64{predicate} << 16;
                const size_t conversion = lowered.instructions.size() - (is_signed ? 2 : 3);
                REQUIRE(lowered.instructions[conversion] ==
                        (0x5cb0000000000000ULL | guard | 2 | (2ULL << 8) | (2ULL << 10) |
                         (u64{is_signed} << 12) | (u64{is_signed ? 2U : 250U} << 20) |
                         (2ULL << 39) | (u64{ftz != 0} << 44)));
                for (size_t index = 0; index + 1 < lowered.instructions.size(); ++index)
                    REQUIRE(((lowered.instructions[index] >> 16) & 15) == predicate);
                if (!is_signed) {
                    REQUIRE(lowered.instructions[2] ==
                            (0x0100000000000000ULL | guard | 252 | (15ULL << 12) |
                             (0x4f800000ULL << 20)));
                    REQUIRE(lowered.instructions[4] ==
                            (0x0100000000000000ULL | guard | 252 | (15ULL << 12) |
                             (0x4f7fffffULL << 20)));
                    REQUIRE(lowered.instructions[7] ==
                            (0x5c40000000000000ULL | guard | 2 | (2ULL << 8) | (251ULL << 20) |
                             (1ULL << 41) | (7ULL << 48)));
                }
            }
        }
    }
}

TEST_CASE("Ampere ceiling F2I rejects non-NTZ, other widths and unknown modifiers",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0x00027305, 2, 0x0020b000, 0x000e3000}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    for (const u32 mode : {0x00209000U, 0x0020a000U, 0x0020b800U, 0x0020b200U,
                           0x0060b000U, 0x0820b000U}) {
        auto invalid = code;
        invalid[0].words[2] = mode;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto invalid = code;
    invalid[0].words[1] = 0x40000002;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[3] |= 1;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ampere byte and ceiling F2I bound registers and extended instruction modes",
          "[video_core][ampere]") {
    for (const u32 mode : {0x0021e100U, 0x0020b000U}) {
        const std::array<Instruction, 2> code{{
            {{0x00027305, 2, mode, 0x000e3000}},
            {{0x0000794d, 0, 0x03800000, 0}},
        }};
        for (const u32 source : {0U, 245U, 255U}) {
            auto valid = code;
            valid[0].words[1] = source;
            REQUIRE(static_cast<bool>(LowerToMaxwell(valid)));
        }
        for (const u32 bit : {0U, 8U, 30U, 31U}) {
            auto invalid = code;
            invalid[0].words[3] ^= 1U << bit;
            REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
        }
        for (u32 bit = 9; bit < 30; ++bit) {
            auto scheduling = code;
            scheduling[0].words[3] ^= 1U << bit;
            REQUIRE(static_cast<bool>(LowerToMaxwell(scheduling)));
        }
    }
}
TEST_CASE("Ampere indexed constants preserve narrow widths and signed byte offsets",
          "[video_core][ampere]") {
    for (u32 width = 0; width <= 5; ++width) {
        for (const u32 mode : {0U, 2U}) {
            for (const u32 offset : {0U, 1U, 0x20U, 0x8000U, 0xffffU}) {
                for (const u32 source : {3U, 255U}) {
                    const std::array<Instruction, 2> code{{
                        {{0x00049b82U | (source << 24), (15U << 22) | (offset << 6),
                          (width << 9) | (mode << 14), 0x000e6200}},
                        {{0x0000794d, 0, 0x03800000, 0}},
                    }};
                    const auto lowered = LowerToMaxwell(code);
                    REQUIRE(static_cast<bool>(lowered));
                    REQUIRE(lowered.instructions.size() == 2);
                    REQUIRE(lowered.instructions.front() ==
                            (0xef90000000090000ULL | 4 | (u64{source} << 8) |
                             (u64{offset} << 20) | (15ULL << 36) | (u64{mode} << 44) |
                             (u64{width} << 48)));
                }
            }
        }
    }
}

TEST_CASE("Ampere indexed constants reject unsupported widths modes and operand fields",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0xff059b82, 0x03c00800, 0, 0x000e6200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    REQUIRE(static_cast<bool>(LowerToMaxwell(code)));
    for (const u32 word : {0x0c00U, 0x0e00U, 0x4000U, 0xc000U, 0xa00U}) {
        auto invalid = code;
        invalid[0].words[2] = word;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 bit : {32U, 37U, 59U, 63U, 64U, 72U, 76U, 80U, 91U}) {
        auto invalid = code;
        invalid[0].words[bit / 32] ^= 1U << (bit % 32);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto invalid = code;
    invalid[0].words[1] = (18U << 22) | (0x20U << 6);
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[0] = 0xf6059b82;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ampere uniform votes use the complete mapped uniform register bank",
          "[video_core][ampere]") {
    for (u32 target = 0; target <= 63; ++target) {
        if (target >= 28 && target != 63) continue;
        for (u32 output = 0; output < 8; ++output) {
            for (u32 mode = 0; mode < 3; ++mode) {
                const std::array<Instruction, 2> code{{
                    {{0x00007886U | (target << 16), 0,
                      0x03800000U | (output << 17) | (mode << 8), 0x000fe400}},
                    {{0x0000794d, 0, 0x03800000, 0}},
                }};
                const auto lowered = LowerToMaxwell(code);
                REQUIRE(static_cast<bool>(lowered));
                const u32 mapped = target < 22 ? target + 216 : target < 28 ? target + 218 : 255;
                const u32 temporary = output == 7 ? 7 : 6 - output;
                const u32 mask = 1U << temporary;
                REQUIRE(lowered.instructions.size() == (output == 7 ? 2U : 5U));
                REQUIRE(lowered.instructions[output == 7 ? 0 : 1] ==
                        (0x50d8000000070000ULL | mapped | (7ULL << 39) |
                         (u64{temporary} << 45) | (u64{mode} << 48)));
                if (output != 7) {
                    REQUIRE(lowered.instructions[0] ==
                            (0x38e8000000070000ULL | 253 | (255ULL << 8) | (u64{mask} << 20)));
                    REQUIRE(lowered.instructions[2] ==
                            (0x38e8000000070000ULL | 239 | (239ULL << 8) | (u64{mask} << 20)));
                    REQUIRE(lowered.instructions[3] ==
                            (0x38f0000000070000ULL | (253ULL << 8) | (u64{mask} << 20)));
                }
            }
        }
    }
}

TEST_CASE("Ampere uniform votes reject unmapped registers and unsupported fields",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0x00107886, 0, 0x038e0100, 0x000fe400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    for (const u32 target : {28U, 62U, 64U}) {
        auto invalid = code;
        invalid[0].words[0] = 0x00007886U | (target << 16);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 bit : {12U, 24U, 32U, 64U, 73U, 74U, 80U, 84U, 91U}) {
        auto invalid = code;
        invalid[0].words[bit / 32] ^= 1U << (bit % 32);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
}

TEST_CASE("Ampere half register comparisons preserve captured swizzles and predicates",
          "[video_core][ampere]") {
    for (const u32 source : {4U, 8U, 12U, 40U, 52U}) {
        for (u32 output = 0; output < 4; ++output) {
            for (const u32 guard : {1U, 7U, 9U}) {
                const std::array<Instruction, 2> code{{
                    {{0x00000234U | (guard << 12) | (source << 24), 0x30000000U | source,
                      0x00702800U | (output << 17) | (output << 23), 0x000fe400}},
                    {{0x0000794d, 0, 0x03800000, 0}},
                }};
                const auto lowered = LowerToMaxwell(code);
                REQUIRE(static_cast<bool>(lowered));
                REQUIRE(lowered.instructions.size() == 2);
                REQUIRE(lowered.instructions.front() ==
                        (0x5d20000000000000ULL | (u64{guard} << 16) | 7 |
                         (u64{output} << 3) | (u64{source} << 8) | (u64{source} << 20) |
                         (3ULL << 28) | (2ULL << 35) | (u64{output} << 39) | (2ULL << 47)));
            }
        }
    }
}

TEST_CASE("Ampere half register comparisons map every semantic modifier bit",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0x28007234, 0x30000028, 0x00702800, 0x000fe400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto baseline = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(baseline));
    const std::array<std::array<u32, 2>, 26> fields{{
        {60, 28}, {61, 29}, {62, 30}, {63, 31}, {69, 45}, {70, 46}, {71, 49},
        {72, 43}, {73, 44}, {74, 47}, {75, 48}, {76, 35}, {77, 36}, {78, 37},
        {79, 38}, {80, 6}, {81, 3}, {82, 4}, {83, 5}, {84, 0}, {85, 1},
        {86, 2}, {87, 39}, {88, 40}, {89, 41}, {90, 42},
    }};
    for (const auto& field : fields) {
        auto modified = code;
        modified[0].words[field[0] / 32] ^= 1U << (field[0] % 32);
        const auto lowered = LowerToMaxwell(modified);
        REQUIRE(static_cast<bool>(lowered));
        REQUIRE(lowered.instructions.front() == (baseline.instructions.front() ^ (1ULL << field[1])));
    }
    for (const u32 bit : {16U, 32U + 8U, 59U, 64U, 68U, 91U}) {
        auto invalid = code;
        invalid[0].words[bit / 32] ^= 1U << (bit % 32);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto invalid = code;
    invalid[0].words[2] |= 3U << 5;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[0] = 0xf6007234;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[1] = 0x300000f6;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}
TEST_CASE("Ampere formatted buffer stores preserve descriptors data and predicates",
          "[video_core][ampere]") {
    for (const u32 guard : {1U, 7U, 9U}) {
        for (const u32 dimension : {1U, 3U, 4U, 5U}) {
            const std::array<Instruction, 2> code{{
                {{0x00000b9bU | (guard << 12), 0x00c00004U | (dimension << 29),
                  0x00100f00, 0x000fe200}},
                {{0x0000794d, 0, 0x03800000, 0}},
            }};
            const auto lowered = LowerToMaxwell(code);
            REQUIRE(static_cast<bool>(lowered));
            REQUIRE(lowered.required_surface_buffer == 3);
            REQUIRE(lowered.required_texture_buffer == ~0U);
            REQUIRE(lowered.instructions.size() == 2);
            REQUIRE(lowered.instructions.front() ==
                    (0xeb20000000000000ULL | (u64{guard} << 16) | 4 | (15ULL << 20) |
                     (u64{dimension} << 33) | (1ULL << 51)));
        }
    }
}

TEST_CASE("Ampere formatted buffer stores retain strict mode and operand bounds",
          "[video_core][ampere]") {
    const std::array<Instruction, 2> code{{
        {{0x00007b9b, 0x20c00004, 0x00100f00, 0x000fe200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    for (const u32 dimension : {0U, 2U, 6U, 7U}) {
        auto invalid = code;
        invalid[0].words[1] = 0x00c00004U | (dimension << 29);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    for (const u32 bit : {59U, 60U, 72U, 80U, 84U, 91U}) {
        auto invalid = code;
        invalid[0].words[bit / 32] ^= 1U << (bit % 32);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto invalid = code;
    invalid[0].words[1] = 0x20c000fd;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[1] = 0x24800004;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[1] |= 0x2000U << 8;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ampere per-tap gathers preserve XY offsets split destinations and predicates",
          "[video_core][ampere]") {
    for (const u32 guard : {1U, 7U, 9U}) {
        for (u32 component = 0; component < 4; ++component) {
            for (const u32 packed : {0xff00ff00U, 0x1ff0002U, 0x1fe01fe0U}) {
                const std::array<Instruction, 4> code{{
                    {{0x00087802, packed, 0x00000f00, 0x000fe400}},
                    {{0x00097802, 0x01000001, 0x00000f00, 0x000fe400}},
                    {{0x040a0b63U | (guard << 12), 0x33800408,
                      0x041e2f08U | (component << 23), 0x100fc200}},
                    {{0x0000794d, 0, 0x03800000, 0}},
                }};
                const auto lowered = LowerToMaxwell(code);
                REQUIRE(static_cast<bool>(lowered));
                REQUIRE(lowered.required_texture_buffer == 14);
                const u64 pred = u64{guard} << 16;
                const auto copy = [pred](u32 target, u32 source) {
                    return 0x5c98000000000000ULL | pred | target | (15ULL << 39) |
                           (u64{source} << 20);
                };
                const std::array<u64, 9> expected{
                    0x010000000007f000ULL | 248 | (u64{packed} << 20),
                    0x010000000007f000ULL | 249 | (0x01000001ULL << 20),
                    copy(250, 4), copy(251, 5),
                    0xc800000000000000ULL | pred | 250 | (250ULL << 8) | (248ULL << 20) |
                        (2ULL << 28) | (15ULL << 31) | (4ULL << 36) | (1ULL << 49) |
                        (7ULL << 51) | (2ULL << 54) | (u64{component} << 56),
                    copy(10, 250), copy(11, 251), copy(8, 252), copy(9, 253),
                };
                const auto begin = lowered.instructions.begin() + lowered.source_instruction_offsets[2];
                REQUIRE(std::equal(expected.begin(), expected.end(), begin));
                REQUIRE(begin + expected.size() + 1 == lowered.instructions.end());
            }
        }
    }
}

TEST_CASE("Ampere per-tap gathers prove constants across unrelated arithmetic and gathers",
          "[video_core][ampere]") {
    const std::array<Instruction, 6> code{{
        {{0x00067802, 0x01ff0002, 0x00000f00, 0x000fe400}},
        {{0x00077802, 0x02020200, 0x00000f00, 0x000fe400}},
        {{0x00057421, 0xbf000000, 0x00010000, 0x002fc800}},
        {{0x04037b63, 0x33800405, 0x041e02ff, 0x000f4200}},
        {{0x04007b63, 0x33800406, 0x041e2f06, 0x000f4400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    REQUIRE(static_cast<bool>(LowerToMaxwell(code)));
    auto clobber = code;
    clobber[2].words[0] = 0x00067421;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(clobber)));
    clobber = code;
    clobber[3].words[0] = 0x04067b63;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(clobber)));
    clobber = code;
    clobber[3].words[2] = 0x041e0f06;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(clobber)));
    auto branch = code;
    branch[2].words = {0x00007947, 0, 0x03800000, 0};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(branch)));
}

TEST_CASE("Ampere per-tap gathers reject unknown offsets precision and register aliases",
          "[video_core][ampere]") {
    const std::array<Instruction, 4> code{{
        {{0x00087802, 0xff00ff00, 0x00000f00, 0x000fe400}},
        {{0x00097802, 0x01000001, 0x00000f00, 0x000fe400}},
        {{0x040a7b63, 0x33800408, 0x041e2f08, 0x100fc200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    for (const u32 value : {32U, 127U, 128U, 223U}) {
        for (u32 shift = 0; shift < 32; shift += 8) {
            auto invalid = code;
            invalid[0].words[1] = (0xff00ff00U & ~(255U << shift)) | (value << shift);
            REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
        }
    }
    for (const u32 bit : {60U, 72U, 76U, 78U, 79U, 80U, 81U, 89U, 91U}) {
        auto invalid = code;
        invalid[2].words[bit / 32] ^= 1U << (bit % 32);
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto invalid = code;
    invalid[0].words[0] = 0x00081802;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[1].words[0] = 0x000a7802;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    for (const u32 target : {0xf50a7b63U, 0x04f57b63U}) {
        invalid = code;
        invalid[2].words[0] = target;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    invalid = code;
    invalid[2].words[1] = 0x338004f5;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[2].words[2] = 0x041e2ff5;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    const Instruction branch{{0x00007947, 0, 0x03800000, 0}};
    const std::array<Instruction, 5> prefix_branch{branch, code[0], code[1], code[2], code[3]};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(prefix_branch)));
    const std::array<Instruction, 5> suffix_branch{code[0], code[1], code[2], branch, code[3]};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(suffix_branch)));
}
TEST_CASE("Ampere mixed texture bindings materialize only the secondary descriptor handle",
          "[video_core][ampere]") {
    const std::array<Instruction, 3> code{{
        {{0x06027b60, 0x71001824, 0x049e0709, 0x000fc200}},
        {{0x00007b60, 0x32c0000e, 0x051e070e, 0x000f4200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    const auto conversion = std::ranges::find_if(lowered.instructions, [](u64 word) {
        return (word >> 48) == 0x5cb0;
    });
    REQUIRE(conversion != lowered.instructions.end());
    const u64 f2i = *conversion;
    REQUIRE((f2i & 0xff) == 9);
    REQUIRE(((f2i >> 8) & 3) == 2);
    REQUIRE(((f2i >> 10) & 3) == 2);
    REQUIRE(((f2i >> 12) & 1) == 1);
    REQUIRE(((f2i >> 39) & 3) == 3);
    REQUIRE(((f2i >> 44) & 1) == 1);

}
