#pragma once

#include "efsw/efsw.hpp"
#include <functional>
#include <filesystem>

namespace PS {
    using FilesystemUpdateCallback = std::function<void(efsw::WatchID, const std::string&, const std::string&, efsw::Action,
        std::string)>;

    class UpdateListener : public efsw::FileWatchListener {
    public:
        void handleFileAction(efsw::WatchID watchId, const std::string& dir,
            const std::string& filename, efsw::Action action,
            std::string oldFilename) override {
            if (m_fileActionCallback)
            {
                m_fileActionCallback(watchId, dir, filename, action, oldFilename);
            }
        }

        void registerCallback(const FilesystemUpdateCallback& callback)
        {
            m_fileActionCallback = callback;
        }
    private:
        FilesystemUpdateCallback m_fileActionCallback = nullptr;
    };

    class FileWatchWrapper {
    public:
        FileWatchWrapper(const std::filesystem::path& path, const FilesystemUpdateCallback& callback);
        ~FileWatchWrapper();

        void Watch();
    private:
        std::unique_ptr<efsw::FileWatcher> m_fileWatcher = nullptr;
        std::unique_ptr<UpdateListener> m_updateListener = nullptr;
        efsw::WatchID m_fileWatchId{};
    };
}