#include "Json/Property/JsonFloatProperty.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Unreal/CoreUObject/UObject/UnrealType.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    JsonFloatProperty::JsonFloatProperty() : JsonProperty(JsonProperty::Type::Float)
    {
    }

    JsonFloatProperty::JsonFloatProperty(const FString& InName) : JsonProperty(InName, JsonProperty::Type::Float)
    {
    }

    const double& JsonFloatProperty::GetValue() const
    {
        return InnerValue;
    }

    bool JsonFloatProperty::Parse(const nlohmann::ordered_json& Data)
    {
        if (Data.is_number_float())
        {
            InnerValue = Data.get<double>();
            return true;
        }

        return false;
    }

    void JsonFloatProperty::CopyValue(FProperty* Property, void* Container)
    {
        using namespace Palworld::PropertyHelper;

        if (FNumericProperty* NumericProperty = CastProperty<FNumericProperty>(Property))
        {
            NumericProperty->SetFloatingPointPropertyValue(Container, GetValue());
        }
        else
        {
            throw std::runtime_error(RC::fmt("Unsupported Type '%S' for Property '%S'",
                *GetTypeString(), Property->GetName().c_str()));
        }
    }

    void JsonFloatProperty::Print(FString& OutString, int Indent)
    {
        OutString = FString::Printf(TEXT("%s %f"), *OutString, InnerValue);
    }

    JsonProperty::Type JsonFloatProperty::StaticType()
    {
        return JsonProperty::Type::Float;
    }
}