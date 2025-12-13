#include "Loader/PalEnumLoader.h"
#include "Unreal/UClass.hpp"
#include "Unreal/UEnum.hpp"
#include "Helpers/String.hpp"
#include "Utility/Logging.h"
#include "SDK/Classes/Custom/UObjectGlobals.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    PalEnumLoader::PalEnumLoader() : PalModLoaderBase("enums") {}

    PalEnumLoader::~PalEnumLoader() {}

    void PalEnumLoader::Initialize()
    {
        PS::Log<LogLevel::Verbose>(STR("Attempting to get UClass for UEnum...\n"));
        auto EnumClass = UECustom::UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/CoreUObject.Enum"));

        if (!EnumClass)
        {
            PS::Log<LogLevel::Error>(STR("Unable to initialize EnumLoader, failed to get /Script/CoreUObject.Enum\n"));
            return;
        }

        TArray<UObject*> Results;

        PS::Log<LogLevel::Verbose>(STR("UClass for UEnum found, fetching UEnums...\n"));
        UECustom::UObjectGlobals::GetObjectsOfClass(EnumClass, Results);

        int AddedUEnums = 0;
        for (auto& Result : Results)
        {
            EnumNameToObjectMap.emplace(Result->GetName(), static_cast<UEnum*>(Result));
            AddedUEnums++;
        }

        PS::Log<LogLevel::Verbose>(STR("Finished mapping {} UEnums.\n"), AddedUEnums);
    }

    void PalEnumLoader::Load(const nlohmann::json& Data)
    {
        for (auto& [EnumClassName, EnumArray]: Data.items())
        {
            auto EnumClassNameWide = RC::to_generic_string(EnumClassName);
            if (!EnumArray.is_array())
            {
                throw std::runtime_error(std::format("Values in {} must be arrays of strings", EnumClassName));
            }

            auto EnumObject = GetEnumByName(EnumClassNameWide);
            if (!EnumObject) {
                throw std::runtime_error(std::format("Enum object was invalid."));
            }

            auto EnumClassName = EnumObject->GetName();
            for (auto& ArrayValue : EnumArray)
            {
                if (!ArrayValue.is_string()) throw std::runtime_error(std::format("Array must only contain strings"));

                auto EnumString = ArrayValue.get<std::string>();
                if (EnumString.find(':') != std::string::npos) {
                    throw std::runtime_error(
                    std::format("Enum value '{}' must not contain the namespace. Example: Write ExampleEnum instead of {}::ExampleEnum",
                    EnumString, RC::to_string(EnumClassName)));
                }

                auto EnumStringWide = std::format(STR("{}::{}"), EnumClassName, RC::to_generic_string(EnumString));

                auto EnumName = FName(EnumStringWide, FNAME_Add);
                int32 IndexToInsertAt = EnumObject->NumEnums() - 1;

                FEnumNamePair EnumNamePair;
                EnumNamePair.Key = EnumName;
                EnumNamePair.Value = IndexToInsertAt;

                auto ResultIndex = EnumObject->InsertIntoNames(EnumNamePair, IndexToInsertAt, true);
                if (ResultIndex < 0)
                {
                    throw std::runtime_error(std::format("Something went wrong adding the enum {}", EnumString));
                }

                PS::Log<LogLevel::Normal>(STR("Enum {} has been added.\n"), EnumStringWide);
            }
        }
    }

    RC::Unreal::UEnum* PalEnumLoader::GetEnumByName(const RC::StringType& Name)
    {
        auto EnumMapIterator = EnumNameToObjectMap.find(Name);
        if (EnumMapIterator != EnumNameToObjectMap.end())
        {
            return EnumMapIterator->second;
        }

        return nullptr;
    }
}