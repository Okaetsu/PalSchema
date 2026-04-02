#include "Loader/Blueprint/PalBlueprintMod.h"
#include <Helpers/String.hpp>

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    PalBlueprintMod::PalBlueprintMod(const RC::Unreal::FName& blueprintName, const nlohmann::json& data) : m_name(blueprintName), m_data(data) {}

    const FName& PalBlueprintMod::GetBlueprintName() const
    {
        return m_name;
    }

    const nlohmann::json& PalBlueprintMod::GetData() const
    {
        return m_data;
    }
}