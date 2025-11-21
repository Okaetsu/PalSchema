#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/NameTypes.hpp"

namespace Palworld {

	struct FPalItemShopSettingDataRow : public RC::Unreal::FTableRowBase
	{
		RC::Unreal::FName CurrencyItemID;

		FPalItemShopSettingDataRow() = default;
		FPalItemShopSettingDataRow(const RC::StringType& InCurrency) : CurrencyItemID(RC::Unreal::FName(InCurrency, RC::Unreal::FNAME_Add)) {}
	};

}
