#include "lxzp_scanner.hpp"
#include <psapi.h>
#include <future>

using namespace lxzp;

c_process_scanner::c_process_scanner(const DWORD process_id) : process_handle_(nullptr) {
    if (process_id == GetCurrentProcessId()) {
        process_handle_ = GetCurrentProcess();
    } else {
        process_handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 
                                     FALSE, process_id);
    }
    
    if (is_valid()) {
        enumerate_memory_regions();
        enumerate_modules();
    }
}

c_process_scanner::~c_process_scanner() {
    cleanup();
}

c_process_scanner::c_process_scanner(c_process_scanner&& other) noexcept 
    : memory_regions_(std::move(other.memory_regions_)),
      modules_(std::move(other.modules_)),
      process_handle_(other.process_handle_) {
    other.process_handle_ = nullptr;
}

c_process_scanner& c_process_scanner::operator=(c_process_scanner&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        memory_regions_ = std::move(other.memory_regions_);
        modules_ = std::move(other.modules_);
        process_handle_ = other.process_handle_;
        
        other.process_handle_ = nullptr;
    }
    return *this;
}

std::pair<std::vector<std::uint8_t>, std::vector<bool>> 
c_process_scanner::parse_pattern(std::string_view pattern) {
    std::vector<std::uint8_t> bytes;
    std::vector<bool> mask;
    
    for (std::size_t i = 0; i < pattern.length(); i += 3) {
        if (i + 1 >= pattern.length()) break;
        
        if (pattern[i] == '?' || pattern[i + 1] == '?') {
            bytes.push_back(0x00);
            mask.push_back(false); // wildcard
        } else {
            auto hex_to_byte = [](char c) -> std::uint8_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return 0;
            };
            
            std::uint8_t byte = (hex_to_byte(pattern[i]) << 4) | hex_to_byte(pattern[i + 1]);
            bytes.push_back(byte);
            mask.push_back(true); // must match
        }
    }
    
    return {std::move(bytes), std::move(mask)};
}

std::vector<c_process_scanner::scan_result> 
c_process_scanner::scan_process(std::string_view pattern_str) {
    auto [pattern, mask] = parse_pattern(pattern_str);
    return scan_process(pattern, mask);
}

std::vector<c_process_scanner::scan_result> 
c_process_scanner::scan_process(const std::vector<std::uint8_t>& pattern, 
                             const std::vector<bool>& mask) {
    std::vector<scan_result> results;
    
    if (!is_valid()) {
        return results;
    }
    
    for (const auto& region : memory_regions_) {
        if (!region.readable) continue;
        
        auto region_results = scan_memory_region(region, pattern, mask);
        results.insert(results.end(), region_results.begin(), region_results.end());
    }
    
    return results;
}

std::vector<c_process_scanner::scan_result> 
c_process_scanner::scan_module(std::string_view module_name, std::string_view pattern_str) {
    auto [pattern, mask] = parse_pattern(pattern_str);
    return scan_module(module_name, pattern, mask);
}

std::vector<c_process_scanner::scan_result> 
c_process_scanner::scan_module(std::string_view module_name, 
                            const std::vector<std::uint8_t>& pattern, 
                            const std::vector<bool>& mask) {
    std::vector<scan_result> results;
    
    if (!is_valid()) {
        return results;
    }
    
    auto it = modules_.find(std::string(module_name));
    if (it == modules_.end()) {
        return results; // Module not found
    }
    
    const auto& module = it->second;
    memory_region region{
        .base_address = module.base_address,
        .size = module.size,
        .name = module.name,
        .readable = true,
        .writable = false,
        .executable = true
    };
    
    return scan_memory_region(region, pattern, mask);
}

std::optional<c_process_scanner::scan_result> 
c_process_scanner::find_pattern(std::string_view pattern_str) {
    auto results = scan_process(pattern_str);
    return results.empty() ? std::nullopt : std::make_optional(results[0]);
}

std::optional<c_process_scanner::scan_result> 
c_process_scanner::find_pattern_in_module(std::string_view module_name, 
                                       std::string_view pattern_str) {
    auto results = scan_module(module_name, pattern_str);
    return results.empty() ? std::nullopt : std::make_optional(results[0]);
}

std::optional<c_process_scanner::module_info> 
c_process_scanner::get_module_info(std::string_view module_name) const {
    auto it = modules_.find(std::string(module_name));
    return it != modules_.end() ? std::make_optional(it->second) : std::nullopt;
}

std::vector<c_process_scanner::module_info> 
c_process_scanner::get_all_modules() const {
    std::vector<module_info> modules;
    modules.reserve(modules_.size());
    
    for (const auto& [name, info] : modules_) {
        modules.push_back(info);
    }
    
    return modules;
}

const std::vector<c_process_scanner::memory_region>& 
c_process_scanner::get_memory_regions() const {
    return memory_regions_;
}

bool c_process_scanner::is_valid() const noexcept {
    return process_handle_ != nullptr && process_handle_ != INVALID_HANDLE_VALUE;
}

std::vector<c_process_scanner::scan_result> 
c_process_scanner::scan_memory_region(const memory_region& region, 
                                   const std::vector<std::uint8_t>& pattern, 
                                   const std::vector<bool>& mask) {
    std::vector<scan_result> results;
    
    if (pattern.empty() || pattern.size() != mask.size()) {
        return results;
    }

    const auto memory_buffer = read_memory(region.base_address, region.size);
    if (!memory_buffer) {
        return results;
    }

    const auto& buffer = *memory_buffer;
    const std::size_t pattern_size = pattern.size();
    
    if (buffer.size() < pattern_size) {
        return results;
    }

    // Use parallel search for large regions
    if (buffer.size() > 1024 * 1024) { // 1MB threshold
        const std::size_t search_size = buffer.size() - pattern_size + 1;
        const std::size_t thread_count = min(std::thread::hardware_concurrency(), 8u);
        const std::size_t chunk_size = search_size / thread_count;

        std::vector<std::future<std::vector<scan_result>>> futures;
        futures.reserve(thread_count);

        for (std::size_t t = 0; t < thread_count; ++t) {
            std::size_t start = t * chunk_size;
            std::size_t end = (t == thread_count - 1) ? search_size : (t + 1) * chunk_size;

            futures.emplace_back(std::async(std::launch::async, [&, start, end]() {
                std::vector<scan_result> thread_results;

                for (std::size_t i = start; i < end; ++i) {
                    bool found = true;

                    for (std::size_t j = 0; j < pattern_size; ++j) {
                        if (mask[j] && buffer[i + j] != pattern[j]) {
                            found = false;
                            break;
                        }
                    }

                    if (found) {
                        thread_results.push_back({
                            .address = region.base_address + i,
                            .offset = i,
                            .module_name = region.name
                        });
                    }
                }

                return thread_results;
            }));
        }

        // Collect results from all threads
        for (auto& future : futures) {
            auto thread_results = future.get();
            results.insert(results.end(), thread_results.begin(), thread_results.end());
        }
    } else {
        // Sequential search for smaller regions
        for (std::size_t i = 0; i <= buffer.size() - pattern_size; ++i) {
            bool found = true;
            
            for (std::size_t j = 0; j < pattern_size; ++j) {
                if (mask[j] && buffer[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            
            if (found) {
                results.push_back({
                    .address = region.base_address + i,
                    .offset = i,
                    .module_name = region.name
                });
            }
        }
    }
    
    return results;
}

std::optional<std::vector<std::uint8_t>> 
c_process_scanner::read_memory(std::uintptr_t address, std::size_t size) {
    if (!is_valid() || size == 0) {
        return std::nullopt;
    }
    
    std::vector<std::uint8_t> buffer(size);
    SIZE_T bytes_read;
    
    if (!ReadProcessMemory(process_handle_, reinterpret_cast<LPCVOID>(address), 
                          buffer.data(), size, &bytes_read) || bytes_read != size) {
        return std::nullopt;
    }
    
    return buffer;
}

void c_process_scanner::enumerate_memory_regions() {
    if (!is_valid()) {
        return;
    }
    
    memory_regions_.clear();
    
    MEMORY_BASIC_INFORMATION mbi;
    std::uintptr_t address = 0;
    
    while (VirtualQueryEx(process_handle_, reinterpret_cast<LPCVOID>(address), 
                         &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT) {
            memory_region region{
                .base_address = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress),
                .size = mbi.RegionSize,
                .name = "",
                .readable = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | 
                           PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0,
                .writable = (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0,
                .executable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | 
                             PAGE_EXECUTE_READWRITE)) != 0
            };
            memory_regions_.push_back(region);
        }
        
        address += mbi.RegionSize;
        
        // Prevent infinite loop
        if (address == 0) {
            break;
        }
    }
}

void c_process_scanner::enumerate_modules() {
    if (!is_valid()) {
        return;
    }
    
    modules_.clear();
    
    constexpr std::size_t max_modules = 1024;
    std::vector<HMODULE> modules(max_modules);
    DWORD bytes_needed;
    
    if (EnumProcessModules(process_handle_, modules.data(), 
                          static_cast<DWORD>(modules.size() * sizeof(HMODULE)), 
                          &bytes_needed)) {
        
        DWORD module_count = bytes_needed / sizeof(HMODULE);
        if (module_count > max_modules) {
            module_count = max_modules;
        }
        
        for (DWORD i = 0; i < module_count; ++i) {
            char module_name[MAX_PATH];
            char module_path[MAX_PATH];
            
            if (GetModuleBaseNameA(process_handle_, modules[i], 
                                  module_name, sizeof(module_name)) &&
                GetModuleFileNameExA(process_handle_, modules[i], 
                                    module_path, sizeof(module_path))) {
                
                MODULEINFO mod_info;
                if (GetModuleInformation(process_handle_, modules[i], 
                                       &mod_info, sizeof(mod_info))) {
                    modules_[module_name] = {
                        .base_address = reinterpret_cast<std::uintptr_t>(mod_info.lpBaseOfDll),
                        .size = mod_info.SizeOfImage,
                        .name = module_name,
                        .path = module_path
                    };
                }
            }
        }
    }
}

void c_process_scanner::cleanup() {
    if (process_handle_ != nullptr && 
        process_handle_ != INVALID_HANDLE_VALUE && 
        process_handle_ != GetCurrentProcess()) {
        CloseHandle(process_handle_);
    }
    process_handle_ = nullptr;
}