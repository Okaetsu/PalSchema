#include "Utility/Config.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include <fstream>
#include "glaze/glaze.hpp"
#include "UE4SSProgram.hpp"

namespace fs = std::filesystem;

namespace PS {
    PSConfig* PSConfig::Get()
    {
        if (!s_config)
        {
            s_config = std::make_unique<PSConfig>();
        }
        
        return s_config.get();
    }

    std::string PSConfig::GetLanguageOverride()
    {
        auto config = Get();
        return config ? config->m_settings.languageOverride : "";
    }

    bool PSConfig::IsAutoReloadEnabled()
    {
        auto config = Get();
        return config ? config->m_settings.enableAutoReload : false;
    }

    bool PSConfig::IsDebugLoggingEnabled()
    {
        auto config = Get();
        return config ? config->m_settings.enableDebugLogging : false;
    }

    void PSConfig::Load()
    {
        auto folderPath = GetConfigPath();
        if (!fs::exists(folderPath))
        {
            fs::create_directory(folderPath);
        }

        auto configFile = folderPath / "config.json";
        if (!fs::exists(configFile))
        {
            this->Save();
            PS::Log<RC::LogLevel::Warning>(STR("Config file not found, a new one was generated. Default values will be used.\n"));
            return;
        }

        auto readErrorCode = glz::read_file_json < glz::opts{ .error_on_missing_keys = true } > (m_settings, configFile.string(), std::string{});
        if (readErrorCode) {
            std::string errorMessage = glz::format_error(readErrorCode, std::string{});
            PS::Log<RC::LogLevel::Error>(STR("Error parsing config: {}\n"), RC::to_generic_string(errorMessage));
            this->Save();
            PS::Log<RC::LogLevel::Normal>(STR("Config has been repaired.\n"));
        }

        PS::Log<RC::LogLevel::Normal>(STR("Config loaded.\n"));
    }

    std::filesystem::path PSConfig::GetConfigPath()
    {
        static auto path = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "config";
        return path;
    }

    void PSConfig::Save()
    {
        auto configFile = GetConfigPath() / "config.json";
        auto writeErrorCode = glz::write_file_json<glz::opts{ .prettify = true }>(m_settings, configFile.string(), std::string{});
        if (writeErrorCode)
        {
            std::string errorMessage = glz::format_error(writeErrorCode, std::string{});
            PS::Log<RC::LogLevel::Error>(STR("Failed to write to config: {}\n"), RC::to_generic_string(errorMessage));
        }
    }
}
