#pragma once

#include "Unreal/NameTypes.hpp"
#include "Loader/PalModLoaderBase.h"
#include "Unreal/Core/Math/UnrealMathUtility.hpp"

namespace RC::Unreal {
	class UObject;
	class UDataTable;
}

namespace UECustom {
	class UDataAsset;
}

namespace Palworld {
	class PalAppearanceModLoader : public PalModLoaderBase {
	public:
		PalAppearanceModLoader();

		~PalAppearanceModLoader();

		void Initialize();

		virtual void Load(const nlohmann::json& Data) override final;
	private:
		RC::Unreal::UDataTable* m_hairTable;
		RC::Unreal::UDataTable* m_headTable;
		RC::Unreal::UDataTable* m_eyesTable;
		RC::Unreal::UDataTable* m_bodyTable;
		RC::Unreal::UDataTable* m_presetTable;
		RC::Unreal::UDataTable* m_colorPresetTable;
		RC::Unreal::UDataTable* m_equipmentTable;

		void Add(const RC::Unreal::FName& RowId, RC::Unreal::UDataTable* DataTable, const nlohmann::json& Data, const std::vector<std::string>& RequiredFields);

		void AddColorPreset(const RC::Unreal::FName& ColorPresetId, const nlohmann::json& Data);

		// This is its own function due to it using TMap properties which I haven't figured out yet how to dynamically set
		void AddEquipment(const RC::Unreal::FName& EquipmentId, const nlohmann::json& Data);
	};
}