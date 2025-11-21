#pragma once

#include "nlohmann/json.hpp"
#include "Loader/PalModLoaderBase.h"

namespace Palworld {
	class PalLanguageModLoader : public PalModLoaderBase {
	public:
		PalLanguageModLoader();

		~PalLanguageModLoader();

		virtual void Load(const nlohmann::json& Data) override final;

		void Initialize();

		const std::string& GetCurrentLanguage();
	private:
		std::string m_currentLanguage{};
	};
}