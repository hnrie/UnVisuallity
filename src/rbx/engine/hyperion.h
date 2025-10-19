//
// Created by savage on 17.04.2025.
//

#pragma once
#include <cstdint>
#include <Windows.h>

template <typename T>
inline T rebase_hyp(uintptr_t rva) {
    return rva != NULL ? (T)(rva + reinterpret_cast<uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.dll"))) : (T)(NULL);
};

namespace rbx::hyperion {
    static void add_to_cfg(void* address) {
        if (address == nullptr)
            return;

        const auto Current = *reinterpret_cast<uint8_t*>(*rebase_hyp<uintptr_t*>(0x2A8558) + ((uintptr_t)address >> 0x13));
        if (Current != 0xFF)
        {
            *reinterpret_cast<uint8_t*>(*rebase_hyp<uintptr_t*>(0x2A8558) + ((uintptr_t)address >> 0x13)) = 0xFF;
        }
    }
}