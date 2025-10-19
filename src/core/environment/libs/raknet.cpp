//
// Created by user on 09/05/2025.
#include <mutex>
#include <thread>

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"

struct network_stream_t
{
     unsigned __int8 *data;

     std::vector<unsigned char> dataVector;


    unsigned int numberOfBytesUsed;

    unsigned int numberOfBytesAllocated;
    unsigned int readOffset;

    bool ownedData;
};

bool compare_bytes(const BYTE* data, const BYTE* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern)
            return false;
    }
    return (*mask) == 0;
}

uintptr_t find_pattern(uintptr_t start, size_t size, const BYTE* pattern, const char* mask) {
     __try {
        for (size_t i = 0; i < size; ++i) {
            if (compare_bytes(reinterpret_cast<BYTE*>(start + i), pattern, mask)) {
                return start + i;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }
    return 0;
}

uintptr_t scan_process(const BYTE* pattern, const char* mask) {
    MEMORY_BASIC_INFORMATION mbi;

    uintptr_t start = 0x10000000000;
    uintptr_t end = 0x7FFFFFFFFFFFFFFF;

    while (start < end) {
        if (VirtualQuery(reinterpret_cast<LPCVOID>(start), &mbi, sizeof(mbi))) {
            if ((mbi.State == MEM_COMMIT) && (mbi.Protect & PAGE_READWRITE)) {
                uintptr_t match = find_pattern(reinterpret_cast<uintptr_t>(mbi.BaseAddress),
                                              mbi.RegionSize, pattern, mask);
                if (match != 0)
                    return match;
            }
            start += mbi.RegionSize;
        } else {
            break;
        }
    }
    return 0;
}

//using original_raknet_send_t = int64_t*(__fastcall*)(RakNet::RakPeer* _0, char _1, double _2);
enum PacketPriority {
    IMMEDIATE_PRIORITY,
    HIGH_PRIORITY,
    MEDIUM_PRIORITY,
    LOW_PRIORITY,
    NUMBER_OF_PRIORITIES
  };

enum PacketReliability {
    UNRELIABLE,
    UNRELIABLE_SEQUENCED,
    RELIABLE,
    RELIABLE_ORDERED,
    RELIABLE_SEQUENCED,
    UNRELIABLE_WITH_ACK_RECEIPT,
    RELIABLE_WITH_ACK_RECEIPT,
    RELIABLE_ORDERED_WITH_ACK_RECEIPT,
    NUMBER_OF_RELIABILITIES
  };

using original_raknet_send_t = int64_t(__fastcall*)(int64_t* a1, int64_t* a2, int a3, int a4, char a5, int64_t* a6, char a7, int a8);


original_raknet_send_t original_raknet_send = nullptr;
auto r_printf = rebase<void(__fastcall*)(int32_t, const char*, ...)>(0x16C1A30);

uintptr_t rakpeer;
bool enabled = false;

std::mutex raknet_mutx{};
std::stringstream buffer;

struct raknet_packet_t
{
    unsigned char *data;
    int size;
};

bool save_packet = false;
raknet_packet_t* latest_packet = nullptr;

using get_packet_size_t = int(*)(int64_t*);
using get_packet_data_t = unsigned char*(*)(int64_t*);

struct rakpeer_data
{
    int64_t* bitstream;
    char ordering_channel;
    int64_t* system_identifier;
} bum_data;

int64_t hook_22(int64_t *a1, int64_t* bitstream, int a3, int a4, char a5, int64_t* a6, char a7, int a8)
{
    raknet_mutx.lock();

    if (!bum_data.system_identifier)
        bum_data.system_identifier = a6;
    if (!bum_data.ordering_channel)
        bum_data.ordering_channel = a7;

    int64_t* rdi_4 = *(int64_t**)bitstream;

    get_packet_size_t get_packet_size = *(get_packet_size_t*)(*rdi_4 + 0x10);

    int packet_size = get_packet_size(rdi_4);


    get_packet_data_t get_packet_data = *(get_packet_data_t*)(*rdi_4 + 0x8);
   // rbx::standard_out::printf(1, "get packet data: %p", (uintptr_t)get_packet_data - (uintptr_t)GetModuleHandle(NULL));
    unsigned char* packet_data = (unsigned char*)*(uint64_t*)((char*)rdi_4 + 0x10);;
   /*
    * if (save_packet)
    {
    // raknet_mutx.unlock();
    // rbx::standard_out::printf(1, "hheyy");
    raknet_packet_t* packet = new raknet_packet_t{};
    packet->data = packet_data;
    packet->size = packet_size;

    latest_packet = packet;
    save_packet = false;
    }
    */

    if (packet_data[0] == 0x83 && packet_data[1] == 3 && packet_data[2] == 1)
    {
        //rbx::standard_out::printf(1, "equip sword packet blocked");
        if (!bum_data.bitstream)
            bum_data.bitstream = bitstream;;
        //raknet_mutx.unlock();
        //return 0;
    }

    raknet_mutx.unlock();
    return original_raknet_send(a1, bitstream, a3, a4, a5, a6, a7, a8);

}

int getpacket(lua_State *L)
{
    lua_newtable(L);

    save_packet = true;

    while (!latest_packet)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    for (int i = 1; i < latest_packet->size; i++)
    {
        lua_pushinteger(L, latest_packet->data[i - 1]);
        lua_rawseti(L, -2, i);
    }
        rbx::standard_out::printf(1, "finished!!");
    return 1;
}

int sendpacket(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TSTRING);

    original_raknet_send(reinterpret_cast<int64_t*>(rakpeer), bum_data.bitstream, HIGH_PRIORITY, RELIABLE, bum_data.ordering_channel, bum_data.system_identifier, true, 0);
    return 0;
}

void environment::load_raknet_lib(lua_State *L) {
    BYTE pattern[] = { 0x30, 0xAD, 0xF7, 0x48, 0xF6, 0x7F };

    rakpeer = scan_process(pattern, "xxxxxx");

    rbx::standard_out::printf(1, "rakpeer: %p", rakpeer);


    #define HOOK_INDEX 20
    #define VFTABLE_HOOK_SIZE 150




    if (rakpeer) {
        void **_vtable = new void*[VFTABLE_HOOK_SIZE + 1];
        memcpy(_vtable, *reinterpret_cast<void**>(rakpeer), sizeof(uintptr_t) * VFTABLE_HOOK_SIZE);
        original_raknet_send = reinterpret_cast<original_raknet_send_t>(_vtable[HOOK_INDEX]);
        rbx::standard_out::printf(1, "original send: %p", ((uintptr_t)original_raknet_send - (uintptr_t)GetModuleHandle(NULL)));
        _vtable[HOOK_INDEX] = hook_22;
        *reinterpret_cast< void** >(rakpeer) = _vtable;
    }



    lua_newtable(L);

    lua_pushcclosure(L, getpacket, nullptr, 0);
    lua_setfield(L, -2, "getpacket");
    lua_pushcclosure(L, sendpacket, nullptr, 0);
    lua_setfield(L, -2, "send");

    lua_setglobal(L, "raknet");


}