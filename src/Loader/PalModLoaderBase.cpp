#include "Loader/PalModLoaderBase.h"
#include "Unreal/Engine/UDataTable.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace Palworld {
	PalModLoaderBase::PalModLoaderBase(const std::string& modFolderName) : m_modFolderName(modFolderName) {}

	PalModLoaderBase::~PalModLoaderBase() {}

    void PalModLoaderBase::Apply(RC::Unreal::UDataTable* Table) {}

    void PalModLoaderBase::Load(const nlohmann::json& Data) {}

	void PalModLoaderBase::Initialize() {}
}