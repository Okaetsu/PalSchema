#pragma once

#include "Unreal/UObject.hpp"
#include "Unreal/NameTypes.hpp"
#include "SDK/Classes/TSoftObjectPtr.h"

namespace Palworld {
	class UPalNoteData : public RC::Unreal::UObject {
	public:
		RC::Unreal::FName TextId_Description;
		UECustom::TSoftObjectPtr<RC::Unreal::UObject> Texture;
	};
}
