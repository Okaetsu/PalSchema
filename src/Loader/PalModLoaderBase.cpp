#include "Loader/PalModLoaderBase.h"
#include "Unreal/Engine/UDataTable.hpp"
#include "Utility/JsonHelpers.h"
#include "Utility/Logging.h"
#include "UE4SSProgram.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace fs = std::filesystem;

namespace Palworld {
	PalModLoaderBase::PalModLoaderBase(const std::string& modFolderType) : m_modFolderType(modFolderType) {}

	PalModLoaderBase::~PalModLoaderBase() {
        m_datatableRegistry->UnregisterDatatableSerializeCallback(m_datatableSerializeCallbackId);
    }

    void PalModLoaderBase::AssignDatatableRegistry(UECustom::UDataTableRegistry& datatableRegistry)
    {
        m_datatableRegistry = &datatableRegistry;

        m_datatableSerializeCallbackId = m_datatableRegistry->RegisterDatatableSerializeCallback([&](RC::Unreal::UDataTable* datatable) {
            OnDatatableSerialized(datatable);
        });
    }

    const RC::StringType& PalModLoaderBase::GetDisplayName() const
    {
        return m_displayName;
    }

    void PalModLoaderBase::Setup()
    {
        OnSetup();
    }

    void PalModLoaderBase::AutoReload(const std::filesystem::path::string_type& modName, const std::filesystem::path& modFilePath)
    {
        PS::JsonHelpers::ParseJsonFileInPath(modFilePath, [&](const nlohmann::json& data) {
            OnAutoReload(modName, data);
        });
    }

    void PalModLoaderBase::Load(const fs::path& modPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase)
    {
        // Loaders should be initialized by this point. Load was called before Initialize or something went wrong during initialization.
        if (!HasInitialized())
        {
            return;
        }

        auto loaderPath = modPath / m_modFolderType;
        if (!fs::is_directory(loaderPath))
        {
            return;
        }

        OnLoad(loaderPath, modName, engineLifecyclePhase);
    }

	void PalModLoaderBase::Initialize(const EEngineLifecyclePhase& engineLifecyclePhase) {
        if (!CanInitialize(engineLifecyclePhase))
        {
            return;
        }

        Initialize_Internal();

        std::lock_guard<std::mutex> guard(m_mutex);

        if (!HasInitialized())
        {
            return;
        }

        PostInitialize();
    }

    const bool& PalModLoaderBase::HasInitialized() const
    {
        return m_hasInitialized;
    }

    const std::string& PalModLoaderBase::GetModFolderType()
    {
        return m_modFolderType;
    }

    void PalModLoaderBase::SetDisplayName(const RC::StringType& displayName)
    {
        m_displayName = displayName;
    }

    void PalModLoaderBase::IterateModsFolder(const std::function<void(const std::filesystem::path&, const RC::StringType&)>& callback)
    {
        static auto modsPath = fs::path(UE4SSProgram::get_program().get_working_directory()) / "Mods" / "PalSchema" / "mods";
        if (fs::exists(modsPath))
        {
            for (const auto& entry : fs::directory_iterator(modsPath)) {
                if (entry.is_directory())
                {
                    auto& path = entry.path();
                    auto folderName = path.stem().native();
                    callback(entry.path(), folderName);
                }
            }
        }
    }

    RC::Unreal::UDataTable* PalModLoaderBase::TryGetDatatableByName(const std::string& name)
    {
        if (!m_datatableRegistry)
        {
            return nullptr;
        }

        return m_datatableRegistry->GetDatatableByName(name);
    }

    RC::Unreal::UDataTable* PalModLoaderBase::GetDatatableByName(const std::string& name)
    {
        if (!m_datatableRegistry)
        {
            throw std::runtime_error(std::format("Unable to process 'GetDatatableByName', UDataTableRegistry has not been initialized properly."));
        }

        auto datatable = TryGetDatatableByName(name);
        if (!datatable)
        {
            throw std::runtime_error(std::format("Failed to find UDataTable '{}'", name));
        }

        return datatable;
    }

    void PalModLoaderBase::OnSetup() {}

    void PalModLoaderBase::OnLoad(const std::filesystem::path& loaderPath, const RC::StringType& modName, const EEngineLifecyclePhase& engineLifecyclePhase) {}

    void PalModLoaderBase::OnAutoReload(const std::filesystem::path::string_type& modName, const nlohmann::json& data) {}

    void PalModLoaderBase::PostInitialize() {}

    void PalModLoaderBase::OnDatatableSerialized(RC::Unreal::UDataTable* datatable) {}

    void PalModLoaderBase::Initialize_Internal()
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        if (HasInitialized())
        {
            // TODO: Debugging purposes only, remove this print when done testing.
            PS::Log<LogLevel::Warning>(STR("Loader '{}' attempted to initialize more than once. Skipping.\n"), RC::to_generic_string(m_modFolderType));
            return;
        }

        if (!OnInitialize())
        {
            PS::Log<LogLevel::Error>(STR("Failed to initialize '{}' loader.\n"), RC::to_generic_string(m_modFolderType));
            return;
        }

        m_hasInitialized = true;

        PS::Log<LogLevel::Normal>(STR("Loader '{}' initialized.\n"), RC::to_generic_string(m_modFolderType));
    }
}