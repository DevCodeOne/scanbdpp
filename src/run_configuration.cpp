#include "run_configuration.h"

namespace scanbdpp {

    bool RunConfiguration::manager_mode() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _manager_mode;
    }

    bool RunConfiguration::foreground() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _foreground;
    }

    bool RunConfiguration::signal() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _signal;
    }

    bool RunConfiguration::debug() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _debug;
    }

    int RunConfiguration::debug_level() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _debug_level;
    }

    const std::experimental::filesystem::path &RunConfiguration::config_path() const {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        return _config_path;
    }

    RunConfiguration &RunConfiguration::manager_mode(bool value) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _manager_mode = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::foreground(bool value) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _foreground = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::signal(bool value) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _signal = value;
        return *this;
    }
    RunConfiguration &RunConfiguration::debug(bool value) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _debug = value;
        return *this;
    }
    RunConfiguration &RunConfiguration::debug_level(int value) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _debug_level = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::config_path(const std::experimental::filesystem::path &config_path) {
        std::lock_guard<std::mutex> guard{_instance_mutex};
        _config_path = config_path;
        return *this;
    }

}  // namespace scanbdpp
