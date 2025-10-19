//
// Created by savage on 18.04.2025.
//

#include "globals.h"
#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"
#include "../environment.h"
#include "src/rbx/engine/game.h"
#include "lmem.h"
#include "sha.h"
#include "hex.h"
#include "filters.h"
#include "src/rbx/taskscheduler/taskscheduler.h"


#include "src/rbx/taskscheduler/yielding.h"

/* setidentity util cuz sunc gay */

__int64 __fastcall make_identity_to_capabilities(int identity)
{
    __int64 ret; // rax

    switch ( identity )
    {
        case 1:
        case 4:
            ret = 0x2000000000000003i64;
            break;
        case 3:
            ret = 0x200000000000000Bi64;
            break;
        case 5:
            ret = 0x2000000000000001i64;
            break;
        case 6:
            ret = 0x600000000000000Bi64;
            break;
        case 7:
        case 8:
            ret = 0x200000000000003Fi64;
            break;
        case 9:
            ret = 12i64;
            break;
        case 0xA:
            ret = 0x6000000000000003i64;
            break;
        case 0xB:
            ret = 0x2000000000000000i64;
            break;
        case 0xC:
            ret = 0x1000000000000000i64;
            break;
        default:
            ret = 0i64;
            break;
    }
    return ret | 0xFFFFFFF00i64;
}


int identifyexecutor(lua_State* L) {
    lua_check(L, 0);

    lua_pushstring(L, OBF("Visual"));
    lua_pushstring(L, OBF("1.0"));
    return 2;
}

int getgenv(lua_State* L) {
    lua_check(L, 0);

    const auto our_state = globals::our_state;

    if (our_state == L) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        return 1;
    }

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(our_state);

    lua_pushvalue(our_state, LUA_GLOBALSINDEX);
    lua_xmove(our_state, L, 1);

    return 1;
}

int getrenv(lua_State *L) {
    lua_check(L, 0);
    lua_State* roblox_state = globals::roblox_state;
    LuaTable *clone = luaH_clone(L, roblox_state->gt);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(roblox_state);

    L->top->value.p = clone;
    L->top->tt = LUA_TTABLE;
    L->top++;

    lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
    lua_setfield(L, -2, OBF("_G"));
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    lua_setfield(L, -2, OBF("shared"));
    return 1;
}

int gettenv(lua_State *L)
{
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TTHREAD);

    const auto thread = lua_tothread(L, 1);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(thread);

    lua_pushvalue(thread, LUA_GLOBALSINDEX);
    lua_xmove(thread, L, 1);

    return 1;
}

int getreg(lua_State *L) {
    lua_check(L, 0);

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

int getgc(lua_State *L) {
    lua_check(L, 1);

    bool include_tables = luaL_optboolean(L, 1, false);

    lua_newtable(L);

    global_State* g = L->global;
    int i = 0;

    for (lua_Page* curr = g->allgcopages; curr;)
    {
        lua_Page* next = curr->listnext; // block visit might destroy the page

        char* start;
        char* end;
        int busyBlocks;
        int blockSize;
        luaM_getpagewalkinfo(curr, &start, &end, &busyBlocks, &blockSize);

        for (char* pos = start; pos != end; pos += blockSize)
        {
            const auto gco = reinterpret_cast<GCObject*>(pos);

            if (!gco)
                continue;

            if (isdead(g, gco))
                continue;

            // skip memory blocks that are already freed
            if (gco->gch.tt == LUA_TFUNCTION || gco->gch.tt == LUA_TUSERDATA || (include_tables && gco->gch.tt == LUA_TTABLE)) {
                lua_rawcheckstack(L, 1);
                luaC_threadbarrier(L);

                L->top->value.gc = gco;
                L->top->tt = gco->gch.tt;
                L->top++;

                lua_rawseti(L, -2, ++i);
            } else
                continue;
        }

        curr = next;
    }

    return 1;
}

std::size_t calculateProtoBytesSize(const Proto *proto) {
    std::size_t size = 2 * sizeof(std::byte);

    for (auto i = 0; i < proto->sizek; i++) {
        switch (const auto currentConstant = &proto->k[i]; currentConstant->tt) {
            case LUA_TNIL:
                size += 1;
                break;
            case LUA_TSTRING: {
                const auto currentConstantString = &currentConstant->value.gc->ts;
                size += currentConstantString->len;
                break;
            }
            case LUA_TNUMBER: {
                size += sizeof(lua_Number);
                break;
            }
            case LUA_TBOOLEAN:
                size += sizeof(int);
                break;
            case LUA_TVECTOR:
                size += sizeof(float) * 3;
                break;
            case LUA_TTABLE: {
                const auto currentConstantTable = &currentConstant->value.gc->h;
                if (currentConstantTable->node != reinterpret_cast<LuaNode*>(rbx::rvas::luau::luah_dummynode)) {
                    for (int nodeI = 0; nodeI < sizenode(currentConstantTable); nodeI++) {
                        const auto currentNode = &currentConstantTable->node[nodeI];
                        if (currentNode->key.tt != LUA_TSTRING)
                            continue;
                        const auto nodeString = &currentNode->key.value.gc->ts;
                        size += nodeString->len;
                    }
                }
                break;
            }
            case LUA_TFUNCTION: {
                const auto constantFunction = &currentConstant->value.gc->cl;
                if (constantFunction->isC) {
                    size += 1;
                    break;
                }
                size += calculateProtoBytesSize(constantFunction->l.p);
                break;
            }
            default:
                break;
        }
    }

    for (auto i = 0; i < proto->sizep; i++)
        size += calculateProtoBytesSize(proto->p[i]);

    size += proto->sizecode * sizeof(Instruction);

    return size;
}

std::vector<unsigned char> getProtoBytes(const Proto *proto) {
    std::vector<unsigned char> protoBytes{};
    auto protoBytesSize = calculateProtoBytesSize(proto); // Pre-Calculate size (optional)
    protoBytes.reserve(protoBytesSize); // Pre-Calculate size (optional)
    protoBytes.push_back(proto->is_vararg);
    protoBytes.push_back(proto->numparams);

    for (auto i = 0; i < proto->sizek; i++) {
        switch (const auto currentConstant = &proto->k[i]; currentConstant->tt) {
            case LUA_TNIL:
                protoBytes.push_back(0);
                break;
            case LUA_TSTRING: {
                const auto currentConstantString = &currentConstant->value.gc->ts;
                protoBytes.insert(protoBytes.end(),
                                  reinterpret_cast<const char *>(currentConstantString->data),
                                  reinterpret_cast<const char *>(currentConstantString->data) + currentConstantString->
                                          len
                );
                break;
            }
            case LUA_TNUMBER: {
                const lua_Number *n = &currentConstant->value.n;
                protoBytes.insert(protoBytes.end(),
                                  reinterpret_cast<const char *>(n),
                                  reinterpret_cast<const char *>(n) + sizeof(lua_Number)
                );
                break;
            }
            case LUA_TBOOLEAN:
                protoBytes.push_back(currentConstant->value.b);
                break;
            case LUA_TVECTOR:
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[0] & 0xff000000);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[0] & 0x00ff0000);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[0] & 0x0000ff00);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[0] & 0x000000ff);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[1] & 0xff000000);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[1] & 0x00ff0000);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[1] & 0x0000ff00);
                protoBytes.push_back(reinterpret_cast<int *>(currentConstant->value.v)[1] & 0x000000ff);
                protoBytes.push_back(currentConstant->extra[0] & 0xff000000); // z lives in extra
                protoBytes.push_back(currentConstant->extra[0] & 0x00ff0000);
                protoBytes.push_back(currentConstant->extra[0] & 0x0000ff00);
                protoBytes.push_back(currentConstant->extra[0] & 0x000000ff);
                break;
            case LUA_TTABLE: {
                const auto currentConstantTable = &currentConstant->value.gc->h;
                if (currentConstantTable->node != reinterpret_cast<LuaNode*>(rbx::rvas::luau::luah_dummynode)) {
                    for (int nodeI = 0; nodeI < sizenode(currentConstantTable); nodeI++) {
                        const auto currentNode = &currentConstantTable->node[nodeI];
                        if (currentNode->key.tt != LUA_TSTRING)
                            continue;
                        const auto nodeString = &currentNode->key.value.gc->ts;
                        protoBytes.insert(protoBytes.end(),
                                          reinterpret_cast<const char *>(nodeString->data),
                                          reinterpret_cast<const char *>(nodeString->data) + nodeString->len
                        );
                    }
                }
                break;
            }
            case LUA_TFUNCTION: {
                const auto constantFunction = &currentConstant->value.gc->cl;
                if (constantFunction->isC) {
                    protoBytes.push_back('C');
                    break;
                }

                std::vector<unsigned char> functionBytes = getProtoBytes(constantFunction->l.p);
                protoBytes.insert(protoBytes.end(), functionBytes.data(), functionBytes.data() + functionBytes.size());
                break;
            }
            default:
                break;
        }
    }

    for (auto i = 0; i < proto->sizep; i++) {
        std::vector<unsigned char> currentProtoBytes = getProtoBytes(proto->p[i]);

        protoBytes.insert(protoBytes.end(), currentProtoBytes.data(),
                          currentProtoBytes.data() + currentProtoBytes.size()
        );
    }


    protoBytes.insert(protoBytes.end(), reinterpret_cast<const char *>(proto->code),
                      reinterpret_cast<const char *>(proto->code) + proto->sizecode * sizeof(Instruction)
    );
    return protoBytes;
}

int getfunctionhash(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TFUNCTION);

    if (lua_iscfunction(L, 1)) {
        luaL_argerrorL(L, 1, "lua function expected");
        return 0;
    }

    Closure *closure = clvalue(luaA_toobject(L, 1));

    const auto closure_bytes = getProtoBytes(closure->l.p); // getProtoBytes and getProtoSize taken from sUnc docs

    std::string hash_digest{};
    hash_digest.resize(48);

    CryptoPP::SHA384 hasher;
    hasher.CalculateDigest(reinterpret_cast<std::uint8_t *>(hash_digest.data()), closure_bytes.data(),
                           closure_bytes.size()
    );

    std::string final_hash{};
    CryptoPP::HexEncoder encoder;
    encoder.Attach(new CryptoPP::StringSink(final_hash));
    encoder.Put(reinterpret_cast<uint8_t *>(hash_digest.data()), hash_digest.size());
    encoder.MessageEnd();

    lua_pushlstring(L, final_hash.c_str(), final_hash.length());
    return 1;
}

__forceinline bool compare_tables(LuaTable *table_one, LuaTable *table_two) {
    return table_one->sizearray == table_two->sizearray && table_one->node == table_two->node;
}

int filtergc(lua_State *L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TTABLE);

    const char* filter_type = lua_tostring(L, 1);
    bool return_one = luaL_optboolean(L, 3, false);

    // function filter variables
    const char* name = nullptr;
    bool ignore_executor_closures = true;
    std::string hash;
    LuaTable *closure_env = nullptr;
    Proto *searched_proto = nullptr;
    std::vector<lua_TValue> constants;
    std::vector<lua_TValue> upvalues;

    // table filter variables
    std::vector<const lua_TValue*> keys;
    std::vector<const lua_TValue*> values;
    std::vector<std::pair<const lua_TValue*, const lua_TValue*>> pairs;
    LuaTable* metatable = nullptr;

    if (strcmp(filter_type, OBF("function")) == 0) {

        lua_getfield(L, 2, OBF("Name"));
        if (!lua_isnil(L, -1)) {
            name = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("IgnoreExecutor"));
        if (!lua_isnil(L, -1)) {
            ignore_executor_closures = lua_toboolean(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("Environment"));
        if (!lua_isnil(L, -1)) {
            closure_env = hvalue(luaA_toobject(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("Hash"));
        if (!lua_isnil(L, -1)) {
            hash = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("Constants"));
        if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                constants.push_back(*luaA_toobject(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);


        lua_getfield(L, 2, OBF("Upvalues"));
        if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                upvalues.push_back(*luaA_toobject(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    } else if (strcmp(filter_type, OBF("table")) == 0) {
        lua_getfield(L, 2, OBF("Keys"));
        if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                keys.push_back(luaA_toobject(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("Values"));
        if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                values.push_back(luaA_toobject(L, -1));
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("KeyValuePairs"));
        if (!lua_isnil(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                const lua_TValue* key = luaA_toobject(L, -2);
                const lua_TValue* value = luaA_toobject(L, -1);
                pairs.emplace_back(key, value);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, OBF("Metatable"));
        if (!lua_isnil(L, -1)) {
            metatable = hvalue(luaA_toobject(L, -1));
        }
        lua_pop(L, 1);
    } else {
        luaL_argerror(L, 1, OBF("'function' and 'table' are the only filter types"));
    }


    if (!return_one)
        lua_newtable(L);

    global_State* g = L->global;
    int table_idx = 0;

    if (strcmp(filter_type, OBF("function")) == 0) {
        for (lua_Page* curr = g->allgcopages; curr;)
        {
            lua_Page* next = curr->listnext; // block visit might destroy the page

            char* start;
            char* end;
            int busyBlocks;
            int blockSize;
            luaM_getpagewalkinfo(curr, &start, &end, &busyBlocks, &blockSize);

            for (char* pos = start; pos != end; pos += blockSize)
            {
                const auto gco = reinterpret_cast<GCObject*>(pos);

                if (!gco)
                    continue;

                if (isdead(g, gco))
                    continue;

                if (gco->gch.tt != LUA_TFUNCTION)
                    continue;

                Closure* cl = (Closure*)pos;

                if (!constants.empty() && cl->isC) continue;
                if (!upvalues.empty() && cl->nupvalues == 0) continue;
                if (!hash.empty() && cl->isC) continue;

                if (ignore_executor_closures && (cl->isC && cl->c.debugname == nullptr || !cl->isC && cl->l.p->linedefined == -1)){
                    continue;
                }


                if (name != nullptr && getfuncname(cl) != name)
                    continue;

                if (!hash.empty()) {
                    const auto closure_bytes = getProtoBytes(cl->l.p); // getProtoBytes and getProtoSize taken from sUnc docs

                    std::string hash_digest{};
                    hash_digest.resize(48);

                    CryptoPP::SHA384 hasher;
                    hasher.CalculateDigest(reinterpret_cast<std::uint8_t *>(hash_digest.data()), closure_bytes.data(),
                                           closure_bytes.size()
                    );

                    std::string final_hash{};
                    CryptoPP::HexEncoder encoder;
                    encoder.Attach(new CryptoPP::StringSink(final_hash));
                    encoder.Put(reinterpret_cast<uint8_t *>(hash_digest.data()), hash_digest.size());
                    encoder.MessageEnd();

                    if (final_hash != hash)
                        continue;
                }


                int constants_matched = 0;
                const size_t num_constants = constants.size();

                for (const auto& k2 : constants) {

                    const int sizek = cl->l.p->sizek;
                    const TValue* cl_k = cl->l.p->k;

                    for (int i = 0; i < sizek; i++) {
                        const TValue& k = cl_k[i];

                        if (k.tt != k2.tt) continue; // let's not waste our time on tsss we want very fast filtergc!!

                        if (k.tt == LUA_TNUMBER) {
                            if (memcmp(&k.value.n, &k2.value.n, sizeof(lua_Number)) == 0) {
                                constants_matched++;

                            }
                        }
                        else if (k.tt == LUA_TSTRING) {
                            if (tsvalue(&k)->hash == tsvalue(&k2)->hash) {
                                constants_matched++;

                            }
                        }
                    }
                }

                if (constants_matched != num_constants)
                    continue;


                int upvalues_matched = 0;
                const size_t num_upvalues = upvalues.size();

                for (const auto& u1 : upvalues) {

                    const int upvalue_count = cl->nupvalues;


                    for (int i = 0; i < upvalue_count; i++) {
                        TValue* u2;


                        const char* upvalue_name = aux_upvalue_2(cl, i+1, &u2);


                        if (upvalue_name) {

                            if (u1.tt != u2->tt) continue; // let's not waste our time on tsss we want very fast filtergc!!

                            if (u2->tt == LUA_TNUMBER) {
                                if (memcmp(&u1.value.n, &u2->value.n, sizeof(lua_Number)) == 0) {
                                    upvalues_matched++;

                                }
                            } else if (u2->tt == LUA_TSTRING) {
                                if (tsvalue(u2)->hash == tsvalue(&u1)->hash) {
                                    upvalues_matched++;

                                }
                            }
                        }
                    }
                }

                if (upvalues_matched != num_upvalues)
                    continue;

                lua_rawcheckstack(L, 1);
                luaC_threadbarrier(L);

                L->top->value.gc = reinterpret_cast<GCObject*>(pos);
                L->top->tt = LUA_TFUNCTION;
                L->top++;

                if (!return_one)
                    lua_rawseti(L, -2, ++table_idx);
                else
                    return 1;
            }

            curr = next;
        }
    } else if (strcmp(filter_type, OBF("table")) == 0) {
        for (lua_Page* curr = g->allgcopages; curr;)
        {
            lua_Page* next = curr->listnext; // block visit might destroy the page

            char* start;
            char* end;
            int busyBlocks;
            int blockSize;
            luaM_getpagewalkinfo(curr, &start, &end, &busyBlocks, &blockSize);

            for (char* pos = start; pos != end; pos += blockSize)
            {
                GCObject* gco = (GCObject*)pos;

                if (gco->gch.tt != LUA_TTABLE)
                    continue;

                LuaTable* table = (LuaTable*)pos;

                auto found_all_keys = true;

                for (const auto key: keys) {
                    if (luaH_get(table, key) == luaO_nilobject) {
                        found_all_keys = false;
                        break;
                    }
                }

                if (!found_all_keys)
                    continue;


                lua_rawcheckstack(L, 1);
                luaC_threadbarrier(L);

                L->top->tt = ::lua_Type::LUA_TTABLE;
                L->top->value.gc = gco;
                L->top++;

                lua_pushnil(L);

                std::vector<lua_TValue> keys_;
                std::vector<lua_TValue> values_;

                while (lua_next(L, -2) != 0) {
                    auto key = luaA_toobject(L, -2);
                    auto value = luaA_toobject(L, -1);
                    keys_.emplace_back(*key);
                    values_.emplace_back(*value);

                    lua_pop(L, 1);
                }
                lua_pop(L, 1);

                auto values_found = 0;

                for (const auto value: values) {
                    for (const auto &valueFound: values) {
                        if (value->tt != valueFound->tt)
                            continue; // Different types, ain't happening chief.

                        auto gcobject_1 = value->value.gc;
                        auto gcobject_2 = value->value.gc;

                        if (value->tt == ::lua_Type::LUA_TTABLE) {
                            if (compare_tables(&gcobject_1->h, &gcobject_2->h))
                                values_found++;
                        } else if (value->tt == ::lua_Type::LUA_TFUNCTION) {
                            if (gcobject_1->cl.isC != gcobject_2->cl.isC)
                                continue;

                            if (gcobject_1->cl.isC) {
                                if (gcobject_1->cl.c.f == gcobject_2->cl.c.f)
                                    values_found++;
                            } else {
                                if (gcobject_1->cl.l.p->sizecode == gcobject_2->cl.l.p->sizecode ||
                                    memcmp(gcobject_1->cl.l.p->code, gcobject_2->cl.l.p->code, gcobject_1->cl.l.p->sizecode) ==
                                    0)
                                    values_found++;
                            }
                        } else if (value->tt == ::lua_Type::LUA_TUSERDATA) {
                            // Cannot compare other than by pointer addy.
                            if (gcobject_1->u.data == gcobject_2->u.data)
                                values_found++;
                        } else if (value->tt == ::lua_Type::LUA_TBUFFER) {
                            if (gcobject_1->buf.len != gcobject_2->buf.len)
                                continue;

                            if (memcmp(gcobject_1->buf.data, gcobject_2->buf.data, gcobject_1->buf.len) == 0)
                                values_found++;
                        }
                    }
                }

                for (const auto key: keys) {
                    for (const auto &keyFound: keys) {
                        if (key->tt != keyFound->tt)
                            continue; // Different types, ain't happening chief.

                        auto gcobject_1 = key->value.gc;
                        auto gcobject_2 = key->value.gc;

                        if (key->tt == ::lua_Type::LUA_TTABLE) {
                            if (compare_tables(&gcobject_1->h, &gcobject_2->h))
                                values_found++;
                        } else if (key->tt == ::lua_Type::LUA_TFUNCTION) {
                            if (gcobject_1->cl.isC != gcobject_2->cl.isC)
                                continue;

                            if (gcobject_1->cl.isC) {
                                if (gcobject_1->cl.c.f == gcobject_2->cl.c.f)
                                    values_found++;
                            } else {
                                if (gcobject_1->cl.l.p->sizecode == gcobject_2->cl.l.p->sizecode ||
                                    memcmp(gcobject_1->cl.l.p->code, gcobject_2->cl.l.p->code, gcobject_1->cl.l.p->sizecode) ==
                                    0)
                                    values_found++;
                            }
                        } else if (key->tt == ::lua_Type::LUA_TUSERDATA) {
                            // Cannot compare other than by pointer addy.
                            if (gcobject_1->u.data == gcobject_2->u.data)
                                values_found++;
                        } else if (key->tt == ::lua_Type::LUA_TBUFFER) {
                            if (gcobject_1->buf.len != gcobject_2->buf.len)
                                continue;

                            if (memcmp(gcobject_1->buf.data, gcobject_2->buf.data, gcobject_1->buf.len) == 0)
                                values_found++;
                        }
                    }
                }


                if (metatable && table->metatable != metatable) {
                    continue;
                }

                lua_rawcheckstack(L, 1);
                luaC_threadbarrier(L);

                L->top->value.gc = gco;
                L->top->tt = gco->gch.tt;
                L->top++;

                if (!return_one)
                    lua_rawseti(L, -2, ++table_idx);
                else
                    return 1;
            }

            curr = next;
        }
    }

    if (return_one)
        lua_pushnil(L);

    return 1;
}

int getidentity(lua_State *L) {
    lua_check(L, 0);
    if (L->userdata == nullptr)
        return 0;

    lua_pushinteger(L, L->userdata->identity);
    return 1;
}

int setidentity(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);

    const int new_identity = lua_tointeger(L, 1);

    if (L->userdata == nullptr)
        return 0;

    const int64_t new_capabilities = make_identity_to_capabilities(new_identity);

    L->userdata->identity = new_identity;
    L->userdata->capabilities = new_capabilities;

    const uintptr_t singleton = *reinterpret_cast<uintptr_t*>((std::uintptr_t)GetModuleHandleA(nullptr )+ rbx::rvas::identity::identity_struct);
    const uintptr_t identity_struct = rbx::identity::get_identity_struct(singleton);
    if (!identity_struct) {
        return 0;
    }

    *reinterpret_cast<std::int32_t*>(identity_struct) = new_identity;
    *reinterpret_cast<std::uintptr_t*>(identity_struct + 0x20) = new_capabilities;

    return 0;
}

int getfpscap(lua_State* L) {
    lua_check(L, 0);
    lua_pushnumber(L, g_taskscheduler->get_fps());

    return 1;
}

int setfpscap(lua_State* L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TNUMBER);


    g_taskscheduler->set_fps(lua_tonumber(L, 1));

    return 0;
}

int setclipboard(lua_State* L)
{
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string value = lua_tostring(L, 1);

    if (!OpenClipboard(nullptr))
        return 0;

    EmptyClipboard();

    const auto global_alloc = GlobalAlloc(GMEM_MOVEABLE, value.size() + 1);
    if (global_alloc == nullptr) {
        CloseClipboard();
        return 0;
    }

    const auto global_lock = GlobalLock(global_alloc);
    if (global_lock == nullptr) {
        CloseClipboard( );
        GlobalFree(global_alloc);
        return 0;
    }

    memcpy(global_lock, value.c_str(), value.size() + 1);

    GlobalUnlock(global_alloc);
    SetClipboardData(CF_TEXT, global_alloc);

    CloseClipboard();
    GlobalFree(global_alloc);

    return 0;
}

int setrbxclipboard(lua_State *L) {
    lua_check(L, 1);

    luaL_checktype(L, 1, LUA_TSTRING);
    const std::string value = lua_tostring(L, 1);

    if (!OpenClipboard(nullptr))
        return 0;

    EmptyClipboard( );

    const std::uintptr_t format = RegisterClipboardFormat(OBF("application/x-roblox-studio"));
    if (format == 0)  {
        CloseClipboard();
        return 0;
    }

    const auto global_alloc = GlobalAlloc(GMEM_MOVEABLE, value.size() + 1);
    if (global_alloc == nullptr) {
        CloseClipboard();
        return 0;
    }


    const auto global_lock = GlobalLock(global_alloc);
    if (global_lock == nullptr) {
        CloseClipboard( );
        GlobalFree(global_alloc);
        return 0;
    }

    memcpy(global_lock, value.c_str(), value.size() + 1);

    GlobalUnlock(global_alloc);
    SetClipboardData(format, global_alloc);

    CloseClipboard();
    GlobalFree(global_alloc);

    return 0;
}

int environment::get_objects(lua_State* L) {
    lua_check(L, 2);
    luaL_checktype(L, 1, LUA_TUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);

    lua_getglobal(L, OBF("game"));
    lua_getfield(L, -1, OBF("GetService"));
    lua_pushvalue(L, -2);
    lua_pushstring(L, OBF("InsertService"));
    lua_call(L, 2, 1);
    lua_remove(L, -2);

    lua_getfield(L, -1, OBF("LoadLocalAsset"));

    lua_pushvalue(L, -2);
    lua_pushvalue(L, 2);
    lua_pcall(L, 2, 1, 0);

    if (lua_type(L, -1) == LUA_TSTRING) {
        luaL_error(L, lua_tostring(L, -1));
    }

    lua_createtable(L, 1, 0);
    lua_pushvalue(L, -2);
    lua_rawseti(L, -2, 1);

    lua_remove(L, -3);
    lua_remove(L, -2);

    return 1;
}

int gethui(lua_State *L)
{
    lua_check(L, 0);

    if (!L->userdata->actor.expired())
    {
        luaC_threadbarrier(L);
        lua_rawcheckstack(L, 1);

        lua_rawgetfield(globals::our_state, LUA_REGISTRYINDEX, OBF("hidden_ui_container"));
        lua_xmove(globals::our_state, L, 1);

        lua_rawgetfield(L, LUA_REGISTRYINDEX, OBF("Instance"));
        lua_setmetatable(L, -2);
        return 1;    
    }

    luaC_threadbarrier(L);
    lua_rawcheckstack(L, 1);

    lua_rawgetfield(L, LUA_REGISTRYINDEX, OBF("hidden_ui_container"));
    return 1;
}

extern std::optional<const std::string> get_hwid();

int gethwid(lua_State *L) {
    lua_check(L, 0);

    const std::optional<const std::string> hwid = get_hwid();

    lua_pushstring(L, hwid.value_or(OBF("no hwid")).c_str());
    return 1;
}

int queue_on_teleport(lua_State *L) {
    lua_check(L, 1);
    luaL_checktype(L, 1, LUA_TSTRING);

    std::string code = lua_tostring(L, 1);

    g_environment->teleport_queue.emplace_back(code);
    return 0;
}

int messagebox(lua_State* L) {
    lua_check(L, 3);
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TNUMBER);

    LPCSTR text = lua_tostring(L, 1);
    LPCSTR caption = lua_tostring(L, 2);
    int messagebox_type = lua_tonumber(L, 3);

    return g_yielding->yield(L, [text, caption, messagebox_type]() -> yielded_func_t {
        int ret = MessageBoxA(nullptr, text, caption, messagebox_type);

        return [ret](lua_State *thread) -> int {
            lua_pushnumber(thread, ret);

            return 1;
        };
    });
};

void environment::load_misc_lib(lua_State *L) {
    lua_pushcclosure(L, identifyexecutor, nullptr, 0);
    lua_setglobal(L, "identifyexecutor");

    lua_pushcclosure(L, identifyexecutor, nullptr, 0);
    lua_setglobal(L, "getexecutorname");

    lua_pushcclosure(L, getgenv, nullptr, 0);
    lua_setglobal(L, "getgenv");

    lua_pushcclosure(L, getrenv, nullptr, 0);
    lua_setglobal(L, "getrenv");

    lua_pushcclosure(L, gettenv, nullptr, 0);
    lua_setglobal(L, "gettenv");

    lua_pushcclosure(L, getreg, nullptr, 0);
    lua_setglobal(L, "getreg");

    lua_pushcclosure(L, getgc, nullptr, 0);
    lua_setglobal(L, "getgc");

    lua_pushcclosure(L, getfunctionhash, nullptr, 0);
    lua_setglobal(L, "getfunctionhash");

    lua_pushcclosure(L, filtergc, nullptr, 0);
    lua_setglobal(L, "filtergc");

    lua_pushcclosure(L, getidentity, nullptr, 0);
    lua_setglobal(L, "getthreadidentity");

    lua_pushcclosure(L, setidentity, nullptr, 0);
    lua_setglobal(L, "setthreadidentity");

    lua_pushcclosure(L, getidentity, nullptr, 0);
    lua_setglobal(L, "getthreadcontext");

    lua_pushcclosure(L, setidentity, nullptr, 0);
    lua_setglobal(L, "setthreadcontext");

    lua_pushcclosure(L, getidentity, nullptr, 0);
    lua_setglobal(L, "getidentity");

    lua_pushcclosure(L, setidentity, nullptr, 0);
    lua_setglobal(L, "setidentity");

    lua_pushcclosure(L, setidentity, nullptr, 0);
    lua_setglobal(L, "set_thread_identity");

    lua_pushcclosure(L, getidentity, nullptr, 0);
    lua_setglobal(L, "get_thread_identity");

    lua_pushcclosure(L, getfpscap, nullptr, 0);
    lua_setglobal(L, "getfpscap");

    lua_pushcclosure(L, setfpscap, nullptr, 0);
    lua_setglobal(L, "setfpscap");

    lua_pushcclosure(L, setclipboard, nullptr, 0);
    lua_setglobal(L, "setclipboard");

    lua_pushcclosure(L, setclipboard, nullptr, 0);
    lua_setglobal(L, "toclipboard");

    lua_pushcclosure(L, setrbxclipboard, nullptr, 0);
    lua_setglobal(L, "setrbxclipboard");

    lua_pushcclosure(L, gethui, nullptr, 0);
    lua_setglobal(L, "gethui");

    lua_pushcclosure(L, gethwid, nullptr, 0);
    lua_setglobal(L, "gethwid");

    lua_pushcclosure(L, messagebox, nullptr, 0);
    lua_setglobal(L, "messagebox");

    lua_pushcclosure(L, queue_on_teleport, nullptr, 0);
    lua_setglobal(L, "queueonteleport");
    lua_pushcclosure(L, queue_on_teleport, nullptr, 0);
    lua_setglobal(L, "queue_on_teleport");

}
