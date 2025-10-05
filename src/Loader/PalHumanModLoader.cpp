#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UObjectGlobals.hpp"
#include "Unreal/UScriptStruct.hpp"
#include "Unreal/FProperty.hpp"
#include "SDK/Classes/UDataTable.h"
#include "SDK/Classes/KismetInternationalizationLibrary.h"
#include "SDK/Helper/PropertyHelper.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "Loader/PalHumanModLoader.h"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
	PalHumanModLoader::PalHumanModLoader() : PalModLoaderBase("npcs") {}

	PalHumanModLoader::~PalHumanModLoader() {}

	void PalHumanModLoader::Initialize()
	{
		n_dataTable = UObjectGlobals::StaticFindObject<UECustom::UDataTable*>(nullptr, nullptr,
			STR("/Game/Pal/DataTable/Character/DT_PalHumanParameter.DT_PalHumanParameter"));

		PS::Log<RC::LogLevel::Normal>(STR("Initialized NpcModLoader\n"));

	}

	void PalHumanModLoader::Load(const nlohmann::json& json)
	{
		
	}
}