#pragma once

#include "Unreal/UObject.hpp"

namespace RC::Unreal {
    class UClass;
}

namespace Palworld {
	class UPalNoteData : public RC::Unreal::UObject {
	public:
        static RC::Unreal::UClass* StaticClass();
	};
}
