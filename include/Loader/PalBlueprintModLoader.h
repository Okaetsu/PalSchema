#pragma once

#include "Loader/PalModLoaderBase.h"
#include "Loader/Blueprint/PalBlueprintMod.h"
#include "Unreal/NameTypes.hpp"
#include "Unreal/UObjectArray.hpp"
#include "safetyhook.hpp"
#include <unordered_map>

namespace UECustom {
    class UBlueprintGeneratedClass;
}

namespace Palworld {
    class PalBlueprintModLoader : public PalModLoaderBase {
    public:
        PalBlueprintModLoader();

        ~PalBlueprintModLoader();

        void OnPostLoadDefaultObject(RC::Unreal::UClass* This, RC::Unreal::UObject* DefaultObject);
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
    private:
        std::unordered_map<RC::StringType, std::vector<PalBlueprintMod>> BPModRegistry;

        // Does not call UE functions, therefore safe to call whenever
        void LoadSafe(const nlohmann::json& data);

        // Calls UE functions, make sure UE is ready
        void LoadUnsafe(const nlohmann::json& data);

        std::vector<PalBlueprintMod>& GetModsForBlueprint(const RC::StringType& Name);
        
        void ApplyMod(const PalBlueprintMod& BPMod, RC::Unreal::UObject* Object);

        void ApplyMod(const nlohmann::json& Data, RC::Unreal::UObject* Object);

        void HandleInheritableComponent(UECustom::UBlueprintGeneratedClass* BPClass, const RC::StringType& ComponentName, const nlohmann::json& ComponentData);

        void HandleNodeComponent(UECustom::UBlueprintGeneratedClass* BPClass, const RC::StringType& ComponentName, const nlohmann::json& ComponentData);

        void ModifyComponent(RC::Unreal::UObject* Component, const nlohmann::json& ComponentData);
    private:
        static inline SafetyHookInline PostLoadHook;
        static inline std::function<void(RC::Unreal::UClass*)> PostLoadCallback = nullptr;
        static void PostLoad(RC::Unreal::UClass* This);
    };
}