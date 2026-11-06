// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
#include "shader_recompiler/frontend/maxwell/decode.h"

namespace {
using Shader::Ampere::Instruction;
constexpr Instruction Exit{{0x0000794d, 0, 0x03800000, 0}};
using Warp = std::array<std::array<u32, 256>, 32>;

// Distinct shared-memory contents expose both the wrong row and the wrong column.
u32 SharedWord(u32 address) {
    REQUIRE(address % 4 == 0);
    return 0x91234567U ^ (address * 0x01010101U);
}

// Execute the scalar/register, indexed shuffle and shared-read instructions produced by LDSM.
// Shuffle reads use a snapshot of the entire warp, including when destination aliases source.
Warp ExecuteMatrixLoad(const std::vector<u64>& instructions, u32 address_register) {
    Warp registers{};
    for (u32 lane = 0; lane < 32; ++lane) {
        registers[lane][address_register] = 64U * lane;
    }
    u32 shuffles{}, loads{};
    for (const u64 word : instructions) {
        const auto opcode = Shader::Maxwell::Decode(word);
        using Opcode = Shader::Maxwell::Opcode;
        const u32 dest = word & 255;
        const u32 a = (word >> 8) & 255;
        const u32 b = (word >> 20) & 255;
        const u32 immediate = (word >> 20) & 0x7ffff;
        const Warp before = registers;
        if (opcode == Opcode::SHFL) ++shuffles;
        if (opcode == Opcode::LDS) ++loads;
        for (u32 lane = 0; lane < 32; ++lane) {
            const auto read = [&](u32 reg) { return reg == 255 ? 0U : before[lane][reg]; };
            switch (opcode) {
            case Opcode::S2R: registers[lane][dest] = lane; break;
            case Opcode::LOP32I: registers[lane][dest] = read(a) & immediate; break;
            case Opcode::SHR_imm: registers[lane][dest] = read(a) >> immediate; break;
            case Opcode::SHL_imm: registers[lane][dest] = read(a) << immediate; break;
            case Opcode::IADD32I: registers[lane][dest] = read(a) + immediate; break;
            case Opcode::IADD_reg: registers[lane][dest] = read(a) + read(b); break;
            case Opcode::SHFL:
                REQUIRE(read(b) < 32);
                registers[lane][dest] = before[read(b)][a];
                break;
            case Opcode::LDS:
                registers[lane][dest] = SharedWord(read(a) + immediate);
                break;
            default: break; // Function exit and lowering scaffolding do not touch these values.
            }
        }
    }
    REQUIRE(shuffles == 4);
    REQUIRE(loads == 4);
    return registers;
}
} // namespace

TEST_CASE("Ampere four-matrix LDSM preserves aliased row addresses", "[video_core][ampere]") {
    // Actual Yoshi captures: R28..31 <- [R28] and R72..75 <- [R74 + 0x8000].
    for (const Instruction instruction : {
             Instruction{{0x1c1c783b, 0, 0x200, 0x000fe800}},
             Instruction{{0x4a48783b, 0x00800000, 0x200, 0x000fe800}},
             Instruction{{0x141c783b, 0, 0x200, 0}}, // Non-overlapping R20 source.
         }) {
        const std::array code{instruction, Exit};
        const auto lowered = Shader::Ampere::LowerToMaxwell(code);
        REQUIRE(static_cast<bool>(lowered));
        const u32 source = instruction.Bits(24, 8);
        const u32 dest = instruction.Bits(16, 8);
        const u32 offset = instruction.Bits(40, 24);
        const Warp registers = ExecuteMatrixLoad(lowered.instructions, source);
        for (u32 matrix = 0; matrix < 4; ++matrix) {
            for (u32 lane = 0; lane < 32; ++lane) {
                // Each eight-lane source group supplies eight contiguous 16-byte rows.
                const u32 row = matrix * 8 + lane / 4;
                const u32 column_byte = (lane % 4) * 4;
                CHECK(registers[lane][dest + matrix] ==
                      SharedWord(64U * row + column_byte + offset));
            }
        }
    }
}

TEST_CASE("Ampere LDSM rejects output spans using lowering scratch registers",
          "[video_core][ampere]") {
    const std::array valid{Instruction{{0x14f2783b, 0, 0x200, 0}}, Exit};
    REQUIRE(static_cast<bool>(Shader::Ampere::LowerToMaxwell(valid))); // R242..245
    auto invalid = valid;
    invalid[0].words[0] += 1U << 16; // R243..246 overlaps reserved scratch.
    REQUIRE_FALSE(static_cast<bool>(Shader::Ampere::LowerToMaxwell(invalid)));
}
