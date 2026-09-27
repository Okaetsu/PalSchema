#pragma once

namespace PS
{
    struct ESchemaLoaderType
    {
        enum class Type : RC::Unreal::uint8
        {
            Unknown,
            Resource,
            Enum,
            Monster,
            Human,
            Item,
            Skin,
            Appearance,
            Building,
            Raw,
            Blueprint,
            HelpGuide,
            Spawn,
            Language
        };

        static inline ESchemaLoaderType::Type GetTypeFromString(const std::string& TypeString)
        {
            if (TypeString == "resource")
            {
                return ESchemaLoaderType::Type::Resource;
            }
            else if (TypeString == "enums")
            {
                return ESchemaLoaderType::Type::Enum;
            }
            else if (TypeString == "pals")
            {
                return ESchemaLoaderType::Type::Monster;
            }
            else if (TypeString == "npcs")
            {
                return ESchemaLoaderType::Type::Human;
            }
            else if (TypeString == "items")
            {
                return ESchemaLoaderType::Type::Item;
            }
            else if (TypeString == "skins")
            {
                return ESchemaLoaderType::Type::Skin;
            }
            else if (TypeString == "appearance")
            {
                return ESchemaLoaderType::Type::Appearance;
            }
            else if (TypeString == "buildings")
            {
                return ESchemaLoaderType::Type::Building;
            }
            else if (TypeString == "raw")
            {
                return ESchemaLoaderType::Type::Raw;
            }
            else if (TypeString == "blueprints")
            {
                return ESchemaLoaderType::Type::Blueprint;
            }
            else if (TypeString == "helpguide")
            {
                return ESchemaLoaderType::Type::HelpGuide;
            }
            else if (TypeString == "spawns")
            {
                return ESchemaLoaderType::Type::Spawn;
            }
            else if (TypeString == "translations")
            {
                return ESchemaLoaderType::Type::Language;
            }

            return ESchemaLoaderType::Type::Unknown;
        }
    };
}