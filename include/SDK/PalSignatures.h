#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>

namespace Palworld {
    class SignatureManager {
    public:
        static void Initialize();
        
        // Expected parameter format: [CLASS]::[FUNCTION] or [FUNCTION], for example AGameModeBase::InitGameState or AsyncTask
        static void* GetSignature(const std::string& ClassAndFunction);
    private:
        static inline std::unordered_map<std::string, void*> SignatureMap;

        static inline std::unordered_map<std::string, std::string> Signatures {
            // Blueprint Loader apply logic
            { "UBlueprintGeneratedClass::PostLoadDefaultObject", "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8D 99 58 03 00 00" },
            // Raw Table apply logic
            { "FPakPlatformFile::GetPakFolders", "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 4C 89 74 24 20 55 48 8B EC 48 83 EC 40 48 8D 4D F0 48 8B DA E8" },
            // Important so we can easily run things on the Game Thread
            { "AsyncTask", "48 8B C4 41 54 41 57 48 81 EC B8 00 00 00 48 89 58 08" },
            // Used to initialize any other loader logic like pals, items, etc
            { "AGameModeBase::InitGameState", "40 53 48 83 EC 20 48 8B 41 10 48 8B D9 48 8B 91 F0 02 00 00" },
            { "UClass::AssembleReferenceTokenStream", "48 8B C4 55 56 48 8D 68 A1 48 81 EC A8 00 00 00 48 89 58 10 48 8D B1 F8 01 00 00" },
            { "UClass::GetDefaultObject", "40 53 48 83 EC 20 48 83 B9 10 01 00 00 00 48 8B D9 75 1A 84 D2 74 16 48 8B 01" },
            { "UStruct::StaticLink", "48 89 5C 24 08 57 48 81 EC C0 00 00 00 48 8B F9 0F B6 DA 48 8D 4C 24 20" },
            // UE4SS has StaticFindObject, but this lets us use it earlier.
            { "UObjectGlobals::StaticFindObject", "48 8B C4 48 89 58 08 48 89 68 18 48 89 70 20 57 41 56 41 57 48 83 EC 60 48 83 FA FF" },
            // I had issues using the IsA provided by UE4SS due to early init, so I switched to using Unreal's own.
            { "FField::IsA", "48 8B 41 08 48 8B 4A 08 48 85 C9 74 08 48 85 48 10 0F 95 C0 C3" },
            // Important, we need this early.
            { "FName::Constructor", "48 89 5C 24 08 57 48 83 EC 30 48 8B D9 48 89 54 24 20" },
            // Points towards the MOV instruction in UPalItemContainer::ApplySaveData after the CALL to UPalItemSlot::UpdateItem_ServerInternal
            { "UPalItemContainer::ApplySaveData", "48 8B CB E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 48 8D 4B 28" },
            // Points towards the MOV instruction in UPalDynamicItemWorldSubsystem::ApplyWorldSaveData after the CALL to UPalDynamicItemWorldSubsystem::Create_ServerInternal
            { "UPalDynamicItemWorldSubsystem::ApplyWorldSaveData", "48 8B 4C 24 60 48 8B D8 48 85 C9 74 05 E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 4C 89 65 50" },
        };
        static inline std::unordered_map<std::string, std::string> SignaturesCallResolve {
            // Don't ask, I know it's long..
            { "FFieldClass::GetNameToFieldClassMap", "E8 ?? ?? ?? ?? 41 B8 01 00 00 00 48 8D ?? ?? ?? ?? ?? 48 8D 4C 24 40 48 8B F8 E8 ?? ?? ?? ?? 48 8B 4C 24 40 48 8B D9 48 C1 EB 20 E8 ?? ?? ?? ?? 4C 8D 44 24 40 48 8B CF 8D 14 03 E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 48 89 30 E8" },
            // Important, we need this early.
            { "FName::ToString_Wchar", "E8 ?? ?? ?? ?? 45 8B F7 90 45 8B C6 48 8D 54 24 40" },
            // More efficient object lookup for certain cases.
            { "GetObjectsOfClass", "E8 ?? ?? ?? ?? 45 33 C0 48 89 6C 24 30 33 C9 41 8D 50 30 E8" },
            // FMemory::Free for GMalloc
            { "FMemory::Free", "E8 ?? ?? ?? ?? 4C 8D 45 D7 48 8D 55 27 48 8D 4D 17 E8" },
            // Raw Tables
            { "UDataTable::Serialize", "E8 ?? ?? ?? ?? 41 F6 06 01 0F 84 1D 03 00 00 48 89 5C 24 50" },
            { "UPalDynamicItemWorldSubsystem::Create_ServerInternal", "E8 ?? ?? ?? ?? 48 8B 4C 24 60 48 8B D8 48 85 C9 74 05 E8 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 4C 89 65 50 48 8D ?? ?? ?? ?? ?? 48 89" },
            { "UPalItemSlot::UpdateItem_ServerInternal", "E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 48 8D 4B 28" },
            { "UWorld::CleanupWorld", "E8 ?? ?? ?? ?? 8B 55 A7 FF C2 49 83 C5 08 89 55 A7" },
        };
    };
}