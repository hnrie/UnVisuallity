//
// Created by savage on 17.04.2025.
//

#pragma once
#include <cstdint>
#include <Windows.h>

/**
 * @brief Re-bases a Hyperion RVA relative to RobloxPlayerBeta.dll.
 *
 * @tparam T Target pointer or function type.
 * @param rva Relative virtual address from Roblox's anti-cheat module.
 * @return T Absolute pointer usable inside this module.
 */
template <typename T>
inline T rebase_hyp(uintptr_t rva) {
    return rva != NULL ? (T)(rva + reinterpret_cast<uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.dll"))) : (T)(NULL);
};

namespace rbx::hyperion {
    /**
     * @brief Marks an address as safe within Hyperion's CFG bitmap.
     *
     * @param address Pointer to flag as executable.
     */
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