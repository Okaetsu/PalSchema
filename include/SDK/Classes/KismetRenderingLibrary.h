#pragma once

#include "Unreal/UObject.hpp"

namespace UECustom {
    class UKismetRenderingLibrary : public RC::Unreal::UObject {
    public:
        // WorldContextObject is never used with this function, at least in 5.1. You can safely pass a nullptr to it.
        static RC::Unreal::UObject* ImportFileAsTexture2D(RC::Unreal::UObject* WorldContextObject, const RC::Unreal::FString& Filename);
    private:
        static UKismetRenderingLibrary* GetDefaultObj();
    };
}