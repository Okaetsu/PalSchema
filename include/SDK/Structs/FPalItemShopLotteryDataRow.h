#pragma once

#include "Unreal/Engine/UDataTable.hpp"
#include "Unreal/Core/Containers/Array.hpp"
#include "Unreal/NameTypes.hpp"

namespace Palworld {
	struct FPalItemShopLotteryEntry
	{
		RC::Unreal::FName ShopGroupName;
		int32_t Weight;
	};

	struct FPalItemShopLotteryDataRow : public RC::Unreal::FTableRowBase
	{
		RC::Unreal::TArray<FPalItemShopLotteryEntry> lotteryDataArray;
	};
}
