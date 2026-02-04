#pragma once

#include "Unreal/NameTypes.hpp"
#include "nlohmann/json.hpp"
#include <string>

namespace RC::Unreal {
	class UDataTable;
}

namespace Palworld {
	class PalModLoaderBase {
	public:
		virtual ~PalModLoaderBase();

        virtual void Apply(RC::Unreal::UDataTable* Table);

		virtual void Load(const nlohmann::json& Data);

		void Initialize();
    protected:
        PalModLoaderBase(const std::string& modFolderName);

        std::string m_modFolderName = "";
	};
}