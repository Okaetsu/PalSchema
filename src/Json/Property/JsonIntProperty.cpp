#include "Json/Property/JsonIntProperty.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonIntProperty::JsonIntProperty() : JsonProperty(JsonProperty::Type::Integer)
    {
    }

    JsonIntProperty::JsonIntProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Integer)
    {
    }

    const int64& JsonIntProperty::GetValue() const
    {
        return InnerValue;
    }

    bool JsonIntProperty::Parse(const nlohmann::ordered_json& Data)
    {
        if (Data.is_number_integer())
        {
            InnerValue = Data.get<int64>();
            return true;
        }

        return false;
    }

    void JsonIntProperty::CopyValue(FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FNumericProperty* NumericProperty = CastProperty<FNumericProperty>(Property))
        {
            NumericProperty->SetIntPropertyValue(Container, GetValue());
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
    }

    void JsonIntProperty::Print(FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %d"), *OutString, InnerValue);
    }
}