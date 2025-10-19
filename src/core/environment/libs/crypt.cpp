//
// Created by user on 24/04/2025.
//
#include <random>
#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "../environment.h"
#include "base64.h"
#include "libsodium/include/sodium.h"
#include "hex.h"
#include "sha.h"
#include "sha3.h"
#include "md5.h"
#include "modes.h"
#include "blowfish.h"
#include "gcm.h"
#include "aes.h"
#include "pwdbased.h"
#include "eax.h"
#include "lz4/lz4.h"

int crypt_base64encode(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    size_t input_len;
    const char* input = lua_tolstring(L, 1, &input_len);

    size_t encoded_len = sodium_base64_encoded_len(input_len, sodium_base64_VARIANT_ORIGINAL);
    char* encoded_data = (char*)malloc(encoded_len);

    if (!encoded_data) {
        luaL_error(L, OBF("memory allocation failed"));
    }

    sodium_bin2base64(
        encoded_data,
        encoded_len,
        (const unsigned char*)input,
        input_len,
        sodium_base64_VARIANT_ORIGINAL
    );

    lua_pushstring(L, encoded_data);
    free(encoded_data);

    return 1;
}

int crypt_base64decode(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    const char *data = lua_tolstring(L, 1, nullptr);

    std::string decoded;

    CryptoPP::StringSource ss( data, true,
                               new CryptoPP::Base64Decoder(
                                       new CryptoPP::StringSink( decoded )
                               )
    );

    luaC_threadbarrier(L);
    lua_pushlstring(L, decoded.c_str(), decoded.size());

    return 1;
}


int crypt_generatebytes(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);

    const size_t key_sz = static_cast<size_t>(lua_tointeger(L, 1));

    char* key = new char[key_sz];

    randombytes_buf(key, key_sz);

    const size_t encoded_len = sodium_base64_encoded_len(key_sz, sodium_base64_VARIANT_ORIGINAL);
    const auto encoded_data = static_cast<char*>(malloc(encoded_len));

    if (!encoded_data) {
        luaL_error(L, OBF("memory allocation failed"));
    }

    sodium_bin2base64(
            encoded_data,
            encoded_len,
            reinterpret_cast<const unsigned char*>(key),
            key_sz,
            sodium_base64_VARIANT_ORIGINAL
    );

    lua_pushstring(L, encoded_data);
    free(encoded_data);

    return 1;
}

int crypt_generatekey(lua_State* L) {
    lua_check(L, 0);
    constexpr size_t key_sz = 32;
    unsigned char key[key_sz];

    randombytes_buf(key, key_sz);

    size_t encoded_len = sodium_base64_encoded_len(key_sz, sodium_base64_VARIANT_ORIGINAL);
    char* encoded_data = (char*)malloc(encoded_len);

    if (!encoded_data) {
        luaL_error(L, OBF("memory allocation failed"));
    }

    sodium_bin2base64(
            encoded_data,
            encoded_len,
            (const unsigned char*)key,
            key_sz,
            sodium_base64_VARIANT_ORIGINAL
    );

    lua_pushstring(L, encoded_data);
    free(encoded_data);

    return 1;
}

template<typename T>
__forceinline std::string hash_with_algo(const std::string& input)
{
    T hash;
    std::string digest;

    CryptoPP::StringSource ss(input, true,
                              new CryptoPP::HashFilter(hash,
                                                       new CryptoPP::HexEncoder(
                                                               new CryptoPP::StringSink(digest), false
                                                       )
                              )
    );

    return digest;
}

enum hashes
{
    sha1,
    sha256,
    sha384,
    sha512,
    sha3_224,
    sha3_256,
    sha3_384,
    sha3_512,
    md5
};

static std::map<std::string, hashes> str_to_hash = {
        {OBF("sha1"), sha1},
        {OBF("sha256"), sha256},
        {OBF("sha384"), sha384},
        {OBF("sha512"), sha512},

        {OBF("sha3-224"), sha3_224},
        {OBF("sha3-256"), sha3_256},
        {OBF("sha3-384"), sha3_384},
        {OBF("sha3-512"), sha3_512},

        {OBF("md5"), md5}
};

int crypt_hash(lua_State* L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TSTRING);

    std::string s = luaL_checkstring(L, 1);
    std::string algo = luaL_optstring(L, 2, OBF("sha256"));

    std::string hashed{ };

    std::transform(algo.begin( ), algo.end( ), algo.begin( ), tolower);

    if (!str_to_hash.count(algo))
    {
        luaL_argerror(L, 2, OBF("unsupported hashing algorithm"));
    }

    auto hash_algo = str_to_hash[algo];

    switch (hash_algo)
    {
        case sha1:
            hashed = hash_with_algo<CryptoPP::SHA1>(s);
            break;
        case sha256:
            hashed = hash_with_algo<CryptoPP::SHA256>(s);
            break;
        case sha384:
            hashed = hash_with_algo<CryptoPP::SHA384>(s);
            break;
        case sha512:
            hashed = hash_with_algo<CryptoPP::SHA512>(s);
            break;
        case sha3_224:
            hashed = hash_with_algo<CryptoPP::SHA3_224>(s);
            break;
        case sha3_256:
            hashed = hash_with_algo<CryptoPP::SHA3_256>(s);
            break;
        case sha3_384:
             hashed = hash_with_algo<CryptoPP::SHA3_384>(s);
            break;
        case sha3_512:
            hashed = hash_with_algo<CryptoPP::SHA3_512>(s);
            break;
        case md5:
            hashed = hash_with_algo<CryptoPP::MD5>(s);
            break;

        default:
            luaL_argerror(L, 2, OBF("unsupported hashing algorithm"));
    }

    lua_pushlstring(L, hashed.c_str(), hashed.size());
    return 1;
}


enum crypts
{
    //AES
    AES_CBC,
    AES_CFB,
    AES_CTR,
    AES_OFB,
    AES_GCM,
    AES_EAX,

    //Blowfish
    BF_CBC,
    BF_CFB,
    BF_OFB
};

std::map<std::string, crypts> str_to_crypt = {
        //AES
        { "aes-cbc", AES_CBC },
        { "aes_cbc", AES_CBC },

        { "aes-cfb", AES_CFB },
        { "aes_cfb", AES_CFB },

        { "aes-ctr", AES_CTR },
        { "aes_ctr", AES_CTR },

        { "aes-ofb", AES_OFB },
        { "aes_ofb", AES_OFB },

        { "aes-gcm", AES_GCM },
        { "aes_gcm", AES_GCM },

        { "aes-eax", AES_EAX },
        { "aes_eax", AES_EAX },

        { "cbc", AES_CBC },

        { "cfb", AES_CFB },

        { "ctr", AES_CTR },

        { "ofb", AES_OFB },

        { "gcm", AES_GCM },

        { "eax", AES_EAX },

        //Blowfish
        { "blowfish-cbc", BF_CBC },
        { "blowfish_cbc", BF_CBC },
        { "bf-cbc", BF_CBC },
        { "bf_cbc", BF_CBC },

        { "blowfish-cfb", BF_CFB },
        { "blowfish_cfb", BF_CFB },
        { "bf-cfb", BF_CFB },
        { "bf_cfb", BF_CFB },

        { "blowfish-ofb", BF_OFB },
        { "blowfish_ofb", BF_OFB },
        { "bf-ofb", BF_OFB },
        { "bf_ofb", BF_OFB },
};

std::string base64_encode(std::string data) {
    std::string encoded;

    CryptoPP::StringSource ss((unsigned char*)( data.c_str( ) ), data.size(), true,
                               new CryptoPP::Base64Encoder(
                                       new CryptoPP::StringSink( encoded )
                               )
    );

    return encoded;
};

std::string base64_decode( const std::string& data ) {
    std::string decoded;

    CryptoPP::StringSource ss( data, true,
                               new CryptoPP::Base64Decoder(
                                       new CryptoPP::StringSink( decoded )
                               )
    );

    return decoded;
};

template<typename T>
std::string encrypt_with_algo(const std::string& data, const std::string& key, const std::string& IV)
{
    try
    {
        std::string encrypted;

        T encryptor;
        encryptor.SetKeyWithIV((uint8_t*)key.c_str( ), key.size( ), (uint8_t*)IV.c_str( ), IV.length( ));

        CryptoPP::StringSource ss(data, true,
                                  new CryptoPP::StreamTransformationFilter(encryptor,
                                                                           new CryptoPP::StringSink(encrypted)
                                  )
        );

        return base64_encode(encrypted);
    }
    catch (CryptoPP::Exception& e)
    {
        //LOGE("%s", e.what( ));
        return e.what( );
    }
}

template<typename T>
std::string encrypt_authenticated_with_algo(const std::string& data, const std::string& key, const std::string& IV)
{
    try
    {
        std::string encrypted;

        T encryptor;
        encryptor.SetKeyWithIV((uint8_t*)key.c_str( ), key.size( ), (uint8_t*)IV.c_str( ), IV.length( ));

        CryptoPP::AuthenticatedEncryptionFilter aef(encryptor,
                                                    new CryptoPP::StringSink(encrypted)
        );

        aef.Put((const uint8_t*)data.data( ), data.size( ));
        aef.MessageEnd( );

        return base64_encode(encrypted);
    }
    catch (CryptoPP::Exception& e)
    {
        //LOGE("%s", e.what( ));
        return e.what( );
    }
}

template<typename T>
std::string decrypt_with_algo(const std::string& cipherText, const std::string& key, const std::string& IV)
{
    try
    {
        std::string decrypted;

        T decryptor;
        decryptor.SetKeyWithIV((uint8_t*)key.c_str( ), key.size( ), (uint8_t*)IV.c_str( ), IV.size( ));

        const auto base = base64_decode(cipherText);

        CryptoPP::StringSource ss(base, true,
                                  new CryptoPP::StreamTransformationFilter(decryptor,
                                                                           new CryptoPP::StringSink(decrypted)
                                  )
        );

        return decrypted;
    }
    catch (CryptoPP::Exception& e)
    {
        return e.what( );
    }
}

template<typename T>
std::string decrypt_authenticated_with_algo(const std::string& cipherText, const std::string& key, const std::string& IV)
{
    try
    {
        std::string decrypted;

        T decryptor;
        decryptor.SetKeyWithIV((uint8_t*)key.c_str( ), key.size( ), (uint8_t*)IV.c_str( ), IV.size( ));

        const auto Base = base64_decode(cipherText);

        CryptoPP::AuthenticatedDecryptionFilter adf(decryptor,
                                                    new CryptoPP::StringSink(decrypted)
        );

        adf.Put((const uint8_t*)Base.data( ), Base.size( ));
        adf.MessageEnd( );

        return decrypted;
    }
    catch (CryptoPP::Exception& e)
    {
        //LOGE("%s", e.what( ));
        return e.what( );
    }
}


int random_integer(int min, int max)
{
    std::random_device rd;
    std::mt19937 eng(rd( ));
    std::uniform_int_distribution<std::intptr_t> distr(min, max);
    return distr(eng);
}

std::string random_string(size_t length)
{
    const auto randchar = [=]( ) -> char {
        std::string charSet = ("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        const auto maxIndex = charSet.size( ) - 1;
        return charSet.at(random_integer(1, 200000) % maxIndex);
    };

    std::string str(length, 0);
    std::generate_n(str.begin( ), length, randchar);
    return str;
}

int crypt_encrypt(lua_State* ls)
{
    lua_check(ls, 4);

    auto plain_Cstr = luaL_checkstring(ls, 1);
    size_t key_Size;
    auto key_Cstr = luaL_checklstring(ls, 2, &key_Size);
    auto algo_Cstr = luaL_optstring(ls, 4, "aes-cbc");

    const auto size = 32;
    auto alloc = (uint8_t*) operator new(size);
    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA384> KDF;
    KDF.DeriveKey(alloc, size, 0, (uint8_t*)key_Cstr, key_Size, NULL, 0, 10000);

    auto key = std::string(reinterpret_cast<const char *>(alloc), size);

    bool iv_was_generated = false;
    std::string IV{ };

    if ( lua_isnoneornil(ls, 3) )
    {
        lua_rawcheckstack(ls, 1);
        luaC_threadbarrier(ls);

        lua_pushcclosurek(ls, crypt_generatekey, nullptr, 0, nullptr);
        lua_call(ls, 0, 1);

        IV = lua_tostring(ls, -1);
        lua_pop(ls, 1);

        if ( IV.size( ) > 16 )
        {
            IV = IV.substr(0, 16);
        }
        iv_was_generated = true;
    }
    else
    {
        IV = lua_tostring(ls, 3);
        if ( IV.size( ) > 16 )
        {
            IV = IV.substr(0, 16);
        }
    }

    auto algo = std::string(algo_Cstr);
    std::ranges::transform(algo, algo.begin( ), tolower);

    if (!str_to_crypt.count(algo))
    {
        luaL_argerror(ls, 4, OBF("unsupported algorithm"));
    }

    auto plain = std::string(plain_Cstr);

    std::string result{ };
    auto c_algo = str_to_crypt[algo];

    switch ( c_algo )
    {
        case AES_CBC:
            result = encrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case AES_CFB:
            result = encrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case AES_CTR:
            result = encrypt_with_algo<CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case AES_OFB:
            result = encrypt_with_algo<CryptoPP::OFB_Mode<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case AES_GCM:
            result = encrypt_authenticated_with_algo<CryptoPP::GCM<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case AES_EAX:
            result = encrypt_authenticated_with_algo<CryptoPP::EAX<CryptoPP::AES>::Encryption>(plain, key, IV);
            break;

        case BF_CBC:
            result = encrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::Blowfish>::Encryption>(plain, key, IV);
            break;

        case BF_CFB:
            result = encrypt_with_algo<CryptoPP::CFB_Mode<CryptoPP::Blowfish>::Encryption>(plain, key, IV);
            break;

        case BF_OFB:
            result = encrypt_with_algo<CryptoPP::OFB_Mode<CryptoPP::Blowfish>::Encryption>(plain, key, IV);
            break;

        default:
            luaL_argerror(ls, 4, OBF("unsupported algorithm"));
    }

    std::string encodedIV = IV;

    lua_pushlstring(ls, result.c_str( ), result.size( ));
    lua_pushlstring(ls, encodedIV.c_str( ), encodedIV.size( ));

    delete alloc;
    return 2;
}

int crypt_decrypt(lua_State* L) {
    lua_check(L, 4);
    std::string plain = luaL_checkstring(L, 1);

    size_t key_Size;
    auto key_Cstr = luaL_checklstring(L, 2, &key_Size);

    const auto size = 32;
    auto alloc = (uint8_t*) operator new(size);
    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA384> KDF;
    KDF.DeriveKey(alloc, size, 0, (uint8_t*)key_Cstr, key_Size, nullptr, 0, 10000);

    auto key = std::string(reinterpret_cast<const char *>(alloc), size);

    std::string IV_encoded = luaL_checkstring(L, 3);
    std::string IV;

    if ( IV_encoded.find(OBF("ivgen")) != std::string::npos )
    {
        IV = base64_decode(IV_encoded.substr(5));
    }
    else
    {
        if ( IV_encoded.size( ) > 16 )
        {
            IV = IV_encoded.substr(0, 16);
        }
        else
        {
            IV = IV_encoded;
        }
    }

    std::string algo = luaL_checkstring(L, 4);

    std::transform(algo.begin(), algo.end( ), algo.begin( ), tolower);

    if ( !str_to_crypt.count(algo) )
    {
        luaL_argerror(L, 4,  OBF("unsupported algorithm"));
    }

    std::string result{ };
    auto c_algo = str_to_crypt[algo];

    switch (c_algo)
    {
        case AES_CBC:
            result = decrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case AES_CFB:
            result = decrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case AES_CTR:
            result = decrypt_with_algo<CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case AES_OFB:
            result = decrypt_with_algo<CryptoPP::OFB_Mode<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case AES_GCM:
            result = decrypt_authenticated_with_algo<CryptoPP::GCM<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case AES_EAX:
            result = decrypt_authenticated_with_algo<CryptoPP::EAX<CryptoPP::AES>::Decryption>(plain, key, IV);
            break;

        case BF_CBC:
            result = decrypt_with_algo<CryptoPP::CBC_Mode<CryptoPP::Blowfish>::Decryption>(plain, key, IV);
            break;

        case BF_CFB:
            result = decrypt_with_algo<CryptoPP::CFB_Mode<CryptoPP::Blowfish>::Decryption>(plain, key, IV);
            break;

        case BF_OFB:
            result = decrypt_with_algo<CryptoPP::OFB_Mode<CryptoPP::Blowfish>::Decryption>(plain, key, IV);
            break;

        default:
            luaL_argerror(L, 4, OBF("unsupported algorithm"));
    }

    lua_pushlstring(L, result.c_str( ), result.size( ));
    delete alloc;
    return 1;
}

int lz4compress(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    size_t len{};
    const char *source = lua_tolstring(L, 1, &len);

    const int max_compressed_sz = LZ4_compressBound(len);

    const auto buffer = new char[max_compressed_sz];
    memset(buffer, 0, max_compressed_sz);

    const auto actual_sz = LZ4_compress_default(source, buffer, len, max_compressed_sz);

    lua_pushlstring(L, buffer, actual_sz);
    return 1;
}

int lz4decompress(lua_State *L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TNUMBER);

    size_t len{};
    const char *source = lua_tolstring(L, 1, &len);
    int data_sz = lua_tointeger(L, 2);

    char *buffer = new char[data_sz];

    memset(buffer, 0, data_sz);

    LZ4_decompress_safe(source, buffer, len, data_sz);

    lua_pushlstring(L, buffer, data_sz);
    return 1;
}

void environment::load_crypt_lib(lua_State *L) {
    if (sodium_init() < 0) {
        LI_FN(MessageBoxA).safe()(nullptr, OBF("failed to load libsodium, report this issue."), OBF("Visual"), MB_OK);
    }

    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setglobal(L, "base64encode");

    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setglobal(L, "base64_encode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setglobal(L, "base64decode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setglobal(L, "base64_decode");

    lua_pushcclosure(L, lz4compress, nullptr, 0);
    lua_setglobal(L, "lz4compress");

    lua_pushcclosure(L, lz4decompress, nullptr, 0);
    lua_setglobal(L, "lz4decompress");



    lua_newtable(L);

    lua_newtable(L);
    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setfield(L, -2, "encode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setfield(L, -2, "decode");
    lua_setfield(L, -2, "base64");

    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setfield(L, -2, "base64encode");

    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setfield(L, -2, "base64_encode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setfield(L, -2, "base64decode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setfield(L, -2, "base64_decode");

    lua_pushcclosure(L, crypt_generatekey, nullptr, 0);
    lua_setfield(L, -2, "generatekey");

    lua_pushcclosure(L, crypt_generatebytes, nullptr, 0);
    lua_setfield(L, -2, "generatebytes");

    lua_pushcclosure(L, crypt_hash, nullptr, 0);
    lua_setfield(L, -2, "hash");

    lua_pushcclosure(L, crypt_encrypt, nullptr, 0);
    lua_setfield(L, -2, "encrypt");

    lua_pushcclosure(L, crypt_decrypt, nullptr, 0);
    lua_setfield(L, -2, "decrypt");

    lua_setglobal(L, "crypt");

    lua_newtable(L);
    lua_pushcclosure(L, crypt_base64encode, nullptr, 0);
    lua_setfield(L, -2, "encode");

    lua_pushcclosure(L, crypt_base64decode, nullptr, 0);
    lua_setfield(L, -2, "decode");
    lua_setglobal(L, "base64");
}