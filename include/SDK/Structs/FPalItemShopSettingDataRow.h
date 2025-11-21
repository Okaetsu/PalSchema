#pragma once

#include "SDK/Structs/FTableRowBase.h"
#include "Unreal/NameTypes.hpp"

namespace Palworld {

	struct FPalItemShopSettingDataRow : public UECustom::FTableRowBase
	{
		RC::Unreal::FName CurrencyItemID;

		FPalItemShopSettingDataRow() = default;
		FPalItemShopSettingDataRow(const RC::StringType& InCurrency) : CurrencyItemID(RC::Unreal::FName(InCurrency, RC::Unreal::FNAME_Add)) {}
	};

}
