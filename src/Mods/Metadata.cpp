#include "Mods/Metadata.h"
#include "Helpers/Format.hpp"
#include "Utility/Logging.h"

using namespace RC;
using namespace RC::Unreal;

namespace PS
{
    void FModMetadata::GetFormattedMetadata(RC::Unreal::FString& OutString)
    {
        std::string AuthorString = "";
        for (RC::Unreal::int32 Index = 0; Index < authors.size(); Index++)
        {
            auto& Author = authors.at(Index);
            AuthorString += Author;

            RC::Unreal::int32 Remaining = authors.size() - Index;
            if (Remaining == 2)
            {
                AuthorString += " and ";
            }
            else if (Remaining > 2)
            {
                AuthorString += ", ";
            }
        }

        if (AuthorString.empty())
        {
            AuthorString = "Unknown Author";
        }

        RC::StringType Format = RC::fmt(TEXT("%S v%S by %S"), name.c_str(), version.c_str(), AuthorString.c_str());
        OutString = FString(Format);
    }
}