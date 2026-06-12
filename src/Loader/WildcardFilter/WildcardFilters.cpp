#include "Utility/JsonHelpers.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Unreal/CoreUObject/UObject/FStrProperty.hpp"
#include "Unreal/Property/FEnumProperty.hpp"
#include "Unreal/Property/FTextProperty.hpp"
#include "Loader/WildcardFilter/WildcardBooleanFilter.h"
#include "Loader/WildcardFilter/WildcardEnumFilter.h"
#include "Loader/WildcardFilter/WildcardNumericFilter.h"
#include "Loader/WildcardFilter/WildcardStringFilter.h"
#include "Loader/WildcardFilter/WildcardFilters.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS {
    void WildcardFilters::Parse(const nlohmann::json& data, UScriptStruct* rowStruct)
    {
        if (!data.is_array())
        {
            throw std::runtime_error("'Filters' must be an array of objects.");
        }

        auto filterListData = data.get<std::vector<nlohmann::json>>();
        for (auto& filterData : filterListData)
        {
            if (!filterData.is_object()) throw std::runtime_error("Array item must be an object.");

            auto newFilter = CreateFilter(filterData, rowStruct);
            if (newFilter)
            {
                m_filters.push_back(std::move(newFilter));
            }
        }
    }

    bool WildcardFilters::Match(void* container)
    {
        for (auto& filter : m_filters)
        {
            if (!filter->Compare(container))
            {
                return false;
            }
        }

        return true;
    }

    bool WildcardFilters::IsEmpty()
    {
        return m_filters.empty();
    }

    std::unique_ptr<WildcardFilter> WildcardFilters::CreateFilter(const nlohmann::json& filterData, RC::Unreal::UScriptStruct* rowStruct)
    {
        using namespace Palworld;

        JsonHelpers::ValidateFieldExists(filterData, "FieldName");

        RC::Unreal::FName fieldName;
        JsonHelpers::ParseFName(filterData, "FieldName", fieldName);

        auto property = rowStruct->GetPropertyByNameInChain(fieldName);
        if (!property)
        {
            throw std::runtime_error(RC::fmt("Property '%S' does not exist.", fieldName.ToString().c_str()));
        }

        if (PropertyHelper::CastProperty<FNumericProperty>(property))
        {
            return std::move(std::make_unique<PS::WildcardNumericFilter>(property, filterData));
        }
        else if (PropertyHelper::CastProperty<FBoolProperty>(property))
        {
            return std::move(std::make_unique<PS::WildcardBooleanFilter>(property, filterData));
        }
        else if (PropertyHelper::CastProperty<FNameProperty>(property) ||
                 PropertyHelper::CastProperty<FStrProperty>(property) ||
                 PropertyHelper::CastProperty<FTextProperty>(property))
        {
            return std::move(std::make_unique<PS::WildcardStringFilter>(property, filterData));
        }
        else if (PropertyHelper::CastProperty<FEnumProperty>(property))
        {
            return std::move(std::make_unique<PS::WildcardEnumFilter>(property, filterData));
        }
        else
        {
            PS::Log<LogLevel::Warning>(STR("Property '{}' does not support filters.\n"), property->GetFullName());
        }

        return nullptr;
    }
}