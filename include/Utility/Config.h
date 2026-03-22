#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include "nlohmann/json.hpp"

namespace PS {
    struct PSConfigSettings {
        std::string languageOverride = "";
        bool enableAutoReload = false;
        bool enableDebugLogging = false;
    };

    class PSConfig {
    public:
        static PSConfig* Get();
    public:
        std::string GetLanguageOverride();

        bool IsAutoReloadEnabled();

        bool IsDebugLoggingEnabled();

        void Load();
    private:
        static std::filesystem::path GetConfigPath();
        
        void Save();
    private:
        PSConfigSettings m_settings;
    };
}