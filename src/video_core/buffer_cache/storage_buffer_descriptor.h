// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstring>
#include "common/common_types.h"

namespace VideoCommon {

template <typename Memory>
std::array<u64, 2> ReadStorageBufferDescriptor(const Memory& memory, GPUVAddr address,
                                             bool bare_pointer) {
    std::array<u64, 2> descriptor{};
    // One fresh translation for both fields; a host pointer cannot cross its device page.
    if (!bare_pointer && (address & 0xfff) <= 0x1000 - sizeof(descriptor)) {
        if (const auto* pointer = memory.GetPointer(address)) {
            std::memcpy(descriptor.data(), pointer, sizeof(descriptor));
            return descriptor;
        }
    }
    descriptor[0] = memory.template Read<u64>(address);
    if (!bare_pointer && descriptor[0] != 0) {
        descriptor[1] = memory.template Read<u64>(address + 8);
    }
    return descriptor;
}

} // namespace VideoCommon
