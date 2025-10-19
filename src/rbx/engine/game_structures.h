//
// Created by savage on 17.04.2025.
//

#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#define __fastcall
#endif
#include <vector>
struct lua_State;
struct Proto;
struct global_State;
struct LuaTable;

namespace rbx {

    struct weak_thread_ref_t {
        int _refs;
        lua_State *thread;
        int32_t thread_ref;
        int32_t object_id;
    };

    namespace Security {
        enum Permissions : std::uint32_t;
    }

    template<typename Derived, typename Base>
    concept TypeConstraint = std::is_base_of_v<Base, Derived>;

    namespace reflection { // ud rbxstu pasta
        enum ReflectionType : uint32_t {
            ReflectionType_Void = 0x0,
            ReflectionType_Bool = 0x1,
            ReflectionType_Int = 0x2,
            ReflectionType_Int64 = 0x3,
            ReflectionType_Float = 0x4,
            ReflectionType_Double = 0x5,
            ReflectionType_String = 0x6,
            ReflectionType_ProtectedString = 0x7,
            ReflectionType_Instance = 0x8,
            ReflectionType_Instances = 0x9,
            ReflectionType_Ray = 0xa,
            ReflectionType_Vector2 = 0xb,
            ReflectionType_Vector3 = 0xc,
            ReflectionType_Vector2Int16 = 0xd,
            ReflectionType_Vector3Int16 = 0xe,
            ReflectionType_Rect2d = 0xf,
            ReflectionType_CoordinateFrame = 0x10,
            ReflectionType_Color3 = 0x11,
            ReflectionType_Color3uint8 = 0x12,
            ReflectionType_UDim = 0x13,
            ReflectionType_UDim2 = 0x14,
            ReflectionType_Faces = 0x15,
            ReflectionType_Axes = 0x16,
            ReflectionType_Region3 = 0x17,
            ReflectionType_Region3Int16 = 0x18,
            ReflectionType_CellId = 0x19,
            ReflectionType_GuidData = 0x1a,
            ReflectionType_PhysicalProperties = 0x1b,
            ReflectionType_BrickColor = 0x1c,
            ReflectionType_SystemAddress = 0x1d,
            ReflectionType_BinaryString = 0x1e,
            ReflectionType_Surface = 0x1f,
            ReflectionType_Enum = 0x20,
            ReflectionType_Property = 0x21,
            ReflectionType_Tuple = 0x22,
            ReflectionType_ValueArray = 0x23,
            ReflectionType_ValueTable = 0x24,
            ReflectionType_ValueMap = 0x25,
            ReflectionType_Variant = 0x26,
            ReflectionType_GenericFunction = 0x27,
            ReflectionType_WeakFunctionRef = 0x28,
            ReflectionType_ColorSequence = 0x29,
            ReflectionType_ColorSequenceKeypoint = 0x2a,
            ReflectionType_NumberRange = 0x2b,
            ReflectionType_NumberSequence = 0x2c,
            ReflectionType_NumberSequenceKeypoint = 0x2d,
            ReflectionType_InputObject = 0x2e,
            ReflectionType_Connection = 0x2f,
            ReflectionType_ContentId = 0x30,
            ReflectionType_DescribedBase = 0x31,
            ReflectionType_RefType = 0x32,
            ReflectionType_QFont = 0x33,
            ReflectionType_QDir = 0x34,
            ReflectionType_EventInstance = 0x35,
            ReflectionType_TweenInfo = 0x36,
            ReflectionType_DockWidgetPluginGuiInfo = 0x37,
            ReflectionType_PluginDrag = 0x38,
            ReflectionType_Random = 0x39,
            ReflectionType_PathWaypoint = 0x3a,
            ReflectionType_FloatCurveKey = 0x3b,
            ReflectionType_RotationCurveKey = 0x3c,
            ReflectionType_SharedString = 0x3d,
            ReflectionType_DateTime = 0x3e,
            ReflectionType_RaycastParams = 0x3f,
            ReflectionType_RaycastResult = 0x40,
            ReflectionType_OverlapParams = 0x41,
            ReflectionType_LazyTable = 0x42,
            ReflectionType_DebugTable = 0x43,
            ReflectionType_CatalogSearchParams = 0x44,
            ReflectionType_OptionalCoordinateFrame = 0x45,
            ReflectionType_CSGPropertyData = 0x46,
            ReflectionType_UniqueId = 0x47,
            ReflectionType_Font = 0x48,
            ReflectionType_Blackboard = 0x49,
            ReflectionType_Max = 0x4a
        };
    }

    struct descriptor_t // Base
    {
        void* vftable; // 0
        std::string& name; // 8

        // somewhere in here is Attributes
        void* unk; // 16
        void* unk1; // 24
        void* unk2; // 32

        char pad_0032[80];
        std::int64_t member;
    };

    struct rbx_type_t  : descriptor_t
    {

        std::string& tag; // 40
        int TypeID; // 48
        bool is_float; // 56
        bool is_primitive; // 57
        bool is_number; // 58
        bool is_enum; // 59
    };

    struct type_holder_t
    {
        void(*construct)(const char*, char*);
        void(*move_construct)(char*, char*);
        void(*destruct)(char*);
    };

    struct variant_t
    {
        struct storage_t
        {
            const type_holder_t* holder;
            const char data[64];
        };

        const rbx_type_t* _type;
        variant_t::storage_t value;
    };


    enum event_target_inclusion : std::int32_t
    {
        only_target = 0x0,
        exclude_target = 0x1,
    };


    struct shared_string_t {
        char pad_0[0x10];
        std::string content;
    };

    struct system_address_t {
        struct peerid {
            int peer_id;
        };

        peerid remote_id;
    };

    struct remoteevent_invocation_targetoptions_t
    {
        const system_address_t* target;
        event_target_inclusion isExcludeTarget;
    };

    enum message_type {
        message_output = 0,
        message_info = 1,
        message_warning = 2,
        message_error = 3
    };

}
