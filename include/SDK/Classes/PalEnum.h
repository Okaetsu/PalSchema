#pragma once

#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

namespace Palworld {
    enum EPropertyType : RC::Unreal::uint8
    {
        CPT_None,
        CPT_Byte,
        CPT_UInt16,
        CPT_UInt32,
        CPT_UInt64,
        CPT_Int8,
        CPT_Int16,
        CPT_Int,
        CPT_Int64,
        CPT_Bool,
        CPT_Bool8,
        CPT_Bool16,
        CPT_Bool32,
        CPT_Bool64,
        CPT_Float,
        CPT_ObjectReference,
        CPT_Name,
        CPT_Delegate,
        CPT_Interface,
        CPT_Unused_Index_19,
        CPT_Struct,
        CPT_Unused_Index_21,
        CPT_Unused_Index_22,
        CPT_String,
        CPT_Text,
        CPT_MulticastDelegate,
        CPT_WeakObjectReference,
        CPT_LazyObjectReference,
        CPT_ObjectPtrReference,
        CPT_SoftObjectReference,
        CPT_Double,
        CPT_Map,
        CPT_Set,
        CPT_FieldPath,
        CPT_FLargeWorldCoordinatesReal,

        CPT_MAX
    };

    class UPalEnum : public RC::Unreal::UEnum
    {
    public:
        EPropertyType GetUnderlyingType();
    };
}