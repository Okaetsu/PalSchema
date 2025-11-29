#pragma once

#include "Unreal/UObject.hpp"
#include "SDK/Structs/FSoftObjectPath.h"
#include "SDK/Classes/TSoftObjectPtr.h"


namespace UECustom {
	class UKismetSystemLibrary : public RC::Unreal::UObject {
	public:
        static void CollectGarbage();

		static FSoftObjectPath MakeSoftObjectPath(const RC::Unreal::FString& Path);

		static RC::Unreal::UObject* LoadAsset_Blocking(UECustom::TSoftObjectPtr<RC::Unreal::UObject> Asset);
	private:
		static UKismetSystemLibrary* GetDefaultObj();
	};
}