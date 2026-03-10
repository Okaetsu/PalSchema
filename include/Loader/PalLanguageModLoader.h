#pragma once

#include "nlohmann/json.hpp"
#include "Loader/PalModLoaderBase.h"

namespace Palworld {
	class PalLanguageModLoader : public PalModLoaderBase {
	public:
		PalLanguageModLoader();

		~PalLanguageModLoader();
    protected:
        virtual void OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) override final;

        virtual bool CanInitialize(const EEngineLifecyclePhase& engineLifecyclePhase) override final;
        virtual bool OnInitialize() override final;
	private:
		std::string m_currentLanguage{};

        void LoadTranslations(const nlohmann::json& data);

        const std::string& GetCurrentLanguage();
	};
}