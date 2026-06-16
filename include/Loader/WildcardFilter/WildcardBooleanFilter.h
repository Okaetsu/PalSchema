#pragma once

#include "Loader/WildcardFilter/WildcardFilter.h"

namespace PS {
    class WildcardBooleanFilter : public WildcardFilter {
    public:
        WildcardBooleanFilter(RC::Unreal::FProperty* sourceProperty, const nlohmann::json& data);

        virtual bool Compare(void* container) override;
    private:
        virtual void Validate() override final;
    };
}