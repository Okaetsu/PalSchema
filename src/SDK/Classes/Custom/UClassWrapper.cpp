#include "SDK/Classes/Custom/UClassWrapper.h"
#include "SDK/Helper/Memory.h"
#include "SDK/PalSignatures.h"
#include "Utility/Logging.h"

using namespace Palworld;
using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    RC::Unreal::UObject* UClassWrapper::GetDefaultObject(bool bCreateIfNeeded)
    {
        using FnSignature = RC::Unreal::UObject* (*)(UClass*, bool);
        static FnSignature fn = nullptr;

        if (!fn)
        {
            fn = reinterpret_cast<FnSignature>(
                Palworld::SignatureManager::GetSignature("UClass::GetDefaultObject")
            );
        }

        if (!fn)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call UClass::GetDefaultObject because function address was invalid.\n"));
            return nullptr;
        }

        return fn(this, bCreateIfNeeded);
    }

    void UClassWrapper::AssembleReferenceTokenStream(bool bForce)
    {
        using FnSignature = void(*)(UClass*, bool);
        static FnSignature fn = nullptr;

        if (!fn)
        {
            fn = reinterpret_cast<FnSignature>(
                Palworld::SignatureManager::GetSignature("UClass::AssembleReferenceTokenStream")
            );
        }

        if (!fn)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call UClass::AssembleReferenceTokenStream because function address was invalid.\n"));
            return;
        }

        fn(this, bForce);
    }

    void UClassWrapper::StaticLink(bool bRelinkExistingProperties)
    {
        using FnSignature = void(*)(UClass*, bool);
        static FnSignature fn = nullptr;

        if (!fn)
        {
            fn = reinterpret_cast<FnSignature>(
                Palworld::SignatureManager::GetSignature("UStruct::StaticLink")
            );
        }

        if (!fn)
        {
            PS::Log<LogLevel::Error>(STR("Failed to call UStruct::StaticLink because function address was invalid.\n"));
            return;
        }

        fn(this, bRelinkExistingProperties);
    }
}