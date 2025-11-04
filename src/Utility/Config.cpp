#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace PS {
    void PSConfig::Load()
    {
        auto configPath = fs::current_path() / "Mods" / "PalSchema" / "config.json";
        
        if (!s_config)
        {
            s_config = std::make_unique<PSConfig>();
        }

        if (!fs::exists(configPath))
        {
            PS::Log<RC::LogLevel::Verbose>(STR("Config file not found, using defaults.\n"));
            return;
        }

        try
        {
            std::ifstream file(configPath);
            nlohmann::ordered_json data = nlohmann::ordered_json::parse(file);
            file.close();

            if (TryRemoveDeprecatedValues(data))
            {
                // Save the cleaned config back
                std::ofstream outFile(configPath);
                outFile << data.dump(4);
                outFile.close();
            }

            GetString(data, "languageOverride", "", s_config->m_languageOverride);
            GetBool(data, "enableAutoReload", false, s_config->m_enableAutoReload);
            GetBool(data, "enableDebugLogging", false, s_config->m_enableDebugLogging);

            PS::Log<RC::LogLevel::Normal>(STR("Config loaded successfully.\n"));
        }
        catch (const std::exception& e)
        {
            PS::Log<RC::LogLevel::Error>(STR("Failed to load config: {}\n"), RC::to_generic_string(e.what()));
        }
    }

    std::string PSConfig::GetLanguageOverride()
    {
        return s_config ? s_config->m_languageOverride : "";
    }

    bool PSConfig::IsAutoReloadEnabled()
    {
        return s_config ? s_config->m_enableAutoReload : false;
    }

    bool PSConfig::IsDebugLoggingEnabled()
    {
        return s_config ? s_config->m_enableDebugLogging : false;
    }

    bool PSConfig::TryRemoveDeprecatedValues(nlohmann::ordered_json& Data)
    {
        bool removed = false;
        for (const auto& key : s_deprecatedValues)
        {
            if (Data.contains(key))
            {
                Data.erase(key);
                removed = true;
                PS::Log<RC::LogLevel::Warning>(STR("Removed deprecated config value: {}\n"), RC::to_generic_string(key));
            }
        }
        return removed;
    }

    bool PSConfig::GetString(const nlohmann::ordered_json& Data, const std::string& Key, const std::string& DefaultValue, std::string& OutValue)
    {
        if (Data.contains(Key) && Data[Key].is_string())
        {
            OutValue = Data[Key].get<std::string>();
            return true;
        }
        OutValue = DefaultValue;
        return false;
    }

    bool PSConfig::GetBool(const nlohmann::ordered_json& Data, const std::string& Key, const bool& DefaultValue, bool& OutValue)
    {
        if (Data.contains(Key) && Data[Key].is_boolean())
        {
            OutValue = Data[Key].get<bool>();
            return true;
        }
        OutValue = DefaultValue;
        return false;
    }
}
