#pragma once

#include <experimental/filesystem>
#include <mutex>

#include "defines.h"

namespace scanbdpp {
    class RunConfiguration {
       public:
        bool manager_mode() const;
        bool foreground() const;
        bool signal() const;
        bool debug() const;
        int debug_level() const;
        const std::experimental::filesystem::path &config_path() const;

        RunConfiguration &manager_mode(bool value);
        RunConfiguration &foreground(bool value);
        RunConfiguration &signal(bool value);
        RunConfiguration &debug(bool value);
        RunConfiguration &debug_level(int value);
        RunConfiguration &config_path(const std::experimental::filesystem::path &config_path);

       private:
        static inline int _debug_level;
        static inline bool _debug;
        static inline bool _manager_mode;
        static inline bool _foreground;
        static inline bool _signal;
        static inline std::experimental::filesystem::path _config_path = SCANBD_CONF;
        static inline std::mutex _instance_mutex;
    };

}  // namespace scanbdpp
