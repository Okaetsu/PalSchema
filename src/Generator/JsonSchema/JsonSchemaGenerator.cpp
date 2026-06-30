#include <fstream>
#include "SDK/Structs/FTopLevelAssetPath.h"
#include "UE4SSProgram.hpp"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/Property/FEnumProperty.hpp"
#include "Unreal/Property/FStrProperty.hpp"
#include "Unreal/Property/FTextProperty.hpp"
#include "Unreal/UAssetRegistry.hpp"
#include "Unreal/UAssetRegistryHelpers.hpp"
#include "Utility/Logging.h"
#include "Generator/JsonSchema/JsonSchemaGenerator.h"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace PS::JsonSchemaGenerator {
    // Fwd decl START
    void ParseEnumPropertyInfo(FEnumProperty* Property, nlohmann::ordered_json& Json);
    void ParseNumericPropertyInfo(FNumericProperty* Property, nlohmann::ordered_json& Json);
    void ParseBoolPropertyInfo(FBoolProperty* Property, nlohmann::ordered_json& Json);
    void ParseStructPropertyInfo(FStructProperty* Property, nlohmann::ordered_json& Json);
    void ParseArrayPropertyInfo(FArrayProperty* Property, nlohmann::ordered_json& Json);
    void ParseMapPropertyInfo(FMapProperty* Property, nlohmann::ordered_json& Json);
    void ParseStrPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json);
    void ParseObjectPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json);
    void ParseClassPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json);
    // Fwd decl END

    void LoadAssetsByClass(UECustom::FTopLevelAssetPath ClassName, const std::function<void(UObject*)>& AssetCallback)
    {
        UAssetRegistry* AssetRegistry = static_cast<UAssetRegistry*>(UAssetRegistryHelpers::GetAssetRegistry().ObjectPointer);
        if (!AssetRegistry)
        {
            PS::Log<LogLevel::Error>(TEXT("Failed to load assets. AssetRegistry was nullptr.\n"));
            return;
        }

        TArray<FAssetData> AssetsByClass{ nullptr, 0, 0 };

        static InstancedReflectedFunction SearchAllAssets{ TEXT("/Script/AssetRegistry.AssetRegistry:SearchAllAssets") };
        SearchAllAssets.IsValid();

        struct SearchAllAssets_Params
        {
            bool bSyncSearch;
        };

        SearchAllAssets_Params SearchAllAssetsParams;
        SearchAllAssetsParams.bSyncSearch = true;
        SearchAllAssets(AssetRegistry, SearchAllAssetsParams);

        static InstancedReflectedFunction GetAssetsByClass{ TEXT("/Script/AssetRegistry.AssetRegistry:GetAssetsByClass") };
        GetAssetsByClass.IsValid();

        struct GetAssetsByClass_Params
        {
            UECustom::FTopLevelAssetPath ClassName;
            TArray<FAssetData> OutAssetData;
            bool bSearchSubClasses;
            bool ReturnValue;
        };

        GetAssetsByClass_Params GetAssetsByClassParams;
        GetAssetsByClassParams.ClassName = ClassName;
        GetAssetsByClassParams.OutAssetData = AssetsByClass;
        GetAssetsByClassParams.bSearchSubClasses = true;

        GetAssetsByClass(AssetRegistry, GetAssetsByClassParams);
        AssetsByClass.CopyFast(GetAssetsByClassParams.OutAssetData);

        auto* AssetDataScriptStruct = UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, TEXT("/Script/CoreUObject.AssetData"));
        if (!AssetDataScriptStruct)
        {
            AssetDataScriptStruct = UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, TEXT("/Script/AssetRegistry.AssetData"));
        }

        if (!AssetDataScriptStruct)
        {
            PS::Log<LogLevel::Error>(TEXT("Failed to load assets. Could not get size of AssetData ScriptStruct because the pointer was nullptr.\n"));
            return;
        }

        for (FAssetData& Asset : AssetsByClass)
        {
            UObject* LoadedAsset = UAssetRegistryHelpers::GetAsset(Asset);
            if (!LoadedAsset) { continue; }

            if (auto* ObjectItem = LoadedAsset->GetObjectItem(); ObjectItem)
            {
                AssetCallback(ObjectItem->GetUObject());
            }
        }
    }

    void ParsePropertyInfo(FProperty* Property, nlohmann::ordered_json& Json)
    {
        StringType PropertyName = Property->GetName();
        StringType PropertyClassName = Property->GetClass().GetName();
        Json[RC::to_string(PropertyName)] = {};
        nlohmann::ordered_json& JsonProperty = Json[RC::to_string(PropertyName)];

        if (auto EnumProperty = CastField<FEnumProperty>(Property))
        {
            ParseEnumPropertyInfo(EnumProperty, JsonProperty);
        }
        else if (auto NumericProperty = CastField<FNumericProperty>(Property))
        {
            ParseNumericPropertyInfo(NumericProperty, JsonProperty);
        }
        else if (auto BoolProperty = CastField<FBoolProperty>(Property))
        {
            ParseBoolPropertyInfo(BoolProperty, JsonProperty);
        }
        else if (auto StructProperty = CastField<FStructProperty>(Property))
        {
            ParseStructPropertyInfo(StructProperty, JsonProperty);
        }
        else if (auto MapProperty = CastField<FMapProperty>(Property))
        {
            ParseMapPropertyInfo(MapProperty, JsonProperty);
        }
        else if (auto ArrayProperty = CastField<FArrayProperty>(Property))
        {
            ParseArrayPropertyInfo(ArrayProperty, JsonProperty);
        }
        else if (CastField<FStrProperty>(Property) || CastField<FTextProperty>(Property) || CastField<FNameProperty>(Property))
        {
            ParseStrPropertyInfo(Property, JsonProperty);
        }
        else if (Property->GetClass().GetFName() == FObjectProperty::StaticClass().GetFName() || Property->GetClass() == FSoftObjectProperty::StaticClass())
        {
            ParseObjectPropertyInfo(Property, JsonProperty);
        }
        else if (Property->GetClass() == FClassProperty::StaticClass() || Property->GetClass() == FSoftClassProperty::StaticClass())
        {
            ParseClassPropertyInfo(Property, JsonProperty);
        }
    }

    void ParseEnumPropertyInfo(FEnumProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "string";
        Json["description"] = "EnumProperty";

        UEnum* Enum = Property->GetEnum();
        Json["$ref"] = RC::fmt("../enums.schema.json#/definitions/%S", Enum->GetName().c_str());
    }

    void ParseNumericPropertyInfo(FNumericProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "integer";
        Json["description"] = "IntProperty";

        if (Property->IsFloatingPoint())
        {
            Json["type"] = "number";
            Json["description"] = "FloatProperty";
        }
    }

    void ParseStrPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "string";

        if (CastField<FStrProperty>(Property))
        {
            Json["description"] = "FString";
        }
        else if (CastField<FTextProperty>(Property))
        {
            Json["description"] = "FText";
        }
        else if (CastField<FNameProperty>(Property))
        {
            Json["description"] = "FName";
        }
    }

    void ParseBoolPropertyInfo(FBoolProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "boolean";
        Json["description"] = "BoolProperty";
    }

    void ParseStructPropertyInfo(FStructProperty* Property, nlohmann::ordered_json& Json)
    {
        UScriptStruct* Struct = Property->GetStruct();
        Json["type"] = "object";
        Json["description"] = "StructProperty";
        Json["properties"] = {};

        for (FProperty* InnerProperty : TFieldRange<FProperty>(Struct, EFieldIterationFlags::None))
        {
            ParsePropertyInfo(InnerProperty, Json["properties"]);
        }
    }

    void ParseMapPropertyInfo(FMapProperty* Property, nlohmann::ordered_json& Json)
    {
        FProperty* KeyProp = Property->GetKeyProp();
        FProperty* ValueProp = Property->GetValueProp();

        Json["type"] = "array";
        Json["description"] = "MapProperty";
        Json["items"] = {
            { "type", "object" },
            { "properties", {
                { "Key", {} },
                { "Value", {} }
            }},
        };

        nlohmann::ordered_json KeyJson;
        nlohmann::ordered_json ValueJson;

        ParsePropertyInfo(KeyProp, KeyJson);
        ParsePropertyInfo(ValueProp, ValueJson);

        // Not a pretty solution, but it works so it'll have to do for now.
        // TODO: Figure out a cleaner way to handle nested properties in arrays/maps.
        for (auto& [Key, Value] : KeyJson.front().items())
        {
            Json["items"]["properties"]["Key"][Key] = Value;
        }

        for (auto& [Key, Value] : ValueJson.front().items())
        {
            Json["items"]["properties"]["Value"][Key] = Value;
        }
    }

    void ParseArrayPropertyInfo(FArrayProperty* Property, nlohmann::ordered_json& Json)
    {
        FProperty* InnerProp = Property->GetInner();
        nlohmann::ordered_json InnerJson = {};
        ParsePropertyInfo(InnerProp, InnerJson);

        nlohmann::ordered_json ArrayItemJson = {
            { "type", "array" },
            { "description", "ArrayProperty" },
            { "items", {} }
        };

        for (auto& [Key, Value] : InnerJson.front().items())
        {
            ArrayItemJson["items"][Key] = Value;
        }

        Json["oneOf"] = nlohmann::json::array();
        Json["oneOf"].push_back(ArrayItemJson);

        Json["oneOf"].push_back({
            { "type", "object" },
            { "properties", {
                { "Items", ArrayItemJson }
            }},
        });
    }

    void ParseObjectPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "string";
        Json["$ref"] = "../utility.schema.json#/definitions/ObjectPathRegex";
    }

    void ParseClassPropertyInfo(FProperty* Property, nlohmann::ordered_json& Json)
    {
        Json["type"] = "string";
        Json["$ref"] = "../utility.schema.json#/definitions/ClassPathRegex";
    }

    void GenerateEnumSchema(const fs::path& DestinationPath)
    {
        nlohmann::ordered_json EnumJson;

        auto AddEnumDefinition = [](UEnum* Enum, nlohmann::ordered_json& DefinitionsJson) {
            nlohmann::ordered_json EnumDefinition;
            RC::StringType EnumObjectName = Enum->GetName();
            std::string EnumObjectNameNarrow = RC::to_string(EnumObjectName);

            EnumDefinition = EnumDefinition[EnumObjectNameNarrow] = {};

            EnumDefinition["type"] = "string";
            EnumDefinition["enum"] = nlohmann::json::array();

            for (const auto& EnumNamePair : Enum->GetEnumNames())
            {
                if (Enum->GetEnumNames().Last().Key == EnumNamePair.Key)
                {
                    // Skip _MAX
                    continue;
                }

                FString EnumNameString = EnumNamePair.Key.ToFString();
                int32 ScopeIndex = EnumNameString.Find(TEXT("::"), ESearchCase::CaseSensitive);
                if (ScopeIndex != INDEX_NONE)
                {
                    FString StrippedEnumNameString = EnumNameString.Mid(ScopeIndex + 2);
                    EnumDefinition["enum"].push_back(RC::to_string(*StrippedEnumNameString));
                }

                EnumDefinition["enum"].push_back(RC::to_string(*EnumNameString));
            }

            DefinitionsJson[EnumObjectNameNarrow] = EnumDefinition;
        };

        EnumJson["$schema"] = "http://json-schema.org/draft-07/schema#";
        EnumJson["definitions"] = {};

        /* Native Enums */

        std::vector<UObject*> EnumObjects;
        UObjectGlobals::FindAllOf(TEXT("Enum"), EnumObjects);

        UClass* UserDefinedEnumClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, TEXT("/Script/Engine.UserDefinedEnum"));

        for (UObject* EnumObject : EnumObjects)
        {
            if (EnumObject->GetClassPrivate() == UserDefinedEnumClass)
            {
                // Skip UserDefinedEnum and handle them separately later
                continue;
            }

            UEnum* Enum = static_cast<UEnum*>(EnumObject);
            AddEnumDefinition(Enum, EnumJson["definitions"]);
        }

        /* User Defined Enums (BP) */

        UECustom::FTopLevelAssetPath TopLevelAssetPath;
        TopLevelAssetPath.TrySetPath(FName(TEXT("/Script/Engine")), FName(TEXT("UserDefinedEnum")));

        LoadAssetsByClass(TopLevelAssetPath, [&](UObject* EnumObject) {
            UEnum* Enum = static_cast<UEnum*>(EnumObject);
            AddEnumDefinition(Enum, EnumJson["definitions"]);
        });

        std::ofstream OutputFile(DestinationPath / "enums.schema.json");
        OutputFile << EnumJson.dump(2);

        PS::Log<LogLevel::Normal>(TEXT("Finished generating enums.schema.json.\n"));
    }

    void GenerateRawSchemas(const fs::path& DestinationPath)
    {
        auto RawSchemaPath = DestinationPath / "raw";
        if (!std::filesystem::exists(RawSchemaPath))
        {
            std::filesystem::create_directories(RawSchemaPath);
        }

        UECustom::FTopLevelAssetPath TopLevelAssetPath;
        TopLevelAssetPath.TrySetPath(FName(TEXT("/Script/Engine")), FName(TEXT("DataTable")));

        nlohmann::ordered_json RawSchemaJson = {
            { "$schema", "http://json-schema.org/draft-07/schema#" },
            { "type", "object" },
            { "properties", {} }
        };

        LoadAssetsByClass(TopLevelAssetPath, [&](UObject* Asset) {
            UDataTable* DataTable = static_cast<UDataTable*>(Asset);
            UScriptStruct* RowStruct = DataTable->GetRowStruct();

            StringType DataTableName = DataTable->GetName();
            std::string DataTableNameNarrow = RC::to_string(DataTableName);

            nlohmann::ordered_json DataTableSchemaJson = {
                { "type" , "object" },
                { "additionalProperties" , {
                    { "type", "object" },
                    { "properties", {} }
                }},
            };

            RawSchemaJson["properties"][DataTableNameNarrow] = {
                { "$ref", std::format("raw/{}.schema.json", DataTableNameNarrow) }
            };

            for (FProperty* InnerProperty : TFieldRange<FProperty>(RowStruct, EFieldIterationFlags::None))
            {
                ParsePropertyInfo(InnerProperty, DataTableSchemaJson["additionalProperties"]["properties"]);
            }

            std::ofstream OutputFile(RawSchemaPath / RC::fmt("%S.schema.json", DataTableName.c_str()));
            OutputFile << DataTableSchemaJson.dump(2);
        });

        std::ofstream OutputFile(DestinationPath / "raw.schema.json");
        OutputFile << RawSchemaJson.dump(2);

        PS::Log<LogLevel::Normal>(TEXT("Finished generating raw schema files.\n"));
    }

    void GenerateSchemaFiles()
    {
        PS::Log<LogLevel::Normal>(TEXT("Beginning generation of schema files, please wait a moment...\n"));

        auto SchemaPath = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "schemas";
        if (!std::filesystem::exists(SchemaPath))
        {
            std::filesystem::create_directories(SchemaPath);
        }

        GenerateEnumSchema(SchemaPath);
        GenerateRawSchemas(SchemaPath);

        PS::Log<LogLevel::Normal>(TEXT("Finished generating all schema files. All done!\n"));
    }
}