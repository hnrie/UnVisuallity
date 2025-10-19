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

namespace constants {
    constexpr const char* BYTECODE_MAGIC = "RSB1";
    constexpr size_t DATA_SIZE_OFFSET = 4;
    constexpr size_t COMPRESSED_DATA_OFFSET = 8;
    constexpr uint32_t XXHASH_SEED = 42u;
    constexpr uint32_t ENCRYPTION_CONSTANT_1 = 41u;
    constexpr size_t HASH_BYTES_SIZE = 4;
    constexpr uint8_t BYTECODE_OBFUSCATION_KEY = 227;
}

// Custom bytecode encoder to obfuscate Luau bytecode.
class bytecode_encoder_t : public Luau::BytecodeEncoder { // thx shade :P
    // Encodes the bytecode by multiplying each opcode by a constant.
    inline void encode(uint32_t* data, size_t count) override {
        for (auto i = 0; i < count;)
        {
            uint8_t op = LUAU_INSN_OP(data[i]);
            const auto oplen = Luau::getOpLength((LuauOpcode)op);
            uint8_t new_op = op * constants::BYTECODE_OBFUSCATION_KEY;
            data[i] = (new_op) | (data[i] & ~0xff);
            i += oplen;
        }
    }
};

// Sets capabilities for a given proto.
void set_capabilities(Proto *proto, uintptr_t* caps) {
    if (!proto)
        return;

    proto->userdata = caps;
    proto->linedefined = -1;
    for (int i = 0; i < proto->sizep; ++i)
        set_capabilities(proto->p[i], caps);
}

// Compresses and encrypts the given bytecode.
std::string compress_bytecode(std::string_view bytecode) {
    // Create a buffer.
    const auto data_size = bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);
    auto buffer = std::vector<char>(max_size + constants::COMPRESSED_DATA_OFFSET);

    // Copy RSB1 and data size into the buffer.
    strcpy_s(&buffer[0], buffer.capacity(), OBF(constants::BYTECODE_MAGIC));
    memcpy_s(&buffer[constants::DATA_SIZE_OFFSET], buffer.capacity(), &data_size, sizeof(data_size));

    // Copy compressed bytecode into the buffer.
    const auto compressed_size = ZSTD_compress(&buffer[constants::COMPRESSED_DATA_OFFSET], max_size, bytecode.data(), data_size, 3);
    if (ZSTD_isError(compressed_size))
        return OBF("");

    // Encrypt the buffer.
    const auto size = compressed_size + constants::COMPRESSED_DATA_OFFSET;
    const auto key = XXH32(buffer.data(), size, constants::XXHASH_SEED);
    const auto bytes = reinterpret_cast<const uint8_t*>(&key);

    for (auto i = 0u; i < size; ++i)
        buffer[i] ^= bytes[i % constants::HASH_BYTES_SIZE] + i * constants::ENCRYPTION_CONSTANT_1;

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

// Decompresses and decrypts the given bytecode.
std::string execution::decompress_bytecode(const std::string& source) const {
    std::string input = source;

    std::uint8_t hash_bytes[constants::HASH_BYTES_SIZE];
    memcpy(hash_bytes, &input[0], constants::HASH_BYTES_SIZE);

    for (auto i = 0u; i < constants::HASH_BYTES_SIZE; ++i) {
        hash_bytes[i] ^= constants::BYTECODE_MAGIC[i];
        hash_bytes[i] -= i * constants::ENCRYPTION_CONSTANT_1;
    }

    for ( size_t i = 0; i < input.size( ); ++i )
        input[i] ^= hash_bytes[i % constants::HASH_BYTES_SIZE] + i * constants::ENCRYPTION_CONSTANT_1;

    XXH32(&input[0], input.size(), constants::XXHASH_SEED);

    std::uint32_t data_size;
    memcpy(&data_size, &input[constants::DATA_SIZE_OFFSET], sizeof(data_size));
    std::vector<std::uint8_t> data(data_size);
    ZSTD_decompress(&data[0], data_size, &input[constants::COMPRESSED_DATA_OFFSET], input.size() - constants::COMPRESSED_DATA_OFFSET);

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
