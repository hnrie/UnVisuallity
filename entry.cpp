#include "entry.h"

#include <future>
#include <fstream>
#include <winternl.h>

#include "curl_wrapper.h"
#include "globals.h"
#include "lstate.h"
#include "lualib.h"
#include "thread"
#include "nlohmann/json.hpp"
#include "src/core/environment/environment.h"
#include "src/core/execution/execution.h"
#include "src/core/render/render.h"

#include "src/rbx/engine/game.h"
#include "src/rbx/engine/hyperion.h"
#include "src/rbx/taskscheduler/taskscheduler.h"
#include "src/core/communication/communication.h"
#include "src/core/teleport_handler/teleport_handler.h"



// CXX EXCEPTION SUPPORT
namespace exceptions {
    std::uintptr_t image_base = 0;
    int image_size = 0;

    #define BASE_ALIGNMENT		0x10

    #define EH_MAGIC_NUMBER1        0x19930520
    #define EH_PURE_MAGIC_NUMBER1   0x01994000
    #define EH_EXCEPTION_NUMBER     ('msc' | 0xE0000000)

    #define VEHDATASIG_64 0xB16B00B500B16A33
    #define VEHDATASIG VEHDATASIG_64

    #pragma optimize( "", off )
    LONG CALLBACK VectoredHandlerShell(EXCEPTION_POINTERS * ExceptionInfo)
    {
        if (ExceptionInfo->ExceptionRecord->ExceptionCode == EH_EXCEPTION_NUMBER)
        {
            if (ExceptionInfo->ExceptionRecord->ExceptionInformation[2] >= image_base && ExceptionInfo->ExceptionRecord->ExceptionInformation[2] < image_base + image_size)
            {
                if (ExceptionInfo->ExceptionRecord->ExceptionInformation[0] == EH_PURE_MAGIC_NUMBER1 && ExceptionInfo->ExceptionRecord->ExceptionInformation[3] == 0)
                {
                    ExceptionInfo->ExceptionRecord->ExceptionInformation[0] = static_cast<ULONG_PTR>(EH_MAGIC_NUMBER1);

                    ExceptionInfo->ExceptionRecord->ExceptionInformation[3] = image_base;
                }
            }
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }
}

typedef struct LDR_MODULE {
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID BaseAddress;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    SHORT LoadCount;
    SHORT TlsIndex;
    LIST_ENTRY HashTableEntry;
    ULONG TimeDateStamp;
} LDR_MODULE, * PLDR_MODULE;

#define UNLINK(x)					\
(x).Flink->Blink = (x).Blink;	\
(x).Blink->Flink = (x).Flink;

inline PLDR_MODULE getLoadOrderModuleList()
{
#ifdef _WIN64
    uintptr_t pPEB = (uintptr_t)__readgsqword(0x60);
    constexpr uint8_t loaderDataOff = 0x18;
    constexpr uint8_t inLoadOrderModuleList = 0x10;
#else
    uintptr_t pPEB = (uintptr_t)__readfsdword(0x30);
    constexpr uint8_t LoaderDataOff = 0xC;
    constexpr uint8_t InLoadOrderModuleList = 0xC;
#endif

    /* PEB->LoaderData->InLoadOrderModuleList */
    return (PLDR_MODULE)(*(uintptr_t*)(pPEB + loaderDataOff) + inLoadOrderModuleList);
}

void UnlinkModuleFromPEB(HMODULE hModule)
{
    PLDR_MODULE StartEntry = getLoadOrderModuleList();
    PLDR_MODULE CurrentEntry = (PLDR_MODULE)StartEntry->InLoadOrderModuleList.Flink;

    while (StartEntry != CurrentEntry && CurrentEntry != NULL)
    {
        if (CurrentEntry->BaseAddress == hModule)
        {
            UNLINK(CurrentEntry->InLoadOrderModuleList);
            UNLINK(CurrentEntry->InMemoryOrderModuleList);
            UNLINK(CurrentEntry->InInitializationOrderModuleList);
            UNLINK(CurrentEntry->HashTableEntry);

            break;
        }

        CurrentEntry = (PLDR_MODULE)CurrentEntry->InLoadOrderModuleList.Flink;
    }
}

#define rebase_hyperion(x) reinterpret_cast<uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.dll")) + x

[[noreturn]] void entry(HMODULE DllModule) {
    std::uintptr_t dll_base = reinterpret_cast<std::uintptr_t>(DllModule);
    //MessageBoxA(nullptr, "pmo", "icl", MB_OK);
   // LoadLibraryA("msvcp140.dll");
    bool mmap = true;

/*
    *    const auto nt_headers= reinterpret_cast<IMAGE_NT_HEADERS *>(
                dll_base + reinterpret_cast<IMAGE_DOS_HEADER *>(dll_base)->e_lfanew);

            const auto image_size = nt_headers->OptionalHeader.SizeOfImage;

            const auto cfg_bitmap = *reinterpret_cast<uintptr_t *>(rebase_hyperion(0x2855a8));
            for (auto current_address = dll_base; current_address < dll_base + image_size; current_address += 0x10000) {
                const auto page_bit_address = reinterpret_cast<int *>(cfg_bitmap + (current_address >> 0x13));
                *page_bit_address = 0xFF;
            }
 */

uintptr_t app_data_va = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + rbx::rvas::app_data::singleton;

uintptr_t app_data_struct = *reinterpret_cast<uintptr_t*>( app_data_va );
while (!app_data_struct) {
    app_data_struct = *reinterpret_cast<uintptr_t*>( app_data_va );

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
int app_data_status = *reinterpret_cast<int*>( app_data_struct + rbx::offsets::app_data::game_status );

/*
    exceptions::image_base = reinterpret_cast<std::uintptr_t>(DllModule);
    auto* Dos = reinterpret_cast<IMAGE_DOS_HEADER*>(exceptions::image_base);
    auto* Nt = reinterpret_cast<IMAGE_NT_HEADERS*>(exceptions::image_base + Dos->e_lfanew);
    auto* Opt = &Nt->OptionalHeader;
    exceptions::image_size = Opt->SizeOfImage;
    AddVectoredExceptionHandler(1, exceptions::VectoredHandlerShell);
*/


     while (app_data_status != 4) {
         app_data_status = *reinterpret_cast<int*>( app_data_struct + rbx::offsets::app_data::game_status );

         std::this_thread::sleep_for(std::chrono::seconds(3));
     }

    uintptr_t data_model = g_teleport_handler->get_datamodel();
    while (!data_model) {
        data_model = g_teleport_handler->get_datamodel();

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    const std::filesystem::path path_file = std::filesystem::path(getenv(OBF("localappdata"))) / OBF("Visual") / OBF("path.txt");
    if (!std::filesystem::exists(path_file))
        std::filesystem::create_directories(path_file);

    std::ifstream f(path_file, std::ios::in | std::ios::binary);
    const auto sz = std::filesystem::file_size(path_file);
    std::string result(sz, '\0');
    f.read(result.data(), sz);
    globals::bin_path = std::filesystem::path(result.data());
    globals::bin_path = globals::bin_path.parent_path() / OBF("bin");

    const std::filesystem::path autoexec_folder = globals::bin_path.parent_path() / OBF("autoexec");
    if (!std::filesystem::exists(autoexec_folder))
        std::filesystem::create_directories(autoexec_folder);

    globals::autoexec_path = autoexec_folder;


    const uintptr_t script_context = g_taskscheduler->get_script_context();
    globals::script_context = script_context;
    //rbx::standard_out::printf(rbx::message_type::message_info, "script_context: %p", script_context);

    globals::datamodel = data_model;
   // rbx::standard_out::printf(rbx::message_type::message_info, "data_model: %p", data_model);

  //  MessageBoxA(nullptr, "eh", "eh", MB_OK);


    lua_State* roblox_state = g_taskscheduler->get_roblox_state();
    globals::roblox_state = roblox_state;

    lua_State* our_state = lua_newthread(roblox_state);

   // rbx::standard_out::printf(rbx::message_error, "state: %p", our_state);

    lua_ref(roblox_state, -1);
    globals::our_state = our_state;
    lua_pop(roblox_state, 1);

    luaL_sandboxthread(our_state);


    our_state->userdata->identity = 8;
    our_state->userdata->capabilities = g_execution->capabilities;


   // MessageBoxA(nullptr, "here 5", "boom", MB_OK);
   //rbx::standard_out::printf(rbx::message_info, "hooking scheduler");
    g_taskscheduler->initialize_hook();
    renderer::initialize();
   // rbx::standard_out::printf(rbx::message_info, "hooking renderer");

   //rbx::standard_out::printf(rbx::message_info, "initializing environment");
    environment::initialize(our_state);




  std::thread([] {
    g_communication->start();
    }).detach();

    g_teleport_handler->initialize();
  //  MessageBoxA(nullptr, "eh", "eh", MB_OK);

   // while (true) {  }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            globals::dll_module = hModule;

            std::thread(entry, hModule).detach();
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
