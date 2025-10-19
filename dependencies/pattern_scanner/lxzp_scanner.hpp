//
// Created by user on 28/05/2025.
//

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <windows.h>

namespace lxzp
{
    class c_process_scanner {
public:
    struct scan_result {
        std::uintptr_t address;
        std::size_t offset;
        std::string module_name;
    };

    struct memory_region {
        std::uintptr_t base_address;
        std::size_t size;
        std::string name;
        bool readable;
        bool writable;
        bool executable;
    };

    struct module_info {
        std::uintptr_t base_address;
        std::size_t size;
        std::string name;
        std::string path;
    };

private:
    std::vector<memory_region> memory_regions_;
    std::unordered_map<std::string, module_info> modules_;
    HANDLE process_handle_;

public:
    explicit c_process_scanner(DWORD process_id = GetCurrentProcessId());
    ~c_process_scanner();

    c_process_scanner(const c_process_scanner&) = delete;
    c_process_scanner& operator=(const c_process_scanner&) = delete;

    c_process_scanner(c_process_scanner&& other) noexcept;
    c_process_scanner& operator=(c_process_scanner&& other) noexcept;

    static std::pair<std::vector<std::uint8_t>, std::vector<bool>>
    parse_pattern(std::string_view pattern);

    std::vector<scan_result> scan_process(std::string_view pattern_str);
    std::vector<scan_result> scan_process(const std::vector<std::uint8_t>& pattern,
                                         const std::vector<bool>& mask);

    std::vector<scan_result> scan_module(std::string_view module_name,
                                        std::string_view pattern_str);
    std::vector<scan_result> scan_module(std::string_view module_name,
                                        const std::vector<std::uint8_t>& pattern,
                                        const std::vector<bool>& mask);

    std::optional<scan_result> find_pattern(std::string_view pattern_str);

    std::optional<scan_result> find_pattern_in_module(std::string_view module_name,
                                                     std::string_view pattern_str);
    [[nodiscard]] std::optional<module_info> get_module_info(std::string_view module_name) const;

    [[nodiscard]] std::vector<module_info> get_all_modules() const;

    [[nodiscard]] const std::vector<memory_region>& get_memory_regions() const;

    [[nodiscard]] bool is_valid() const noexcept;

private:
    std::vector<scan_result> scan_memory_region(const memory_region& region,
                                              const std::vector<std::uint8_t>& pattern,
                                              const std::vector<bool>& mask);

    std::optional<std::vector<std::uint8_t>> read_memory(std::uintptr_t address,
                                                        std::size_t size);

    void enumerate_memory_regions();
    void enumerate_modules();
    void cleanup();
};

inline const auto g_scanner = std::make_unique<c_process_scanner>();
}