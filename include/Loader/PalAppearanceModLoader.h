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
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual void OnAutoReload(const std::filesystem::path::string_type& modName, const std::filesystem::path& modFilePath) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
	private:
		RC::Unreal::UDataTable* m_hairTable{};
		RC::Unreal::UDataTable* m_headTable{};
		RC::Unreal::UDataTable* m_eyesTable{};
		RC::Unreal::UDataTable* m_bodyTable{};
		RC::Unreal::UDataTable* m_presetTable{};
		RC::Unreal::UDataTable* m_colorPresetTable{};
		RC::Unreal::UDataTable* m_equipmentTable{};

        void LoadAppearances(const nlohmann::json& data);

		void Add(const RC::Unreal::FName& RowId, RC::Unreal::UDataTable* DataTable, const nlohmann::json& Data, const std::vector<std::string>& RequiredFields);

		void AddColorPreset(const RC::Unreal::FName& ColorPresetId, const nlohmann::json& Data);

		// This is its own function due to it using TMap properties which I haven't figured out yet how to dynamically set
		void AddEquipment(const RC::Unreal::FName& EquipmentId, const nlohmann::json& Data);
	};
}