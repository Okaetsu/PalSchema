#include "Misc/FileWatchWrapper.h"
#include "UE4SSProgram.hpp"

namespace fs = std::filesystem;

namespace PS {
    FileWatchWrapper::FileWatchWrapper(const std::filesystem::path& path, const FilesystemUpdateCallback& callback)
    {
        m_fileWatcher = std::make_unique<efsw::FileWatcher>();
        m_updateListener = std::make_unique<UpdateListener>();
        m_updateListener->registerCallback(callback);

        m_fileWatchId = m_fileWatcher->addWatch(path.string(), m_updateListener.get(), true);
    }

    FileWatchWrapper::~FileWatchWrapper()
    {
        m_fileWatcher->removeWatch(m_fileWatchId);
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