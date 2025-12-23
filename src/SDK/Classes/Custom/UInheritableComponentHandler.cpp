#include "SDK/Classes/Custom/UInheritableComponentHandler.h"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    TArray<FComponentOverrideRecord> UECustom::UInheritableComponentHandler::GetRecords()
    {
        auto Records = this->GetValuePtrByPropertyNameInChain<TArray<FComponentOverrideRecord>>(STR("Records"));
        
        if (!Records)
        {
            return {};
        }

        return *Records;
    }
}