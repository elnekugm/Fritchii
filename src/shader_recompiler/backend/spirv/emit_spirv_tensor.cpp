// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>

#include "common/common_types.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "common/common_types.h"

namespace Shader::Backend::SPIRV {
namespace {

// HMMA.16816.F16 fragment layout (PTX m16n8k16), lane t = 4g + i:
//   A (16x16 row-major): a0={A[g][2i],A[g][2i+1]} a1={A[g+8][2i],..}
//                        a2={A[g][8+2i],..}       a3={A[g+8][8+2i],..}
//   B (16x8  row-major): b0={B[2i][g],B[2i+1][g]}  b1={B[8+2i][g],B[8+2i+1][g]}
//   C/D (16x8 row-major): d0={D[g][2i],D[g][2i+1]} d1={D[g+8][2i],D[g+8][2i+1]}
// The fragments are staged through a Workgroup f16 scratch and consumed with the
// NV cooperative-matrix instructions.
constexpr u32 A_BASE = 0;   // 16x16 f16
constexpr u32 B_BASE = 256; // 16x8 f16
constexpr u32 C_BASE = 384; // 16x8 f16
constexpr u32 D_BASE = 512; // 16x8 f16

} // Anonymous namespace

Id EmitTensorMulAdd(EmitContext& ctx, IR::Inst* inst, Id a0, Id a1, Id a2, Id a3, Id b0, Id b1,
                    Id c0, Id c1) {
    const Id u32ty{ctx.U32[1]};
    const Id f16{ctx.F16[1]};
    const Id f16x2{ctx.F16[2]};

    const auto add = [&](Id x, Id y) { return ctx.OpIAdd(u32ty, x, y); };
    const auto shl = [&](Id x, Id y) { return ctx.OpShiftLeftLogical(u32ty, x, y); };
    const auto shr = [&](Id x, Id y) { return ctx.OpShiftRightLogical(u32ty, x, y); };
    const auto and_ = [&](Id x, Id y) { return ctx.OpBitwiseAnd(u32ty, x, y); };
    const auto imm = [&](u32 v) { return ctx.Const(v); };

    const auto ptr = [&](Id index) {
        return ctx.uses_explicit_workgroup_layout
                   ? ctx.OpAccessChain(ctx.coop_scratch_f16, ctx.coop_scratch, imm(0U), index)
                   : ctx.OpAccessChain(ctx.coop_scratch_f16, ctx.coop_scratch, index);
    };
    const auto lo_f16 = [&](Id value) {
        return ctx.OpCompositeExtract(f16, ctx.OpBitcast(f16x2, value), 0U);
    };
    const auto hi_f16 = [&](Id value) {
        return ctx.OpCompositeExtract(f16, ctx.OpBitcast(f16x2, value), 1U);
    };
    const auto store = [&](Id index, Id value_f16) { ctx.OpStore(ptr(index), value_f16); };
    const auto load = [&](Id index) { return ctx.OpLoad(f16, ptr(index)); };
    const auto combine = [&](Id lo, Id hi) {
        return ctx.OpBitcast(u32ty, ctx.OpCompositeConstruct(f16x2, lo, hi));
    };
    const auto barrier = [&]() { ctx.OpControlBarrier(imm(3U), imm(3U), imm(0x108U)); };

    const Id lane{ctx.OpLoad(u32ty, ctx.subgroup_local_invocation_id)};
    const Id sub_base{ctx.OpIMul(u32ty, ctx.OpLoad(u32ty, ctx.subgroup_id), imm(640U))};
    const Id abase{add(imm(A_BASE), sub_base)};
    const Id bbase{add(imm(B_BASE), sub_base)};
    const Id cbase{add(imm(C_BASE), sub_base)};
    const Id dbase{add(imm(D_BASE), sub_base)};
    const Id g{shr(lane, imm(2U))};                // lane / 4
    const Id i{and_(lane, imm(3U))};               // lane % 4
    const Id i2{shl(i, imm(1U))};                  // 2i
    const Id i4_2{shl(add(i, imm(4U)), imm(1U))};  // 8+2i
    const Id g16{shl(g, imm(4U))};                 // g*16
    const Id g8_16{shl(add(g, imm(8U)), imm(4U))}; // (g+8)*16
    const Id g3{shl(g, imm(3U))};                  // g*8
    const Id g8_3{shl(add(g, imm(8U)), imm(3U))};  // (g+8)*8

    barrier();
    // Stage A (row-major, 16x16).
    {
        const Id idx0{add(abase, add(g16, i2))};
        const Id idx1{add(abase, add(g8_16, i2))};
        const Id idx2{add(abase, add(g16, i4_2))};
        const Id idx3{add(abase, add(g8_16, i4_2))};
        store(idx0, lo_f16(a0));
        store(add(idx0, imm(1U)), hi_f16(a0));
        store(idx1, lo_f16(a1));
        store(add(idx1, imm(1U)), hi_f16(a1));
        store(idx2, lo_f16(a2));
        store(add(idx2, imm(1U)), hi_f16(a2));
        store(idx3, lo_f16(a3));
        store(add(idx3, imm(1U)), hi_f16(a3));
    }
    // Stage B (row-major, 16x8): b0={B[2i][g],B[2i+1][g]}, b1={B[8+2i][g],B[8+2i+1][g]}.
    {
        const Id idx0{add(bbase, add(shl(i, imm(4U)), g))};               // (2i)*8+g
        const Id idx1{add(bbase, add(shl(add(i, imm(4U)), imm(4U)), g))}; // (8+2i)*8+g
        store(idx0, lo_f16(b0));
        store(add(idx0, imm(8U)), hi_f16(b0));
        store(idx1, lo_f16(b1));
        store(add(idx1, imm(8U)), hi_f16(b1));
    }
    if (!has_hmma) {
        return std::nullopt;
    }
    const u32 invocations = program.workgroup_size[0] * program.workgroup_size[1] *
                            program.workgroup_size[2];
    const u32 subgroups = invocations / GUEST_WARP_LANES;
    const u32 scratch_offset = (program.shared_memory_size + 15U) & ~15U;
    const u32 total = scratch_offset + subgroups * 384U * 4U;
    // Past the device limit the pipeline would fail to build: keep the emulation.
    if (profile.max_compute_shared_memory_size != 0 &&
        total > profile.max_compute_shared_memory_size) {
        return std::nullopt;
    }
    program.shared_memory_size = total;
    return scratch_offset / 4U;
}

void DefineSoftwareMatrixFunctions(EmitContext& ctx, const IR::Program& program) {
    if (ctx.tensor_scratch_word) {
        const u32 rows = ctx.profile.support_cooperative_hmma_16x16x16 ? 16 : 8;
        ctx.cooperative_hmma =
            DefineCooperativeHmmaTypes(ctx, ctx.U32[1], rows, 16, ctx.profile.supported_spirv);
        ctx.AddCapability(spv::Capability::GroupNonUniform);
        const Id input_u32 = ctx.TypePointer(spv::StorageClass::Input, ctx.U32[1]);
        ctx.tensor_subgroup_id = ctx.AddGlobalVariable(input_u32, spv::StorageClass::Input);
        ctx.Decorate(ctx.tensor_subgroup_id, spv::Decoration::BuiltIn, spv::BuiltIn::SubgroupId);
        ctx.interfaces.push_back(ctx.tensor_subgroup_id);
        return;
    }
    const bool cooperative{ctx.profile.support_cooperative_hmma && SupportsCooperativeHmma(program)};
    std::array<bool, 2> needed{};
    for (const auto* block : program.blocks) {
        for (const auto& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::TensorMulAdd || inst.Flags<u32>() == 0) continue;
            const u32 depth = inst.Flags<u32>();
            if (depth != 8 && depth != 16) throw LogicError("Invalid software HMMA depth");
            if (depth == 16 && cooperative) {
                if (!ctx.cooperative_hmma) {
                    ctx.cooperative_hmma = DefineCooperativeHmmaTypes(
                        ctx, ctx.U32[1], 16, 8, ctx.profile.supported_spirv);
                }
                continue;
            }
            needed[depth == 16] = true;
        }
    }
    for (u32 variant = 0; variant < 2; ++variant) {
        if (!needed[variant]) continue;
        ctx.AddCapability(spv::Capability::GroupNonUniform);
        ctx.AddCapability(spv::Capability::GroupNonUniformShuffle);
        if (!ctx.SupportsSubgroups() ||
            ctx.profile.has_broken_spirv_subgroup_shuffle)
            throw NotImplementedException("Software HMMA requires subgroup shuffle support");
        const Id u32ty = ctx.U32[1], f32ty = ctx.F32[1];
        const auto imm = [&](u32 v) { return ctx.Const(v); };
        const auto add = [&](Id a, Id b) { return ctx.OpIAdd(u32ty, a, b); };
        const auto and_ = [&](Id a, u32 b) { return ctx.OpBitwiseAnd(u32ty, a, imm(b)); };
        const auto unpack = [&](Id v) { return ctx.OpUnpackHalf2x16(ctx.F32[2], v); };
        const auto round_half = [&](Id value) {
            // Do not express the intermediate rounding as unpack(pack(value)): drivers may
            // eliminate that round trip. Round normal significands explicitly, ties to even;
            // half subnormals lie on the exactly representable 2^-24 grid.
            const Id bits = ctx.OpBitcast(u32ty, value);
            const Id magnitude = and_(bits, 0x7fffffffU);
            const Id tie = and_(ctx.OpShiftRightLogical(u32ty, magnitude, imm(13)), 1);
            const Id rounded = and_(add(add(magnitude, imm(0xfff)), tie), ~0x1fffU);
            const Id normal = ctx.OpSelect(u32ty,
                ctx.OpUGreaterThanEqual(ctx.U1, rounded, imm(0x47800000)), imm(0x7f800000), rounded);
            const Id scaled = ctx.OpFMul(f32ty, ctx.OpBitcast(f32ty, magnitude), ctx.Const(16777216.0f));
            const Id subnormal = ctx.OpBitcast(u32ty,
                ctx.OpFMul(f32ty, ctx.OpRoundEven(f32ty, scaled), ctx.Const(0x1p-24f)));
            const Id finite = ctx.OpSelect(u32ty,
                ctx.OpUGreaterThanEqual(ctx.U1, magnitude, imm(0x38800000)), normal, subnormal);
            const Id result = ctx.OpSelect(u32ty,
                ctx.OpUGreaterThanEqual(ctx.U1, magnitude, imm(0x7f800000)), magnitude, finite);
            return ctx.OpBitcast(f32ty, ctx.OpBitwiseOr(u32ty, result, and_(bits, 0x80000000)));
        };
        const Id type = ctx.TypeFunction(ctx.U32[2], ctx.U32[4], ctx.U32[2], ctx.U32[2]);
        const Id fn = ctx.OpFunction(ctx.U32[2], spv::FunctionControlMask::DontInline, type);
        ctx.software_matrix_functions[variant] = fn;
        const Id matrix_a = ctx.OpFunctionParameter(ctx.U32[4]);
        const Id matrix_b = ctx.OpFunctionParameter(ctx.U32[2]);
        const Id matrix_c = ctx.OpFunctionParameter(ctx.U32[2]);
        ctx.AddLabel();
        const Id counter = ctx.AddLocalVariable(ctx.TypePointer(spv::StorageClass::Function, u32ty),
                                           spv::StorageClass::Function);
        std::array<Id, 4> accumulators;
        for (auto& acc : accumulators)
            acc = ctx.AddLocalVariable(ctx.TypePointer(spv::StorageClass::Function, f32ty),
                                 spv::StorageClass::Function);
        std::array<Id, 8> inputs;
        for (u32 i = 0; i < 4; ++i)
            inputs[i] = ctx.OpCompositeExtract(u32ty, matrix_a, i);
        for (u32 i = 0; i < 2; ++i) {
            inputs[4 + i] = ctx.OpCompositeExtract(u32ty, matrix_b, i);
            inputs[6 + i] = ctx.OpCompositeExtract(u32ty, matrix_c, i);
        }
        for (u32 row = 0; row < 2; ++row) {
            const Id c = unpack(inputs[6 + row]);
            for (u32 col = 0; col < 2; ++col)
                ctx.OpStore(accumulators[row * 2 + col], ctx.OpCompositeExtract(f32ty, c, col));
        }
        ctx.OpStore(counter, imm(0));
        const Id lane = ctx.OpLoad(u32ty, ctx.subgroup_local_invocation_id);
        const Id warp_base = and_(lane, ~31U);
        const Id row_base = and_(lane, 28);
        const Id col_base = ctx.OpShiftLeftLogical(u32ty, and_(lane, 3), imm(3));
        const Id a_base = add(warp_base, row_base);
        const Id b_base = add(warp_base, col_base);
        // Unrolling a K=8 group lets its shuffles overlap, which latency-bound small dispatches
        // need; unrolling all of K=16 raises register pressure and slows large dispatches.
        const auto emit_group = [&](auto pick) {
            for (u32 component = 0; component < 4; ++component) {
                const Id b = pick(inputs[4], inputs[5]);
                std::array<Id, 2> bs;
                for (u32 col = 0; col < 2; ++col)
                    bs[col] = unpack(ctx.OpGroupNonUniformShuffle(
                        u32ty, imm(3), b, add(b_base, imm(component + col * 4))));
                for (u32 row = 0; row < 2; ++row) {
                    const Id as = unpack(ctx.OpGroupNonUniformShuffle(
                        u32ty, imm(3), pick(inputs[row], inputs[row + 2]),
                        add(a_base, imm(component))));
                    for (u32 col = 0; col < 2; ++col) {
                        Id value = ctx.OpLoad(f32ty, accumulators[row * 2 + col]);
                        for (u32 half = 0; half < 2; ++half) {
                            value = ctx.OpFma(f32ty, ctx.OpCompositeExtract(f32ty, as, half),
                                              ctx.OpCompositeExtract(f32ty, bs[col], half), value);
                            ctx.Decorate(value, spv::Decoration::NoContraction);
                        }
                        ctx.OpStore(accumulators[row * 2 + col], value);
                    }
                }
            }
            // HMMA.F16 rounds after each K=8 group, including the midpoint of K=16.
            for (const Id accumulator : accumulators)
                ctx.OpStore(accumulator, round_half(ctx.OpLoad(f32ty, accumulator)));
        };
        if (variant == 0) {
            emit_group([](Id low, Id) { return low; });
        } else {
            const Id header = ctx.OpLabel(), body = ctx.OpLabel(), next = ctx.OpLabel(),
                     end = ctx.OpLabel();
            ctx.OpBranch(header);
            ctx.AddLabel(header);
            const Id group = ctx.OpLoad(u32ty, counter);
            const Id more = ctx.OpULessThan(ctx.U1, group, imm(2));
            ctx.OpLoopMerge(end, next, spv::LoopControlMask::DontUnroll);
            ctx.OpBranchConditional(more, body, end);
            ctx.AddLabel(body);
            const Id upper = ctx.OpIEqual(ctx.U1, group, imm(1));
            emit_group([&](Id low, Id high) { return ctx.OpSelect(u32ty, upper, high, low); });
            ctx.OpBranch(next);
            ctx.AddLabel(next);
            ctx.OpStore(counter, add(group, imm(1)));
            ctx.OpBranch(header);
            ctx.AddLabel(end);
        }
        std::array<Id, 2> packed;
        for (u32 row = 0; row < 2; ++row)
            packed[row] = ctx.OpPackHalf2x16(u32ty, ctx.OpCompositeConstruct(ctx.F32[2],
                ctx.OpLoad(f32ty, accumulators[row * 2]), ctx.OpLoad(f32ty, accumulators[row * 2 + 1])));
        ctx.OpReturnValue(ctx.OpCompositeConstruct(ctx.U32[2], packed[0], packed[1]));
        ctx.OpFunctionEnd();
    }
}

    const Id mat_a = ctx.OpCoopMatrixLoadNV(ctx.cooperative_matrix_type_a, ptr(abase), imm(16U),
                                            ctx.false_value);
    const Id mat_b =
        ctx.OpCoopMatrixLoadNV(ctx.cooperative_matrix_type_b, ptr(bbase), imm(8U), ctx.false_value);
    const Id mat_c =
        ctx.OpCoopMatrixLoadNV(ctx.cooperative_matrix_type_c, ptr(cbase), imm(8U), ctx.false_value);
    const Id mat_d{ctx.OpCoopMatrixMulAddNV(ctx.cooperative_matrix_type_c, mat_a, mat_b, mat_c)};
    ctx.OpCoopMatrixStoreNV(ptr(dbase), mat_d, imm(8U), ctx.false_value);
    barrier();

void PatchCooperativeHmma(std::vector<u32>& code, const CooperativeHmmaTypes& types) {
    const auto is_matrix{[&](u32 id) {
        return id == types.matrix_a.value || id == types.matrix_b.value ||
               id == types.matrix_c.value;
    }};
    std::vector<u32> products;
    for (size_t offset = 5; offset < code.size();) {
        const u32 word_count{code[offset] >> 16};
        if (word_count == 0) {
            return;
        }
        const auto opcode{static_cast<spv::Op>(code[offset] & 0xFFFFu)};
        std::optional<spv::Op> replacement;
        if (opcode == spv::Op::OpTypeStruct && is_matrix(code[offset + 1])) {
            replacement = spv::Op::OpTypeCooperativeMatrixKHR;
        } else if (opcode == spv::Op::OpSelect && code[offset + 1] == types.matrix_c.value) {
            replacement = spv::Op::OpCooperativeMatrixMulAddKHR;
            products.push_back(code[offset + 2]);
        } else if (opcode == spv::Op::OpBitFieldUExtract && is_matrix(code[offset + 1])) {
            replacement = spv::Op::OpCooperativeMatrixLoadKHR;
        } else if (opcode == spv::Op::OpStore && word_count == 5 &&
                   std::ranges::find(products, code[offset + 2]) != products.end()) {
            replacement = spv::Op::OpCooperativeMatrixStoreKHR;
        }
        if (replacement) {
            code[offset] = (code[offset] & 0xFFFF0000u) | static_cast<u32>(*replacement);
        }
        offset += word_count;
    }
}

std::vector<u32> EmitCooperativeHmmaTest(u32 spirv_version) {
    Sirit::Module module{spirv_version};
    module.AddCapability(spv::Capability::Shader);
    module.AddCapability(spv::Capability::GroupNonUniform);

    const Id void_type{module.TypeVoid()};
    const Id u32_type{module.TypeUInt(32)};
    const Id u32x2_type{module.TypeVector(u32_type, 2)};
    const Id u32x4_type{module.TypeVector(u32_type, 4)};
    const CooperativeHmmaTypes types{
        DefineCooperativeHmmaTypes(module, u32_type, 16, 8, spirv_version)};

    const Id words_type{module.TypeRuntimeArray(u32_type)};
    module.Decorate(words_type, spv::Decoration::ArrayStride, 4U);
    const Id block_type{module.TypeStruct(words_type)};
    module.Decorate(block_type, spv::Decoration::Block);
    module.MemberDecorate(block_type, 0U, spv::Decoration::Offset, 0U);
    const Id buffer{module.AddGlobalVariable(
        module.TypePointer(spv::StorageClass::StorageBuffer, block_type),
        spv::StorageClass::StorageBuffer)};
    module.Decorate(buffer, spv::Decoration::DescriptorSet, 0U);
    module.Decorate(buffer, spv::Decoration::Binding, 0U);
    const Id lane_variable{module.AddGlobalVariable(
        module.TypePointer(spv::StorageClass::Input, u32_type), spv::StorageClass::Input)};
    module.Decorate(lane_variable, spv::Decoration::BuiltIn,
                    spv::BuiltIn::SubgroupLocalInvocationId);
    const Id word_pointer{module.TypePointer(spv::StorageClass::StorageBuffer, u32_type)};
    const Id zero{module.Constant(u32_type, 0U)};

    const Id main{module.OpFunction(void_type, spv::FunctionControlMask::MaskNone,
                                    module.TypeFunction(void_type))};
    module.AddLabel();
    const Id base{module.OpIMul(u32_type, module.OpLoad(u32_type, lane_variable),
                                module.Constant(u32_type, COOPERATIVE_HMMA_TEST_LANE_WORDS))};
    const auto word_at{[&](u32 index) {
        return module.OpAccessChain(word_pointer, buffer, zero,
                                    module.OpIAdd(u32_type, base,
                                                  module.Constant(u32_type, index)));
    }};
    const auto load{[&](u32 index) { return module.OpLoad(u32_type, word_at(index)); }};
    const Id a{module.OpCompositeConstruct(u32x4_type, load(0), load(1), load(2), load(3))};
    const Id b{module.OpCompositeConstruct(u32x2_type, load(4), load(5))};
    const Id c{module.OpCompositeConstruct(u32x2_type, load(6), load(7))};
    const Id d{EmitCooperativeHmma(module, types, u32_type, u32x2_type, a, b, c)};
    module.OpStore(word_at(8), module.OpCompositeExtract(u32_type, d, 0U));
    module.OpStore(word_at(9), module.OpCompositeExtract(u32_type, d, 1U));
    module.OpReturn();
    module.OpFunctionEnd();

    // Since SPIR-V 1.4 the interface lists every global the entry point uses.
    std::vector<Id> interfaces{lane_variable};
    if (spirv_version >= 0x00010400U) {
        interfaces.push_back(buffer);
    }
    module.AddEntryPoint(spv::ExecutionModel::GLCompute, main, "main", interfaces);
    module.AddExecutionMode(main, spv::ExecutionMode::LocalSize, GUEST_WARP_LANES, 1U, 1U);

    std::vector<u32> code{module.Assemble()};
    PatchCooperativeHmma(code, types);
    return code;
}

} // namespace Shader::Backend::SPIRV
