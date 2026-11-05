// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
#include "shader_recompiler/frontend/ampere/program_capture.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;
using Shader::Ampere::ProgramCaptureState;

constexpr Instruction EXIT{{0x0000794d, 0, 0x03800000, 0}};

// Lowered Maxwell words whose top bits (above `shift`) are `opcode`.
template <typename Program>
size_t CountMaxwell(const Program& lowered, u64 opcode, u32 shift) {
    return static_cast<size_t>(std::count_if(lowered.instructions.begin(),
                                             lowered.instructions.end(),
                                             [&](u64 word) { return (word >> shift) == opcode; }));
}

template <typename Program>
u64 FindMaxwell(const Program& lowered, u64 opcode, u32 shift) {
    const auto it = std::find_if(lowered.instructions.begin(), lowered.instructions.end(),
                                 [&](u64 word) { return (word >> shift) == opcode; });
    return it != lowered.instructions.end() ? *it : 0;
}

TEST_CASE("Ampere BRA.CONV always branches and BRA.DIV never does", "[video_core][ampere]") {
    // BRA.CONV UR6 / BRA.DIV ~URZ to the second EXIT: the mode is in the offset's low two bits.
    const std::array<Instruction, 3> convergent{{
        {{0x06007947, 0x10 | 3, 0x0b800000, 0}},
        EXIT,
        EXIT,
    }};
    ProgramCaptureState capture;
    REQUIRE(capture.Observe(convergent[0], 0) == ProgramCaptureState::Result::Continue);
    const auto taken = LowerToMaxwell(convergent);
    REQUIRE(static_cast<bool>(taken));
    REQUIRE(CountMaxwell(taken, 0xe24, 52) == 1);

    const std::array<Instruction, 3> divergent{{
        {{0xff007947, 0x10 | 2, 0x0b800000, 0}},
        EXIT,
        EXIT,
    }};
    const auto dropped = LowerToMaxwell(divergent);
    REQUIRE(static_cast<bool>(dropped));
    REQUIRE(CountMaxwell(dropped, 0xe24, 52) == 0);
}

TEST_CASE("Bananza captured BRA.U preserves its predicate and relative target",
          "[video_core][ampere]") {
    // 040074A01BF12000 fragment abb4ff47..., code offset 0x5b0.
    constexpr Instruction Nop{{0x00007918, 0, 0, 0x000fc000}};
    std::array<Instruction, 133> code;
    code.fill(Nop);
    code[91] = Instruction{{0x00008947, 0x00000281, 0x03800000, 0x000fea00}};
    code.back() = EXIT;
    const auto lowered = LowerToMaxwell(code);
    INFO(lowered.error);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions.size() == code.size());
    CHECK(lowered.instructions[91] == 0xe24000001b08000fULL);

    const auto backward = LowerToMaxwell(std::array{
        Nop, Instruction{{0x00008947, 0xffffffe1, 0x0383ffff, 0x000fea00}}, EXIT});
    REQUIRE(backward);
    CHECK(backward.instructions[1] == 0xe2400fffff08000fULL);

    const auto guarded = LowerToMaxwell(std::array{
        Instruction{{0x00008947, 0x11, 0x04800000, 0x000fea00}}, Nop, EXIT});
    REQUIRE(guarded);
    REQUIRE(guarded.instructions.size() == 4);
    CHECK(guarded.instructions[0] == 0xe24000000100000fULL);
    CHECK(guarded.instructions[1] == 0xe24000000109000fULL);
}

TEST_CASE("Ampere BRA.U rejects unsupported fields and invalid targets", "[video_core][ampere]") {
    const Instruction branch{{0x00008947, 0x1, 0x03800000, 0x000fea00}};
    for (const u32 bit : {82U, 83U, 84U, 85U, 86U, 91U, 92U, 93U, 94U, 95U,
                          96U, 97U, 98U, 99U, 100U, 101U, 102U, 103U, 104U}) {
        auto unsupported = branch;
        unsupported.words[bit / 32] |= 1U << (bit % 32);
        const auto lowered = LowerToMaxwell(std::array{unsupported, EXIT});
        INFO(bit);
        REQUIRE_FALSE(lowered);
        CHECK(lowered.error == "native branch convergence mode");
    }
    for (const u32 offset : {5U, 0x11U, 0xffffffe1U}) {
        auto invalid = branch;
        invalid.words[1] = offset;
        if (offset == 0xffffffe1U) invalid.words[2] |= 0x3ffff;
        REQUIRE_FALSE(LowerToMaxwell(std::array{invalid, EXIT}));
    }
}

TEST_CASE("Ampere TLD.SCR.LZ takes its handle from a uniform register", "[video_core][ampere]") {
    // TLD.SCR.LZ R5, R8, R4, R5, UR4, 0x0, 2D, 0x7 -> Maxwell TLD.B.
    const std::array<Instruction, 2> code{{
        {{0x04087f66, 0x30000405, 0x089e0705, 0x001f4200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    const u64 tld = FindMaxwell(lowered, 0xdd, 56);
    REQUIRE(tld != 0);
    REQUIRE(((tld >> 28) & 7) == 2); // 2D
    REQUIRE(((tld >> 31) & 0xf) == 7);
}

TEST_CASE("Ampere I2F converts signed 16-bit sources", "[video_core][ampere]") {
    // I2F.S16 R8, R8
    const std::array<Instruction, 2> code{{
        {{0x00087306, 0x00000008, 0x00101400, 0x000fe200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    const u64 i2f = FindMaxwell(lowered, 0x5cb, 52);
    REQUIRE(((i2f >> 10) & 3) == 1); // 16-bit source
    REQUIRE(((i2f >> 13) & 1) == 1); // signed
}

TEST_CASE("Ampere integer-to-float conversions take immediates", "[video_core][ampere]") {
    const std::array<Instruction, 3> code{{
        {{0x00077906, 0x00000020, 0x00209000, 0x000e2200}},
        {{0x00047845, 0x00000100, 0x0020d000, 0x000fe200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions.size() == 3);
    CHECK(lowered.instructions[0] == 0x010420000007f007ULL);
    CHECK(lowered.instructions[1] == 0x010438000007f004ULL);
    const Instruction warp_sync{{0x00007348, 0x00000007, 0x03800000, 0x000fe800}};
    const auto rejected = LowerToMaxwell(std::array{code[0], code[1], warp_sync, EXIT});
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error == "unsupported native opcode");
    CHECK(rejected.error_offset == 0x20);
}

TEST_CASE("Ampere SUST.P keeps its channel mask", "[video_core][ampere]") {
    // SUST.P.2D.STRONG.SM.R.IGN [R6], R4, 0x0, 0x59
    const std::array<Instruction, 2> code{{
        {{0x06007b9b, 0x60005904, 0x0010a100, 0x000fe200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(((FindMaxwell(lowered, 0xeb2, 52) >> 20) & 0xf) == 1);
}

TEST_CASE("Ampere SHFL with a register lane keeps its mode", "[video_core][ampere]") {
    // SHFL.DOWN PT, R7, R0, R5, 0x1f
    const std::array<Instruction, 2> code{{
        {{0x00077589, 0x08001f05, 0x000e0000, 0x000fe200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(((FindMaxwell(lowered, 0xef1, 52) >> 30) & 3) == 2);
}

TEST_CASE("Ampere calls find a return address set ahead of them", "[video_core][ampere]") {
    // MOV R9, 0x30; MOV R4, 0x5; CALL.REL.NOINC; EXIT; RET.REL.NODEC R9
    std::array<Instruction, 5> code{{
        {{0x00097802, 0x30, 0x00000f00, 0}},
        {{0x00047802, 0x5, 0x00000f00, 0}},
        {{0x00007944, 0x10, 0x03c00000, 0}},
        EXIT,
        {{0x09007950, 0xffffffb0, 0x03c3ffff, 0}},
    }};
    REQUIRE(static_cast<bool>(LowerToMaxwell(code)));

    // The link register is written again between its MOV and the call.
    code[1] = {{0x00097802, 0x5, 0x00000f00, 0}};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(!static_cast<bool>(lowered));
    REQUIRE(lowered.error == "native call linkage mode");
}

TEST_CASE("Ampere programs go on past an EXIT that a branch jumps over", "[video_core][ampere]") {
    // A call, then a branch over an EXIT to more of the program, with another call there.
    const std::array<Instruction, 8> code{{
        {{0x00047802, 0x20, 0x00000f00, 0}},
        {{0x00007944, 0x50, 0x03c00000, 0}},
        {{0x00007947, 0x10, 0x03800000, 0}},
        EXIT,
        {{0x00047802, 0x60, 0x00000f00, 0}},
        {{0x00007944, 0x10, 0x03c00000, 0}},
        EXIT,
        {{0x04007950, 0xffffff80, 0x03c3ffff, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(lowered.source_size == sizeof(code));
}

TEST_CASE("Ampere capture ends at the return of a helper called past the EXIT",
          "[video_core][ampere]") {
    // Ocarina 69a40579971fb867: MOV R0, 0x20; CALL.REL.NOINC; EXIT; RET.REL.NODEC R0, then the
    // next program in memory.
    const std::array<Instruction, 4> code{{
        {{0x00007802, 0x20, 0x00000f00, 0}},
        {{0x00007944, 0x10, 0x03c00000, 0}},
        EXIT,
        {{0x00007950, 0xffffffc0, 0x03c3ffff, 0}},
    }};
    ProgramCaptureState capture;
    for (u32 index = 0; index + 1 < code.size(); ++index)
        CHECK(capture.Observe(code[index], index) == ProgramCaptureState::Result::Continue);
    CHECK(capture.Observe(code.back(), 3) == ProgramCaptureState::Result::Complete);
    REQUIRE(static_cast<bool>(LowerToMaxwell(code)));

    ProgramCaptureState jump;
    CHECK(jump.Observe({{0x00007944, 0x10, 0x03c00000, 0}}, 0) == ProgramCaptureState::Result::Continue);
    CHECK(jump.Observe(EXIT, 1) == ProgramCaptureState::Result::Continue);
    CHECK(jump.Observe(EXIT, 2) == ProgramCaptureState::Result::Complete);
}

TEST_CASE("Ampere uniform select and bit operations reach every uniform register",
          "[video_core][ampere]") {
    // Ocarina e8decc3e5c0c184e: @UP0 USEL UR4, UR4, 0x20, UP1. UP0 and UP1 come from R239 into
    // P6 and P5; the SEL runs under P6 and the guest predicates are restored.
    const std::array<Instruction, 2> select{{
        {{0x04040887, 0x20, 0x08800000, 0x000fe200}},
        EXIT,
    }};
    const auto selected = LowerToMaxwell(select);
    INFO(selected.error);
    REQUIRE(static_cast<bool>(selected));
    const std::array<u64, 5> expected_select{
        0x010000000207f0fcULL, 0x38e800000607fffdULL, 0x38f000000607ef00ULL,
        0x5ca002800fc6dcdcULL, 0x38f000000607fd00ULL};
    CHECK(std::search(selected.instructions.begin(), selected.instructions.end(),
                      expected_select.begin(), expected_select.end()) != selected.instructions.end());

    // Ocarina 2079791ecb288b83: UBREV UR4, UR17.
    const std::array<Instruction, 2> reverse{{
        {{0x000472be, 0x11, 0x08000000, 0x000fc800}},
        EXIT,
    }};
    const auto reversed = LowerToMaxwell(reverse);
    INFO(reversed.error);
    REQUIRE(static_cast<bool>(reversed));
    CHECK(reversed.instructions.front() == 0x382801800007e9dcULL);

    // Ocarina 2079791ecb288b83: UFLO.U32.SH UR21, UR21.
    const std::array<Instruction, 2> leading{{
        {{0x001572bd, 0x15, 0x080e0400, 0x000fe400}},
        EXIT,
    }};
    const auto found = LowerToMaxwell(leading);
    INFO(found.error);
    REQUIRE(static_cast<bool>(found));
    CHECK(found.instructions.front() == 0x5c3002000ed700edULL);

    // Ocarina 2079791ecb288b83: UPOPC UR10, UR17.
    const std::array<Instruction, 2> count{{
        {{0x000a72bf, 0x11, 0x08000000, 0x000fe200}},
        EXIT,
    }};
    const auto counted = LowerToMaxwell(count);
    INFO(counted.error);
    REQUIRE(static_cast<bool>(counted));
    CHECK(counted.instructions.front() == 0x5c0800000e9700e2ULL);
}

TEST_CASE("Ampere guarded uniform comparisons keep the output when the guard is false",
          "[video_core][ampere]") {
    // Ocarina 2079791ecb288b83: @UP4 UISETP.GT.U32.AND UP0, UPT, UR21, 0x1f, UPT. UP4 and the old
    // UP0 load into P2 and P6, so a false guard merges UP0 back unchanged.
    const std::array<Instruction, 2> code{{
        {{0x1500488c, 0x1f, 0x0bf04070, 0x000fe200}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    const std::array<u64, 6> expected{
        0x0100000001f7f0fcULL, 0x38e800000447fffdULL, 0x38f000000447ef00ULL,
        0x5b6803800fc2ed37ULL, 0x38e800000407efefULL, 0x38f000000447fd00ULL};
    CHECK(std::equal(expected.begin(), expected.end(), lowered.instructions.begin()));

    // SEL R3, R4, UR7, P0
    const std::array<Instruction, 2> select{{
        {{0x04037c07, 0x7, 0x08000000, 0x000fe400}},
        EXIT,
    }};
    const auto selected = LowerToMaxwell(select);
    INFO(selected.error);
    REQUIRE(static_cast<bool>(selected));
    CHECK(selected.instructions.front() == 0x5ca000000df70403ULL);
}
} // namespace

TEST_CASE("Ocarina TLD.LZ.AOFFI lowers to TLDS 2D.LZ.AOFFI", "[video_core][ampere]") {
    // @!P0 TLD.LZ.AOFFI R14, R10 (x, y in R10, R11), R19 (offsets), 2D, red only.
    const std::array<Instruction, 2> code{{
        {{0x0a0e8b66, 0x30007c13, 0x009e11ff, 0x00014200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    const auto tlds = std::find_if(lowered.instructions.begin(), lowered.instructions.end(),
                                   [](u64 insn) { return (insn >> 58) == 0xda >> 2; });
    REQUIRE(tlds != lowered.instructions.end());
    REQUIRE(((*tlds >> 53) & 0xf) == 4);
    REQUIRE((*tlds & 0xff) == 14);
    REQUIRE(((*tlds >> 8) & 0xff) == 10);
    REQUIRE(((*tlds >> 20) & 0xff) == 19);
    auto invalid = code;
    invalid[0].words[1] &= ~(7U << 29); // 1D: TLDS has no AOFFI form for it here
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[0] = 0x0b0e8b66; // x, y not an even pair
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[2] |= 1U << 13; // bit 77 next to the offset bit
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ocarina LEA.HI with a negated Rb adds the shifted high word to -Rb",
          "[video_core][ampere]") {
    // LEA.HI R1, R1, -R0, RZ, 0x1e: -R0 + (R1 >> 2).
    const std::array<Instruction, 2> code{{
        {{0x01017211, 0x80000000, 0x078ff0ff, 0x000fca00}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(std::any_of(lowered.instructions.begin(), lowered.instructions.end(), [](u64 insn) {
        return (insn >> 52) == 0x5c1 && (insn & 0xff) == 1 && ((insn >> 8) & 0xff) == 250 &&
               ((insn >> 20) & 0xff) == 0 && ((insn >> 48) & 1) == 1;
    }));
    auto invalid = code;
    invalid[0].words[1] = 0x40000000; // bit 62: not a known Rb modifier here
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[2] = 0x078ff000; // a register high word (Rc = R0) with the negation
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ocarina RED.E on a uniform address keeps its S32 and F32 types for Maxwell",
          "[video_core][ampere]") {
    // RED.E.ADD.F32.FTZ.RN.STRONG.GPU [UR4.64], R1.
    const std::array<Instruction, 2> code{{
        {{0xff00798e, 0x00000001, 0x0c10e704, 0x000fe800}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(std::any_of(lowered.instructions.begin(), lowered.instructions.end(), [](u64 insn) {
        return (insn >> 51) == (0xebf8000000000000ULL >> 51) && (insn & 0xff) == 1 &&
               ((insn >> 20) & 7) == 3 && ((insn >> 23) & 7) == 0 && ((insn >> 48) & 1) == 1;
    }));
    auto invalid = code;
    invalid[0].words[2] = 0x0c90e704; // F32 with op 1 (MIN): Maxwell RED F32 only adds
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[2] = 0x0c10e504; // type 2 (U64) on this path is not proven
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    auto signed_minimum = code;
    signed_minimum[0].words[2] = 0x0c90e304; // RED.E.MIN.S32, as Ocarina's REDUX.MIN.S32 follow-up
    const auto lowered_minimum = LowerToMaxwell(signed_minimum);
    REQUIRE(static_cast<bool>(lowered_minimum));
    REQUIRE(std::any_of(lowered_minimum.instructions.begin(), lowered_minimum.instructions.end(),
                        [](u64 insn) {
        return (insn >> 51) == (0xebf8000000000000ULL >> 51) && ((insn >> 20) & 7) == 1 &&
               ((insn >> 23) & 7) == 1;
    }));
}

TEST_CASE("Ocarina REDUX.MIN.S32 biases lanes into an unsigned minimum", "[video_core][ampere]") {
    // REDUX.MIN.S32 UR5, R0.
    const std::array<Instruction, 2> code{{
        {{0x000573c4, 0, 0x00010200, 0x001e2400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    // LOP32I.XOR R251, R251, 0x80000000 once per lane (32 of them), plus one on the result.
    const auto biased = std::count_if(lowered.instructions.begin(), lowered.instructions.end(),
                                      [](u64 insn) {
        return (insn >> 58) == 1 && ((insn >> 53) & 3) == 2 &&
               ((insn >> 20) & 0xffffffffULL) == 0x80000000ULL;
    });
    REQUIRE(biased == 33);
    auto invalid = code;
    invalid[0].words[2] = 0x00004200; // .S32 on OR means nothing: not a known form
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ocarina TLD4.DC gathers compare against the reference in Rb", "[video_core][ampere]") {
    // TLD4.DC R12|R14, R24, R28 (x, y in R24, R25; reference in R28), shadow PCF.
    const std::array<Instruction, 2> code{{
        {{0x180c7b63, 0x30800e1c, 0x001e4f0e, 0x000f4200}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    const auto& out = lowered.instructions;
    const auto has = [&](u64 value) { return std::find(out.begin(), out.end(), value) != out.end(); };
    // MOV R251, R25 and MOV R252, R28 feed the coordinate pair and the meta register.
    REQUIRE(has(0x5c98000000070000ULL | 251 | (u64{15} << 39) | (u64{25} << 20)));
    REQUIRE(has(0x5c98000000070000ULL | 252 | (u64{15} << 39) | (u64{28} << 20)));
    REQUIRE(std::any_of(out.begin(), out.end(), [](u64 insn) {
        return (insn >> 59) == (0xc800000000000000ULL >> 59) && ((insn >> 50) & 1) == 1 &&
               ((insn >> 20) & 0xff) == 252 && ((insn >> 8) & 0xff) == 250 && ((insn >> 28) & 7) == 2;
    }));
    auto invalid = code;
    invalid[0].words[2] = 0x001e5f0e; // with AOFFI the meta register would start with offsets
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    invalid = code;
    invalid[0].words[2] = 0x001ecf0e; // .F16 packing with a compare is not proven
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
}

TEST_CASE("Ocarina bindless handles copied into the pair keep their sampler half",
          "[video_core][ampere]") {
    // ULDC.64 UR4; USHF.R UR9, UR4, 20; UMOV UR8, UR4; @P2 TEX.ARRAY_2D R43, R20, R39 with the
    // handle in UR8 (UR9 = UR8 >> 20 on every path).
    const Instruction load{{0x00047ab9, 0x00004800, 0x00000a00, 0x000fe200}};
    const Instruction shift{{0x3f097899, 0x00000014, 0x08011604, 0x000fc800}};
    const Instruction copy{{0x00087c82, 0x00000004, 0x08000000, 0x000fc400}};
    const Instruction texture{{0x142b2f60, 0xb0000827, 0x089e01ff, 0x000f4200}};
    const auto handle_moved = [](const auto& lowered) {
        // MOV R246, R224 (UR8): Maxwell TEX.B reads the handle as its first meta register.
        return std::any_of(lowered.instructions.begin(), lowered.instructions.end(), [](u64 insn) {
            return (insn & ~(u64{15} << 16)) == (0x5c98000000000000ULL | 246 | (u64{15} << 39) | (u64{224} << 20));
        });
    };
    const auto lowered = LowerToMaxwell(std::array{load, shift, copy, texture, EXIT});
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(handle_moved(lowered));
    // The copy must come from the register the shift read.
    auto other_source = copy;
    other_source.words[1] = 6;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{load, shift, other_source, texture, EXIT})));
    // UR4 reloaded between the shift and the copy: UR9 no longer matches it.
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{load, shift, load, copy, texture, EXIT})));
    // A predicated copy may not run.
    auto guarded = copy;
    guarded.words[0] = 0x00080c82;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{load, shift, guarded, texture, EXIT})));
    // Ocarina's jump tables (BRX) are taken as forward only: a UR4 reload after the texture
    // cannot reach it, so only the BRX's relative base is refused.
    const Instruction jump{{0x20007949, 0xffffbaa0, 0x0383ffff, 0x000fea00}};
    const auto table = LowerToMaxwell(std::array{load, shift, copy, texture, load, jump, EXIT});
    REQUIRE_FALSE(static_cast<bool>(table));
    REQUIRE(table.error == "native computed branch relative base");
}

TEST_CASE("Ocarina task and mesh stage buffer accesses become the nx2 pseudo instruction",
          "[video_core][ampere]") {
    // ISBERD.32.ATTR R0, [RZ]; ISBEWR.32.ATTR.SKEW.O [R13 + 0x80], R20; ISBEWR.32.O [RZ + 3], RZ.
    const std::array<Instruction, 4> code{{
        {{0xff007923, 0x00000000, 0x00003800, 0x000e2200}},
        {{0x0d007927, 0x00008014, 0x0000f800, 0x0003e200}},
        {{0xff007927, 0x000003ff, 0x00008800, 0x000fe600}},
        EXIT,
    }};
    const auto lowered = LowerToMaxwell(code);
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    constexpr u64 pseudo = 0xefc0000000070000ULL;
    REQUIRE(lowered.instructions[0] == (pseudo | (u64{255} << 8) | (u64{1} << 44) | (u64{1} << 45)));
    REQUIRE(lowered.instructions[1] == (pseudo | 20 | (u64{13} << 8) | (u64{0x80} << 20) | (u64{1} << 44) |
                                        (u64{1} << 45) | (u64{1} << 46) | (u64{1} << 47) | (u64{1} << 48)));
    REQUIRE(lowered.instructions[2] == (pseudo | 255 | (u64{255} << 8) | (u64{3} << 20) | (u64{1} << 44) |
                                        (u64{1} << 47) | (u64{1} << 48)));
    // Reads of the output buffer or the map region, region 1 and unknown bits stay rejected.
    for (const u32 mode : {0x0000b800U, 0x00000800U, 0x00001800U, 0x00013800U}) {
        auto invalid = code;
        invalid[0].words[2] = mode;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    }
    auto unread = code; // a write without .O
    unread[1].words[2] = 0x00007800;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(unread)));
}

TEST_CASE("Ocarina SR_VARIABLE_RATE reads as 1x1 shading", "[video_core][ampere]") {
    // S2R R6, SR_VARIABLE_RATE (132): MOV32I R6, 0x00010100.
    const auto lowered = LowerToMaxwell(std::array{Instruction{{0x00067919, 0, 0x00008400, 0x000e2200}}, EXIT});
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(lowered.instructions[0] == (0x010000000007f006ULL | (u64{0x00010100} << 20)));
    REQUIRE_FALSE(static_cast<bool>(
        LowerToMaxwell(std::array{Instruction{{0x00067919, 1, 0x00008400, 0x000e2200}}, EXIT})));
}

TEST_CASE("Ocarina reciprocal of a negated uniform register", "[video_core][ampere]") {
    // MUFU.RCP R30, -UR8: MOV R251, R224; LOP32I.XOR R251, R251, 0x80000000; MUFU.RCP R30, R251.
    const auto lowered = LowerToMaxwell(std::array{Instruction{{0x001e7d08, 0x80000008, 0x08001000, 0x002e2600}}, EXIT});
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(lowered.instructions[0] == (0x5c98000000070000ULL | 251 | (u64{15} << 39) | (u64{224} << 20)));
    REQUIRE(lowered.instructions[1] ==
            (0x0400000000070000ULL | 251 | (u64{251} << 8) | (u64{0x80000000} << 20) | (u64{2} << 53)));
    REQUIRE(lowered.instructions[2] == (0x5080000000070000ULL | 30 | (u64{251} << 8) | (u64{4} << 20)));
}

TEST_CASE("Ocarina SHF.R.U64 low word with an immediate shift", "[video_core][ampere]") {
    // SHF.R.U64 R14, R14, 21, R17: Maxwell SHF.R R14, R14, 21, R17 (U32 clamp, shifts up to 32).
    const Instruction shift{{0x0e0e7819, 0x00000015, 0x00001211, 0x020fe200}};
    const auto lowered = LowerToMaxwell(std::array{shift, EXIT});
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(lowered.instructions[0] ==
            (0x38f8000000070000ULL | 14 | (u64{14} << 8) | (u64{21} << 20) | (u64{17} << 39)));
    for (const auto& [amount, mode] : std::array{std::pair{33U, 0x1211U}, std::pair{21U, 0x1a11U},
                                                 std::pair{21U, 0x11211U}, std::pair{21U, 0x1311U}}) {
        auto invalid = shift; // a shift past 32, .W, .HI, an unknown bit 72
        invalid.words[1] = amount;
        invalid.words[2] = mode;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{invalid, EXIT})));
    }
}

TEST_CASE("Ocarina 64-bit right shifts keep the low word as Maxwell SHF.R", "[video_core][ampere]") {
    // SHF.R.U64 R20, R20, R43, R24 (register count): SHF.R with the U64 clamp.
    const std::array<Instruction, 2> reg{{
        {{0x09147219, 0x0000002b, 0x00001218, 0x004fc400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered = LowerToMaxwell(reg);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(std::find(lowered.instructions.begin(), lowered.instructions.end(),
                      0x5cf8000000070000ULL | 20 | (u64{9} << 8) | (u64{43} << 20) | (u64{2} << 37) |
                          (u64{24} << 39)) != lowered.instructions.end());
    auto invalid = reg;
    invalid[0].words[2] = 0x00001a18; // .W wraps the count: not this form
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(invalid)));
    // USHF.R.U64 UR10, UR6, 0x18, UR7: the uniform pair by an immediate up to 32.
    const std::array<Instruction, 2> uniform{{
        {{0x060a7899, 0x00000018, 0x08001207, 0x002fe400}},
        {{0x0000794d, 0, 0x03800000, 0}},
    }};
    const auto lowered_uniform = LowerToMaxwell(uniform);
    REQUIRE(static_cast<bool>(lowered_uniform));
    REQUIRE(std::any_of(lowered_uniform.instructions.begin(), lowered_uniform.instructions.end(),
                        [](u64 insn) {
        return (insn >> 51) == (0x38f8000000000000ULL >> 51) && ((insn >> 20) & 0x3f) == 0x18;
    }));
    auto too_far = uniform;
    too_far[0].words[1] = 33;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(too_far)));
}

TEST_CASE("Ocarina guarded uniform selects run under their UPn guard", "[video_core][ampere]") {
    // @UP4 USEL UR21, UR21, 0x20, UP0: the guard is swapped into P2, the condition through R254
    // (R253 holds the guest P2), and the SEL itself is guarded by P2.
    const Instruction select{{0x15154887, 0x00000020, 0x08000000, 0x000fc800}};
    const auto lowered = LowerToMaxwell(std::array{select, EXIT});
    INFO(lowered.error);
    REQUIRE(static_cast<bool>(lowered));
    const auto& words = lowered.instructions;
    const auto has = [&](auto match) { return std::any_of(words.begin(), words.end(), match); };
    REQUIRE(has([](u64 w) { return (w >> 52) == 0x5ca && ((w >> 16) & 0xf) == 2; }));
    REQUIRE(has([](u64 w) { return (w >> 52) == 0x38e && (w & 0xff) == 254; }));
    // Guard and condition both UP0: already in place, no second swap.
    auto same = select;
    same.words[0] = 0x15150887;
    const auto in_place = LowerToMaxwell(std::array{same, EXIT});
    REQUIRE(static_cast<bool>(in_place));
    REQUIRE(std::none_of(in_place.instructions.begin(), in_place.instructions.end(),
                         [](u64 w) { return (w >> 52) == 0x38e && (w & 0xff) == 254; }));
}
