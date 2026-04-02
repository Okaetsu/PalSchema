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
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const std::filesystem::path::string_type& modName, const std::filesystem::path& modFilePath) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
    private:
        std::unordered_map<RC::Unreal::FName, std::vector<PalBlueprintMod>> m_modsMap;

        bool HookPostLoad();
        bool HookPostInitComponents();

        // Does not call UE functions, therefore safe to call whenever
        void LoadSafe(const nlohmann::json& data);

        // Calls UE functions, make sure UE is ready
        void LoadUnsafe(const nlohmann::json& data);

        std::vector<PalBlueprintMod>& GetModsForBlueprint(const RC::Unreal::FName& name);

        void ModifyObject(RC::Unreal::UObject* object);
        
        void ApplyMod(const PalBlueprintMod& mod, RC::Unreal::UObject* object);

        void ApplyData(const nlohmann::json& data, RC::Unreal::UObject* object);

        void HandleInheritableComponent(UECustom::UBlueprintGeneratedClass* bpClass, const RC::StringType& componentName, const nlohmann::json& componentData);

        void HandleNodeComponent(UECustom::UBlueprintGeneratedClass* bpClass, const RC::StringType& componentName, const nlohmann::json& componentData);

        void ModifyComponent(RC::Unreal::UObject* component, const nlohmann::json& componentData);
    private:
        static inline SafetyHookInline PostLoadHook;
        static inline std::function<void(RC::Unreal::UClass*)> PostLoadCallback = nullptr;
        static void PostLoad(RC::Unreal::UClass* self);

        static inline SafetyHookInline PostInitComponentsHook;
        static inline std::function<void(RC::Unreal::AActor*)> PostInitComponentsCallback = nullptr;
        static void PostInitComponents(RC::Unreal::AActor* self);
    };
}