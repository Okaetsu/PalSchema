#pragma once

#include "Loader/WildcardFilter/WildcardFilter.h"

namespace PS {
    class WildcardEnumFilter : public WildcardFilter {
    public:
        WildcardEnumFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data);

        virtual bool Compare(void* container) override;
    private:
        virtual void Validate() override final;

        void FixupEnumString(RC::StringType& enumString);

        std::vector<RC::StringType> m_enumFilterStrings;
    };
}