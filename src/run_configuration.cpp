#include "run_configuration.h"

namespace scanbdpp {

    bool RunConfiguration::manager_mode() const { return m_manager_mode; }

    bool RunConfiguration::foreground() const { return m_foreground; }

    bool RunConfiguration::signal() const { return m_signal; }

    bool RunConfiguration::debug() const { return m_debug; }

    int RunConfiguration::debug_level() const { return m_debug_level; }

    const std::experimental::filesystem::path &RunConfiguration::config_path() const { return m_config_path; }

    RunConfiguration &RunConfiguration::manager_mode(bool value) {
        m_manager_mode = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::foreground(bool value) {
        m_foreground = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::signal(bool value) {
        m_signal = value;
        return *this;
    }
    RunConfiguration &RunConfiguration::debug(bool value) {
        m_debug = value;
        return *this;
    }
    RunConfiguration &RunConfiguration::debug_level(int value) {
        m_debug_level = value;
        return *this;
    }

    RunConfiguration &RunConfiguration::config_path(const std::experimental::filesystem::path &config_path) {
        m_config_path = config_path;
        return *this;
    }

}  // namespace scanbdpp
