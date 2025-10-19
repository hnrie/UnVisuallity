//
// Created by savage on 17.04.2025.
//

#include "execution.h"

#include "lapi.h"
#include "lstate.h"
#include "lualib.h"
#include "Luau/Compiler.h"
#include "Luau/Bytecode.h"
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeUtils.h"
#include "xorstr/xorstr.h"
#include "zstd.h"
#include "xxhash.h"

class bytecode_encoder_t : public Luau::BytecodeEncoder { // thx shade :P
    inline void encode(uint32_t* data, size_t count) override {
        for (auto i = 0; i < count;)
        {
            uint8_t op = LUAU_INSN_OP(data[i]);
            const auto oplen = Luau::getOpLength((LuauOpcode)op);
            uint8_t new_op = op * 227;
            data[i] = (new_op) | (data[i] & ~0xff);
            i += oplen;
        }
    }
};

void set_capabilities(Proto *proto, uintptr_t* caps) {
    if (!proto)
        return;

    proto->userdata = caps;
    proto->linedefined = -1;
    for (int i = 0; i < proto->sizep; ++i)
        set_capabilities(proto->p[i], caps);
}

std::string compress_bytecode(std::string_view bytecode) {
    // Create a buffer.
    const auto data_size = bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);
    auto buffer = std::vector<char>(max_size + 8);

    // Copy RSB1 and data size into the buffer.
    strcpy_s(&buffer[0], buffer.capacity(), OBF("RSB1"));
    memcpy_s(&buffer[4], buffer.capacity(), &data_size, sizeof(data_size));

    // Copy compressed bytecode into the buffer.
    const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
    if (ZSTD_isError(compressed_size))
        return OBF("");

    // Encrypt the buffer.
    const auto size = compressed_size + 8;
    const auto key = XXH32(buffer.data(), size, 42u);
    const auto bytes = reinterpret_cast<const uint8_t*>(&key);

    for (auto i = 0u; i < size; ++i)
        buffer[i] ^= bytes[i % 4] + i * 41u;

    // Create and return output.
    return std::string(buffer.data(), size);
}


std::string execution::compile(const std::string& code) {
    auto encoder = bytecode_encoder_t();

    //const char *mutableglobals[] = {
    //    "Game", "Workspace", "game", "plugin", "script", "shared", "workspace",
    //    "_G",
    //    nullptr
    //};

    auto compileoptions = Luau::CompileOptions { };
    compileoptions.optimizationLevel = 1;//1;
    compileoptions.debugLevel = 1;//1;
    //compileoptions.vectorLib = "Vector3";
    //compileoptions.vectorCtor = "new";
    //compileoptions.vectorType = "Vector3";
    //compileoptions.mutableGlobals = mutableglobals;
    std::string bytecode = Luau::compile(code, compileoptions, {}, &encoder);
    return compress_bytecode(bytecode);
}

std::string execution::decompress_bytecode(const std::string& source) const {
    constexpr const char bytecode_magic[] = "RSB1";

    std::string input = source;

    std::uint8_t hash_bytes[4];
    memcpy(hash_bytes, &input[0], 4);

    for (auto i = 0u; i < 4; ++i) {
        hash_bytes[i] ^= bytecode_magic[i];
        hash_bytes[i] -= i * 41;
    }

    for ( size_t i = 0; i < input.size( ); ++i )
        input[i] ^= hash_bytes[i % 4] + i * 41;

    XXH32(&input[0], input.size(), 42);

    std::uint32_t data_size;
    memcpy(&data_size, &input[4], 4);
    std::vector<std::uint8_t> data(data_size);
    ZSTD_decompress(&data[0], data_size, &input[8], input.size() - 8);

    return std::string(reinterpret_cast<char*>(&data[0]), data_size);
}

int execution::load_string(lua_State* L, const std::string& chunk_name, const std::string& code) {
    std::string bytecode = this->compile(code);

    if (rbx::luau::vm_load(L, &bytecode, chunk_name.c_str(), 0) != LUA_OK) {
        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 2;
    }

    const Closure* cl = clvalue(luaA_toobject(L, -1));
    set_capabilities(cl->l.p, &capabilities);

    lua_setsafeenv(L, LUA_GLOBALSINDEX, false);

    return 1;
}


bool execution::run_code(lua_State* state, const std::string& code) {
    std::string bytecode = this->compile(code);

    if (bytecode.at(0) == 0) {
        const char* error = bytecode.c_str() + 1;
        rbx::standard_out::printf(rbx::message_type::message_warning, error);
        return false;
    }
    lua_State* L = lua_newthread(state);
    luaL_sandboxthread(L);
    lua_pop(state, 1);

    L->userdata->identity = 8;
    L->userdata->capabilities = capabilities;

    if (rbx::luau::vm_load(L, &bytecode, OBF(""), 0) != LUA_OK) {
        rbx::standard_out::printf(rbx::message_type::message_warning, lua_tostring(L, -1));
        lua_pop(L, 1);

        return false;
    }

    const Closure* cl = clvalue(luaA_toobject(L, -1));

    set_capabilities(cl->l.p, &capabilities);

    rbx::script_context::task_defer(L);
    return true;
}
