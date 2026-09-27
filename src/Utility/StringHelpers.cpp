#include "Utility/StringHelpers.h"
#include <algorithm>

namespace PS::StringHelpers
{
    void ToLowerCase(std::vector<std::string>& StringList)
    {
        for (std::string& String : StringList)
        {
            StringHelpers::ToLowerCase(String);
        }
    }

    void ToLowerCase(std::string& String)
    {
        std::transform(String.begin(), String.end(), String.begin(), [](char Char)
        {
            return std::tolower(Char);
        });
    }
}