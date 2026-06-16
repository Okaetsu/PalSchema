#pragma once

#include "Loader/WildcardFilter/WildcardFilter.h"

namespace PS {
    class WildcardStringFilter : public WildcardFilter {
    public:
        WildcardStringFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data);

        virtual bool Compare(void* container) override final;
    private:
        virtual void Validate() override final;

        bool CompareString(RC::StringViewType a, RC::StringViewType b);

        std::vector<RC::StringType> m_filterStrings;
    };
}