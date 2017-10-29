#pragma once

#include <experimental/filesystem>
#include <mutex>

namespace scanbdpp {
    class RunConfiguration {
       public:
        bool manager_mode() const;
        bool foreground() const;
        bool signal() const;
        bool debug() const;
        int debug_level() const;
        std::experimental::filesystem::path config_path() const;

        RunConfiguration &manager_mode(bool value);
        RunConfiguration &foreground(bool value);
        RunConfiguration &signal(bool value);
        RunConfiguration &debug(bool value);
        RunConfiguration &debug_level(int value);
        RunConfiguration &config_path(const std::experimental::filesystem::path &config_path);

       private:
        int m_debug_level;
        bool m_debug;
        bool m_manager_mode;
        bool m_foreground;
        bool m_signal;
        // TODO change later for release
        std::experimental::filesystem::path m_config_path = "../config/scanbd.conf";
        std::mutex instance_mutex;
    };

    inline RunConfiguration run_config;

}  // namespace scanbdpp
