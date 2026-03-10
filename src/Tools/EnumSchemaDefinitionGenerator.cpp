#include "Tools/EnumSchemaDefinitionGenerator.h"
#include "Unreal/UEnum.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "nlohmann/json.hpp"
#include <fstream>

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	void EnumSchemaDefinitionGenerator::Generate()
	{
        std::vector<UObject*> EnumObjects;
        UObjectGlobals::FindAllOf(TEXT("Enum"), EnumObjects);

        nlohmann::ordered_json JsonDefinitions;

        for (auto EnumObject : EnumObjects)
        {
            auto Enum = static_cast<UEnum*>(EnumObject);
            auto EnumName = RC::to_string(Enum->GetName());

            if (!EnumName.starts_with("EPal"))
            {
                continue;
            }

            auto Names = Enum->GetEnumNames();

            JsonDefinitions[EnumName] = {
                { "type", "string" },
                { "enum", {} }
            };

            for (auto& Name : Names)
            {
                auto EnumKey = RC::to_string(Name.Key.ToString());
                if (EnumKey.ends_with("MAX") || EnumKey.ends_with("Max"))
                {
                    continue;
                }

                auto Offset = EnumKey.find("::");
                EnumKey.erase(0, Offset + 2);
                JsonDefinitions[EnumName]["enum"].push_back(EnumKey);
            }
        }

        std::ofstream DefinitionsFile("enum_definitions.json");
        DefinitionsFile << JsonDefinitions.dump(2);
        DefinitionsFile.close();
	}
}