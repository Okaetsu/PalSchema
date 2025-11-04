#pragma once

#include "SDK/Classes/UDataAsset.h"
#include "SDK/Classes/PalNoteData.h"
#include "Unreal/Core/Containers/Map.hpp"

namespace Palworld {
	class UPalNoteDataAsset : public UECustom::UDataAsset {
	public:
		RC::Unreal::TMap<RC::Unreal::FName, UPalNoteData*> NoteDataMap;
	};
}
