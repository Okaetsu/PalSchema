#include "Misc/FileWatchWrapper.h"
#include "Platform/RuntimeEnvironment.h"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "UE4SSProgram.hpp"

namespace fs = std::filesystem;

namespace PS {
    FileWatchWrapper::FileWatchWrapper(const std::filesystem::path& path, const FilesystemUpdateCallback& callback)
    {
        const auto useGenericWatcher = Platform::IsRunningUnderWine();
        m_fileWatcher = std::make_unique<efsw::FileWatcher>(useGenericWatcher);
        m_updateListener = std::make_unique<UpdateListener>();
        m_updateListener->registerCallback(callback);

        m_fileWatchId = m_fileWatcher->addWatch(path.string(), m_updateListener.get(), true);
        if (m_fileWatchId <= efsw::Errors::NoError)
        {
            auto error = efsw::Errors::Log::getLastErrorLog();
            PS::Log<RC::LogLevel::Error>(
                STR("Unable to watch PalSchema mods for changes: {}\n"),
                RC::to_generic_string(error)
            );
            m_fileWatcher.reset();
            return;
        }

        if (useGenericWatcher)
        {
            PS::Log<RC::LogLevel::Normal>(
                STR("Using the polling file watcher for auto-reload under Wine.\n")
            );
        }
    }

    FileWatchWrapper::~FileWatchWrapper()
    {
        if (m_fileWatcher)
        {
            m_fileWatcher->removeWatch(m_fileWatchId);
        }
    }

    void FileWatchWrapper::Watch()
    {
        if (!m_fileWatcher)
        {
            return;
        }

        m_fileWatcher->watch();
    }
}
