#include <regex>
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/UObject.hpp"
#include "Unreal/AActor.hpp"
#include "Helpers/String.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "Utility/JsonHelpers.h"
#include "Loader/PalBlueprintModLoader.h"
#include "SDK/Classes/KismetSystemLibrary.h"
#include "SDK/Helper/BPGeneratedClassHelper.h"
#include "SDK/Helper/Memory.h"
#include "SDK/Classes/Custom/UBlueprintGeneratedClass.h"
#include "SDK/Classes/Custom/UInheritableComponentHandler.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    PalBlueprintModLoader::PalBlueprintModLoader() : PalModLoaderBase("blueprints")
    {
        SetDisplayName(TEXT("Blueprint Mod Loader"));
    }

    PalBlueprintModLoader::~PalBlueprintModLoader()
    {
        auto expected = PostLoadHook.disable();
        PostLoadHook = {};
        PostLoadCallback = nullptr;
        BPModRegistry.clear();
    }

    void PalBlueprintModLoader::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::PostEngineInit)
        {
            PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
                LoadSafe(data);
            });
        }
        else if (engineLifecyclePhase == EEngineLifecyclePhase::GameInstanceInit)
        {
            PS::JsonHelpers::ParseJsonFilesInPath(loaderPath, [&](const nlohmann::json& data) {
                LoadUnsafe(data);
            });
        }
    }

    void PalBlueprintModLoader::OnAutoReload(const std::filesystem::path::string_type& modName, const std::filesystem::path& modFilePath)
    {
        PS::JsonHelpers::ParseJsonFileInPath(modFilePath, [&](const nlohmann::json& data) {
            LoadUnsafe(data);
        });
    }

    bool PalBlueprintModLoader::CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        if (engineLifecyclePhase == EEngineLifecyclePhase::PostEngineInit)
        {
            return true;
        }

        return false;
    }

    void PalBlueprintModLoader::OnPostLoadDefaultObject(RC::Unreal::UClass* This, RC::Unreal::UObject* DefaultObject)
    {
        if (!DefaultObject) return;

        auto BPName = This->GetNamePrivate().ToString();
        if (Palworld::PalBlueprintModLoader::BPModRegistry.contains(BPName))
        {
            auto& Mods = PalBlueprintModLoader::GetModsForBlueprint(BPName);
            for (auto& Mod : Mods)
            {
                try
                {
                    PalBlueprintModLoader::ApplyMod(Mod, DefaultObject);
                    PS::Log<RC::LogLevel::Normal>(TEXT("Applied modifications to {}\n"), BPName);
                }
                catch (const std::exception& e)
                {
                    PS::Log<RC::LogLevel::Error>(TEXT("Failed modifying blueprint '{}', {}\n"), BPName, RC::to_generic_string(e.what()));
                }
            }
        }
    }

    bool PalBlueprintModLoader::OnInitialize()
    {
        // Should in theory be more consistent than finding a signature for BlueprintGeneratedClass::PostLoad
        auto vtablePtr = Palworld::GetVTablePtrByClassPath(TEXT("/Script/Engine.BlueprintGeneratedClass"));
        if (!vtablePtr)
        {
            PS::Log<LogLevel::Error>(TEXT("Something went wrong getting VTable pointer for BlueprintGeneratedClass. Cannot hook PostLoad which means blueprint mods will not function.\n"));
            return false;
        }

        void* postloadPtr = Palworld::GetVirtualFunctionFromVTable(vtablePtr, 20);
        PS::Log<LogLevel::Verbose>(TEXT("Found UBlueprintGeneratedClass::PostLoad: {}\n"), postloadPtr);

        PostLoadCallback = [&](UClass* BPGeneratedClass) {
            OnPostLoadDefaultObject(BPGeneratedClass, BPGeneratedClass->GetClassDefaultObject());
        };

        PostLoadHook = safetyhook::create_inline(postloadPtr,
            reinterpret_cast<void*>(PostLoad));

        return true;
    }

    void PalBlueprintModLoader::LoadSafe(const nlohmann::json& data)
    {
        for (auto& [BlueprintName, BlueprintData] : data.items())
        {
            auto BlueprintName_Conv = RC::to_generic_string(BlueprintName);
            if (!BlueprintName_Conv.starts_with(TEXT("/Game/")))
            {
                auto NewBlueprintMod = PalBlueprintMod(BlueprintName, BlueprintData);
                auto ModsIt = BPModRegistry.find(BlueprintName_Conv);
                if (ModsIt != BPModRegistry.end())
                {
                    BPModRegistry.at(BlueprintName_Conv).push_back(NewBlueprintMod);
                }
                else
                {
                    auto NewModContainer = std::vector<PalBlueprintMod>{
                        NewBlueprintMod
                    };
                    BPModRegistry.emplace(BlueprintName_Conv, NewModContainer);
                }
            }
        }
    }

    void PalBlueprintModLoader::LoadUnsafe(const nlohmann::json& data)
    {
        for (auto& [BlueprintName, BlueprintData] : data.items())
        {
            auto BlueprintNameWide = RC::to_generic_string(BlueprintName);
            if (BlueprintNameWide.starts_with(TEXT("/Game/")))
            {
                static const std::wregex Pattern(LR"(^(.*/)([^/.]+)$)");
                BlueprintNameWide = std::regex_replace(BlueprintNameWide, Pattern, TEXT("$1$2.$2_C"));

                auto SoftObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(BlueprintNameWide));
                auto Asset = UECustom::UKismetSystemLibrary::LoadAsset_Blocking(SoftObjectPtr);
                if (!Asset)
                {
                    throw std::runtime_error(RC::fmt("Failed to apply blueprint changes, asset '%S' was invalid", BlueprintNameWide.c_str()));
                }

                Asset->SetRootSet();

                auto DefaultObject = static_cast<UClass*>(Asset)->GetClassDefaultObject();
                ApplyMod(BlueprintData, DefaultObject);

                PS::Log<RC::LogLevel::Normal>(TEXT("Applied changes to {}\n"), static_cast<UClass*>(Asset)->GetNamePrivate().ToString());
            }
        }
    }

    std::vector<PalBlueprintMod>& PalBlueprintModLoader::GetModsForBlueprint(const RC::StringType& Name)
    {
        auto Iterator = BPModRegistry.find(Name);
        if (Iterator != BPModRegistry.end())
        {
            return Iterator->second;
        }

        throw std::runtime_error(RC::fmt("Failed to get mods for this blueprint from BPModRegistry. Affected mod name: %S", Name.c_str()));
    }

    void PalBlueprintModLoader::ApplyMod(const PalBlueprintMod& BPMod, UObject* Object)
    {
        auto& Data = BPMod.GetData();
        ApplyMod(Data, Object);
    }

    void PalBlueprintModLoader::ApplyMod(const nlohmann::json& Data, RC::Unreal::UObject* Object)
    {
        auto Class = static_cast<UECustom::UBlueprintGeneratedClass*>(Object->GetClassPrivate());

        for (auto& [PropertyName, PropertyValue] : Data.items())
        {
            auto PropertyNameWide = RC::to_generic_string(PropertyName);
            auto Property = Palworld::PropertyHelper::GetPropertyByName(Class, PropertyNameWide);
            
            if (!Property)
            {
                PS::Log<RC::LogLevel::Warning>(TEXT("Property '{}' does not exist in {}\n"), PropertyNameWide, Class->GetNamePrivate().ToString());
                continue;
            }

            if (auto ObjectProperty = CastField<FObjectProperty>(Property))
            {
                auto ObjectValue = *Property->ContainerPtrToValuePtr<UObject*>(Object);
                if (!ObjectValue)
                {
                    // null Object means that this property could be a component template, so we should check if it has an associated GEN_VARIABLE.
                    HandleInheritableComponent(Class, PropertyNameWide, PropertyValue);
                }
                else
                {
                    // Object has a pointer assigned to it so we let PropertyHelper handle it.
                    PropertyHelper::CopyJsonValueToContainer(Object, Property, PropertyValue);
                }
            }
            else
            {
                // Any other property values get handled here like Numeric, Bool, String, etc.
                PropertyHelper::CopyJsonValueToContainer(Object, Property, PropertyValue);
            }
        }
    }

    void PalBlueprintModLoader::HandleInheritableComponent(UECustom::UBlueprintGeneratedClass* BPClass, const RC::StringType& ComponentName,
                                                         const nlohmann::json& ComponentData)
    {
        auto BPClassName = BPClass->GetNamePrivate().ToString();

        if (!ComponentData.is_object())
        {
            PS::Log<LogLevel::Warning>(TEXT("{} failed to apply, provided JSON value wasn't an object\n"), BPClassName);
            return;
        }

        auto ComponentFullName = std::format(TEXT("{}_GEN_VARIABLE"), ComponentName);
        UObject* InheritableComponent = nullptr;

        auto InheritableComponentHandler = BPClass->GetInheritableComponentHandler();
        if (InheritableComponentHandler)
        {
            auto Records = InheritableComponentHandler->GetRecords();
            for (auto& Record : Records)
            {
                if (Record.ComponentTemplate.Get() == nullptr) continue;

                if (Record.ComponentTemplate.Get()->GetName() == ComponentFullName)
                {
                    InheritableComponent = Record.ComponentTemplate.Get();
                    break;
                }
            }
        }

        if (InheritableComponent)
        {
            ModifyComponent(InheritableComponent, ComponentData);
            return;
        }

        // Component wasn't inside Inheritable Components list, so check SimpleConstructionScript next.
        HandleNodeComponent(BPClass, ComponentFullName, ComponentData);
    }

    void PalBlueprintModLoader::HandleNodeComponent(UECustom::UBlueprintGeneratedClass* BPClass, const RC::StringType& ComponentName, const nlohmann::json& ComponentData)
    {
        auto SimpleConstructionScript = BPClass->GetSimpleConstructionScript();
        if (!SimpleConstructionScript)
        {
            return;
        }

        UObject* NodeComponent = nullptr;

        auto Nodes = SimpleConstructionScript->GetAllNodes();
        for (auto& NodeElement : Nodes)
        {
            auto NodeComponentTemplate = NodeElement->GetComponentTemplate();
            if (!NodeComponentTemplate)
            {
                continue;
            }

            if (NodeComponentTemplate->GetName() == ComponentName)
            {
                NodeComponent = NodeComponentTemplate;
                break;
            }
        }

        if (!NodeComponent)
        {
            return;
        }

        ModifyComponent(NodeComponent, ComponentData);
    }

    void PalBlueprintModLoader::ModifyComponent(RC::Unreal::UObject* Component, const nlohmann::json& ComponentData)
    {
        int SuccessfulChanges = 0;
        for (auto& [InnerKey, InnerValue] : ComponentData.items())
        {
            auto ComponentPropertyName = RC::to_generic_string(InnerKey);
            auto ComponentProperty = PropertyHelper::GetPropertyByName(Component->GetClassPrivate(), ComponentPropertyName.c_str());
            if (!ComponentProperty)
            {
                PS::Log<LogLevel::Warning>(TEXT("Property {} doesn't exist in {}\n"), ComponentPropertyName, Component->GetName());
                continue;
            }

            PropertyHelper::CopyJsonValueToContainer(Component, ComponentProperty, InnerValue);
            SuccessfulChanges++;
        }

        if (SuccessfulChanges > 0)
        {
            PS::Log<LogLevel::Normal>(TEXT("Applied changes to {}\n"), Component->GetName());
        }
        else
        {
            PS::Log<LogLevel::Normal>(TEXT("No changes were made to {}\n"), Component->GetName());
        }
    }

    void PalBlueprintModLoader::PostLoad(RC::Unreal::UClass* This)
    {
        PostLoadHook.call(This);

        if (!PostLoadCallback)
        {
            return;
        }

        PostLoadCallback(This);
    }
}