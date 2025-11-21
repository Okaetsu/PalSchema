#pragma once

#include "Loader/PalModLoaderBase.h"
#include "Loader/PalLanguageModLoader.h"
#include "nlohmann/json.hpp"

namespace RC::Unreal {
    class UDataTable;
}

namespace Palworld {
    class PalMonsterModLoader : public PalModLoaderBase {
    public:
        PalMonsterModLoader();

        ~PalMonsterModLoader();

        void Initialize();
        
        virtual void Load(const nlohmann::json& json) override final;
    private:
        void Add(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void Edit(uint8_t* TableRow, const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void AddIcon(const RC::Unreal::FName& CharacterId, const RC::StringType& IconPath);

        void AddBlueprint(const RC::Unreal::FName& CharacterId, const RC::StringType& BlueprintPath);

        void AddAbilities(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void AddLoot(const RC::Unreal::FName& CharacterId, const nlohmann::json& properties);

        void AddTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

        void EditTranslations(const RC::Unreal::FName& CharacterId, const nlohmann::json& Data);

        RC::Unreal::UDataTable* m_dataTable;
        RC::Unreal::UDataTable* m_iconDataTable;
        RC::Unreal::UDataTable* m_palBpClassTable;
        RC::Unreal::UDataTable* m_wazaMasterLevelTable;
        RC::Unreal::UDataTable* m_palDropItemTable;
        RC::Unreal::UDataTable* m_palNameTable;
        RC::Unreal::UDataTable* m_palShortDescTable;
        RC::Unreal::UDataTable* m_palLongDescTable;
    };
}