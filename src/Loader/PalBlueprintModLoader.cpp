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
        m_modsMap.clear();
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

    bool PalBlueprintModLoader::OnInitialize()
    {
        // PostLoad hook handles default objects for classes
        if (!HookPostLoad())
        {
            PS::Log<LogLevel::Error>(TEXT("Cannot hook UBlueprintGeneratedClass::PostLoad which means blueprint mods will not function properly.\n"));
            return false;
        }

        // PostInitComponents hook handles actor instances
        if (!HookPostInitComponents())
        {
            PS::Log<LogLevel::Error>(TEXT("Cannot hook AActor::PostInitComponents which means blueprint mods will not function properly.\n"));
            return false;
        }

        return true;
    }

    bool PalBlueprintModLoader::HookPostLoad()
    {
        auto vtable = Palworld::GetVTablePtrByClassPath(TEXT("/Script/Engine.BlueprintGeneratedClass"));
        if (!vtable)
        {
            PS::Log<LogLevel::Error>(STR("Something went wrong with getting VTable pointer for UBlueprintGeneratedClass."));
            return false;
        }

        void* postloadPtr = Palworld::GetVirtualFunctionFromVTable(vtable, 20);
        PS::Log<LogLevel::Verbose>(TEXT("Found UBlueprintGeneratedClass::PostLoad: {}\n"), postloadPtr);

        PostLoadCallback = [&](UClass* actorClass) {
            ModifyObject(actorClass->GetClassDefaultObject());
        };

        PostLoadHook = safetyhook::create_inline(postloadPtr,
            reinterpret_cast<void*>(PostLoad));

        return true;
    }

    bool PalBlueprintModLoader::HookPostInitComponents()
    {
        auto vtable = Palworld::GetVTablePtrByClassPath(TEXT("/Script/Engine.Actor"));
        if (!vtable)
        {
            PS::Log<LogLevel::Error>(TEXT("Something went wrong with getting VTable pointer for AActor.\n"));
            return false;
        }

        void* postInitCompsPtr = Palworld::GetVirtualFunctionFromVTable(vtable, 159);
        PS::Log<LogLevel::Verbose>(TEXT("Found AActor::PostInitializeComponents: {}\n"), postInitCompsPtr);

        PostInitComponentsCallback = [&](AActor* self) {
            ModifyObject(self);
        };

        PostInitComponentsHook = safetyhook::create_inline(postInitCompsPtr,
            reinterpret_cast<void*>(PostInitComponents));

        return true;
    }

    void PalBlueprintModLoader::LoadSafe(const nlohmann::json& data)
    {
        for (auto& [assetName, assetData] : data.items())
        {
            auto assetNameWide = RC::to_generic_string(assetName);
            if (!assetNameWide.starts_with(TEXT("/Game/")))
            {
                auto assetFName = FName(assetNameWide, FNAME_Add);
                auto newMod = PalBlueprintMod(assetFName, assetData);
                auto it = m_modsMap.find(assetFName);
                if (it != m_modsMap.end())
                {
                    m_modsMap.at(assetFName).push_back(newMod);
                }
                else
                {
                    auto newModContainer = std::vector<PalBlueprintMod>{
                        newMod
                    };
                    m_modsMap.emplace(assetFName, newModContainer);
                }

                PS::Log<LogLevel::Normal>(STR("Loaded changes to {}\n"), assetNameWide);
            }
        }
    }

    void PalBlueprintModLoader::LoadUnsafe(const nlohmann::json& data)
    {
        for (auto& [assetName, assetData] : data.items())
        {
            auto assetNameWide = RC::to_generic_string(assetName);
            if (assetNameWide.starts_with(TEXT("/Game/")))
            {
                static const std::wregex Pattern(LR"(^(.*/)([^/.]+)$)");
                assetNameWide = std::regex_replace(assetNameWide, Pattern, TEXT("$1$2.$2_C"));

                auto softObjectPtr = UECustom::TSoftObjectPtr<UObject>(UECustom::FSoftObjectPath(assetNameWide));
                auto asset = UECustom::UKismetSystemLibrary::LoadAsset_Blocking(softObjectPtr);
                if (!asset)
                {
                    throw std::runtime_error(RC::fmt("Failed to apply blueprint changes, asset '%S' was invalid", assetNameWide.c_str()));
                }

                asset->SetRootSet();

                auto& defaultObject = static_cast<UClass*>(asset)->GetClassDefaultObject();
                ApplyData(assetData, defaultObject.Get());

                PS::Log<RC::LogLevel::Normal>(TEXT("Applied changes to {}\n"), static_cast<UClass*>(asset)->GetNamePrivate().ToString());
            }
        }
    }

    std::vector<PalBlueprintMod>& PalBlueprintModLoader::GetModsForBlueprint(const RC::Unreal::FName& name)
    {
        auto it = m_modsMap.find(name);
        if (it != m_modsMap.end())
        {
            return it->second;
        }

        throw std::runtime_error(RC::fmt("Failed to get mods for this blueprint. Affected mod name: %S", name.ToString().c_str()));
    }

    void PalBlueprintModLoader::ModifyObject(RC::Unreal::UObject* object)
    {
        if (!object) return;

        auto objectClass = object->GetClassPrivate();
        auto& objectName = objectClass->GetNamePrivate();

        if (!m_modsMap.contains(objectName))
        {
            return;
        }

        auto& mods = GetModsForBlueprint(objectName);
        for (auto& mod : mods)
        {
            try
            {
                ApplyMod(mod, object);
            }
            catch (const std::exception& e)
            {
                PS::Log<RC::LogLevel::Error>(TEXT("Failed modifying blueprint '{}', {}\n"), objectName.ToString(), RC::to_generic_string(e.what()));
            }
        }
    }

    void PalBlueprintModLoader::ApplyMod(const PalBlueprintMod& mod, UObject* object)
    {
        auto& data = mod.GetData();
        ApplyData(data, object);
    }

    void PalBlueprintModLoader::ApplyData(const nlohmann::json& data, RC::Unreal::UObject* object)
    {
        auto objectClass = static_cast<UECustom::UBlueprintGeneratedClass*>(object->GetClassPrivate());

        for (auto& [propertyName, propertyValue] : data.items())
        {
            auto propertyNameWide = RC::to_generic_string(propertyName);
            auto property = Palworld::PropertyHelper::GetPropertyByName(objectClass, propertyNameWide);
            
            if (!property)
            {
                PS::Log<RC::LogLevel::Warning>(TEXT("Property '{}' does not exist in {}\n"), propertyNameWide, objectClass->GetNamePrivate().ToString());
                continue;
            }

            if (auto objectProperty = CastField<FObjectProperty>(property))
            {
                auto objectValue = *property->ContainerPtrToValuePtr<UObject*>(object);
                if (!objectValue)
                {
                    // null Object means that this property could be a component template, so we should check if it has an associated GEN_VARIABLE.
                    HandleInheritableComponent(objectClass, propertyNameWide, propertyValue);
                }
                else
                {
                    // Object has a pointer assigned to it so we let PropertyHelper handle it.
                    PropertyHelper::CopyJsonValueToContainer(object, property, propertyValue);
                }
            }
            else
            {
                // Any other property values get handled here like Numeric, Bool, String, etc.
                PropertyHelper::CopyJsonValueToContainer(object, property, propertyValue);
            }
        }
    }

    void PalBlueprintModLoader::HandleInheritableComponent(UECustom::UBlueprintGeneratedClass* bpClass, const RC::StringType& componentName,
                                                         const nlohmann::json& componentData)
    {
        auto& bpClassName = bpClass->GetNamePrivate();

        if (!componentData.is_object())
        {
            PS::Log<LogLevel::Warning>(TEXT("{} failed to apply, provided JSON value wasn't an object\n"), bpClassName.ToString());
            return;
        }

        auto componentFullName = std::format(TEXT("{}_GEN_VARIABLE"), componentName);
        UObject* inheritableComponent = nullptr;

        auto inheritableComponentHandler = bpClass->GetInheritableComponentHandler();
        if (inheritableComponentHandler)
        {
            auto records = inheritableComponentHandler->GetRecords();
            for (auto& record : records)
            {
                if (record.ComponentTemplate.Get() == nullptr) continue;

                if (record.ComponentTemplate.Get()->GetName() == componentFullName)
                {
                    inheritableComponent = record.ComponentTemplate.Get();
                    break;
                }
            }
        }

        if (inheritableComponent)
        {
            ModifyComponent(inheritableComponent, componentData);
            return;
        }

        // Component wasn't inside Inheritable Components list, so check SimpleConstructionScript next.
        HandleNodeComponent(bpClass, componentFullName, componentData);
    }

    void PalBlueprintModLoader::HandleNodeComponent(UECustom::UBlueprintGeneratedClass* bpClass, const RC::StringType& componentName, const nlohmann::json& componentData)
    {
        auto simpleConstructionScript = bpClass->GetSimpleConstructionScript();
        if (!simpleConstructionScript)
        {
            return;
        }

        UObject* nodeComponent = nullptr;

        auto& nodes = simpleConstructionScript->GetAllNodes();
        for (auto& nodeElement : nodes)
        {
            auto nodeComponentTemplate = nodeElement->GetComponentTemplate();
            if (!nodeComponentTemplate)
            {
                continue;
            }

            if (nodeComponentTemplate->GetName() == componentName)
            {
                nodeComponent = nodeComponentTemplate;
                break;
            }
        }

        if (!nodeComponent)
        {
            return;
        }

        ModifyComponent(nodeComponent, componentData);
    }

    void PalBlueprintModLoader::ModifyComponent(RC::Unreal::UObject* component, const nlohmann::json& componentData)
    {
        for (auto& [innerKey, innerValue] : componentData.items())
        {
            auto componentPropertyName = RC::to_generic_string(innerKey);
            auto componentProperty = PropertyHelper::GetPropertyByName(component->GetClassPrivate(), componentPropertyName.c_str());
            if (!componentProperty)
            {
                PS::Log<LogLevel::Warning>(TEXT("Property {} doesn't exist in {}\n"), componentPropertyName, component->GetName());
                continue;
            }

            PropertyHelper::CopyJsonValueToContainer(component, componentProperty, innerValue);
        }
    }

    void PalBlueprintModLoader::PostLoad(RC::Unreal::UClass* self)
    {
        PostLoadHook.call(self);

        if (!PostLoadCallback)
        {
            return;
        }

        PostLoadCallback(self);
    }

    void PalBlueprintModLoader::PostInitComponents(RC::Unreal::AActor* self)
    {
        PostInitComponentsHook.call(self);

        if (!PostInitComponentsCallback)
        {
            return;
        }

        PostInitComponentsCallback(self);
    }
}