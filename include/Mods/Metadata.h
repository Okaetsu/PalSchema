#pragma once

#include <string>
#include <vector>
#include "Unreal/Core/Containers/FString.hpp"

namespace PS
{
    struct FModMetadata
    {
        std::string mod_id{};
        std::string name{};
        std::vector<std::string> authors{};
        std::string description{};
        std::string version = "1.0.0";
        std::vector<std::string> dependencies{};
        bool enabled = true;

        // Outputs a string in the following format:
        // {ModName} v{Version} by {Authors}
        void GetFormattedMetadata(RC::Unreal::FString& OutString);
    };
}