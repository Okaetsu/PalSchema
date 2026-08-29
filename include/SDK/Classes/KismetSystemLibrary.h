#pragma once

#include "Unreal/UObject.hpp"
#include "Unreal/SoftObjectPtr.hpp"


namespace UECustom {
	class UKismetSystemLibrary : public RC::Unreal::UObject {
	public:
        static void CollectGarbage();

		static RC::Unreal::UObject* LoadAsset_Blocking(const RC::Unreal::TSoftObjectPtr<UObject>& Asset, bool bSetRootSet = false);

		static RC::Unreal::UObject* LoadAsset_Blocking(const RC::StringType& AssetPath, bool bSetRootSet = false);
	private:
		static UKismetSystemLibrary* GetDefaultObj();
	};
}