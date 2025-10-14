#pragma once

#include "SDK/Structs/FTableRowBase.h"
#include "Unreal/Core/Containers/Array.hpp"
#include "Unreal/NameTypes.hpp"

namespace Palworld {
	struct FPalItemShopLotteryEntry
	{
		RC::Unreal::FName ShopGroupName;
		int32_t Weight;
	};

	struct FPalItemShopLotteryDataRow : public UECustom::FTableRowBase
	{
		RC::Unreal::TArray<FPalItemShopLotteryEntry> lotteryDataArray;
	};
}
