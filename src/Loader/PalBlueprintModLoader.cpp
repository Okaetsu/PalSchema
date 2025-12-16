#include <regex>
#include "Unreal/CoreUObject/UObject/Class.hpp"
#include "Unreal/UObject.hpp"
#include "Unreal/AActor.hpp"
#include "Unreal/Property/FObjectProperty.hpp"
#include "Helpers/String.hpp"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "Loader/PalBlueprintModLoader.h"
#include "SDK/Classes/KismetSystemLibrary.h"
#include "SDK/Helper/BPGeneratedClassHelper.h"
#include "SDK/Classes/Custom/UBlueprintGeneratedClass.h"
#include "SDK/Classes/Custom/UInheritableComponentHandler.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    PalBlueprintModLoader::PalBlueprintModLoader() : PalModLoaderBase("blueprints")
    {
    }

    PalBlueprintModLoader::~PalBlueprintModLoader()
    {
        BPModRegistry.clear();
    }

    void PalBlueprintModLoader::Load(const nlohmann::json& Data)
    {
        for (auto& [BlueprintName, BlueprintData] : Data.items())
        {
            auto BlueprintName_Conv = RC::to_generic_string(BlueprintName);
            if (BlueprintName_Conv.starts_with(STR("/Game/")))
            {
                static const std::wregex Pattern(LR"(^(.*/)([^/.]+)$)");
                BlueprintName_Conv = std::regex_replace(BlueprintName_Conv, Pattern, STR("$1$2.$2_C"));

                auto SoftObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(BlueprintName_Conv));
                auto Asset = UECustom::UKismetSystemLibrary::LoadAsset_Blocking(SoftObjectPtr);
                if (!Asset)
                {
                    throw std::runtime_error(RC::fmt("Failed to apply blueprint changes, asset '%S' was invalid", BlueprintName_Conv.c_str()));
                }

                Asset->SetRootSet();

                auto DefaultObject = static_cast<UClass*>(Asset)->GetClassDefaultObject();
                ApplyMod(BlueprintData, DefaultObject);

                PS::Log<RC::LogLevel::Normal>(STR("Applied changes to {}\n"), static_cast<UClass*>(Asset)->GetNamePrivate().ToString());
            }
        }
    }

    void PalBlueprintModLoader::LoadSafe(const nlohmann::json& Data)
    {
        for (auto& [BlueprintName, BlueprintData] : Data.items())
        {
            auto BlueprintName_Conv = RC::to_generic_string(BlueprintName);
            if (!BlueprintName_Conv.starts_with(STR("/Game/")))
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

    void PalBlueprintModLoader::Initialize()
    {
        
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
                    PS::Log<RC::LogLevel::Normal>(STR("Applied modifications to {}\n"), BPName);
                }
                catch (const std::exception& e)
                {
                    PS::Log<RC::LogLevel::Error>(STR("Failed modifying blueprint '{}', {}\n"), BPName, RC::to_generic_string(e.what()));
                }
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
                PS::Log<RC::LogLevel::Warning>(STR("Property '{}' does not exist in {}\n"), PropertyNameWide, Class->GetNamePrivate().ToString());
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
            PS::Log<LogLevel::Warning>(STR("{} failed to apply, provided JSON value wasn't an object\n"), BPClassName);
            return;
        }

        auto ComponentFullName = std::format(STR("{}_GEN_VARIABLE"), ComponentName);
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
                PS::Log<LogLevel::Warning>(STR("Property {} doesn't exist in {}\n"), ComponentPropertyName, Component->GetName());
                continue;
            }

            PropertyHelper::CopyJsonValueToContainer(Component, ComponentProperty, InnerValue);
            SuccessfulChanges++;
        }

        if (SuccessfulChanges > 0)
        {
            PS::Log<LogLevel::Normal>(STR("Applied changes to {}\n"), Component->GetName());
        }
        else
        {
            PS::Log<LogLevel::Normal>(STR("No changes were made to {}\n"), Component->GetName());
        }
    }
}