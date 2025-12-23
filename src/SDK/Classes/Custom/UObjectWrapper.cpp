#include "SDK/Classes/Custom/UObjectWrapper.h"
#include "SDK/Helper/PropertyHelper.h"

using namespace RC;
using namespace RC::Unreal;

namespace UECustom {
    void* UObjectWrapper::GetValuePtrByPropertyNameInChain(const TCHAR* PropertyName)
    {
        return Palworld::PropertyHelper::GetValuePtrByPropertyNameInChain(this, PropertyName);
    }
}