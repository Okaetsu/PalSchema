#include "SDK/Classes/Property/PalEnumProperty.h"
#include "SDK/Classes/PalEnum.h"

using namespace RC::Unreal;

namespace Palworld {
    RC::Unreal::int64 FPalEnumProperty::GetPropertyValue(void* Container)
    {
        UPalEnum* Enum = static_cast<UPalEnum*>(GetEnum());
        EPropertyType UnderlyingType = Enum->GetUnderlyingType();

        switch (UnderlyingType)
        {
        case Palworld::CPT_Byte:
            return *ContainerPtrToValuePtr<uint8>(Container);
        case Palworld::CPT_UInt16:
            return *ContainerPtrToValuePtr<uint16>(Container);
        case Palworld::CPT_UInt32:
            return *ContainerPtrToValuePtr<uint32>(Container);
        case Palworld::CPT_UInt64:
            return *ContainerPtrToValuePtr<uint64>(Container);
        case Palworld::CPT_Int8:
            return *ContainerPtrToValuePtr<int8>(Container);
        case Palworld::CPT_Int16:
            return *ContainerPtrToValuePtr<int16>(Container);
        case Palworld::CPT_Int:
            return *ContainerPtrToValuePtr<int32>(Container);
        case Palworld::CPT_Int64:
            return *ContainerPtrToValuePtr<int64>(Container);
        default:
            // Should never happen, but throw anyway to make debugging future issues easier.
            throw std::runtime_error(RC::fmt("Enum '%S' had unsupported underlying type.", GetName().c_str()));
        }
    }

    UPalEnum* FPalEnumProperty::GetPalEnum()
    {
        return static_cast<UPalEnum*>(GetEnum());
    }
}