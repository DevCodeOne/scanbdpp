#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "confusepp.h"

#include "defines.h"

namespace scanbdpp {

    class Config {
       public:
        Config();

        template<typename T>
        std::optional<T> get(const confusepp::path& element_path) const;
        void reload_config();

        explicit operator bool() const;

        class Constants final {
           public:
            Constants() = delete;

            static inline const confusepp::path from_value = C_FROM_VALUE;
            static constexpr int from_value_def_int = C_FROM_VALUE_DEF_INT;
            static constexpr char from_value_def_str[] = C_FROM_VALUE_DEF_STR;

            static inline const confusepp::path to_value = C_TO_VALUE;
            static constexpr int to_value_def_int = C_TO_VALUE_DEF_INT;
            static constexpr char to_value_def_str[] = C_TO_VALUE_DEF_STR;

            static inline const confusepp::path filter = C_FILTER;

            static inline const confusepp::path numerical_trigger = C_NUMERICAL_TRIGGER;

            static inline const confusepp::path string_trigger = C_STRING_TRIGGER;

            static inline const confusepp::path desc = C_DESC;
            static constexpr char desc_def[] = C_DESC_DEF;

            static inline const confusepp::path script = C_SCRIPT;
            static constexpr char script_def[] = C_SCRIPT_DEF;

            static inline const confusepp::path env = C_ENV;
            static constexpr char env_function_def[] = C_ENV_FUNCTION_DEF;

            static inline const confusepp::path env_device = C_ENV_DEVICE;
            static constexpr char env_device_def[] = C_ENV_DEVICE_DEF;

            static inline const confusepp::path env_action = C_ENV_ACTION;
            static constexpr char env_action_def[] = C_ENV_ACTION_DEF;

            static inline const confusepp::path debug = C_DEBUG;
            static constexpr bool debug_def = C_DEBUG_DEF;

            static inline const confusepp::path multiple_actions = C_MULTIPLE_ACTIONS;
            static constexpr bool multiple_actions_def = C_MULTIPLE_ACTIONS_DEF;

            static inline const confusepp::path debug_level = C_DEBUG_LEVEL;
            static constexpr int debug_level_def = C_DEBUG_LEVEL_DEF;

            static inline const confusepp::path user = C_USER;
            static constexpr char user_def[] = C_USER_DEF;

            static inline const confusepp::path group = C_GROUP;
            static constexpr char group_def[] = C_GROUP_DEF;

            static inline const confusepp::path saned = C_SANED;
            static constexpr char saned_def[] = C_SANED_DEF;

            static inline const confusepp::path saned_opts = C_SANED_OPTS;
            static constexpr char saned_opts_def[] = C_SANED_OPTS_DEF;

            static inline const confusepp::path script_dir = C_SCRIPT_DIR;
            static constexpr char script_dir_def[] = C_SCRIPT_DIR_DEF;

            static inline const confusepp::path device_insert_script = C_DEVICE_INSERT_SCRIPT;
            static constexpr char device_insert_script_def[] = C_DEVICE_INSERT_SCRIPT_DEF;
            static inline const confusepp::path device_remove_script = C_DEVICE_REMOVE_SCRIPT;
            static constexpr char device_remove_script_def[] = C_DEVICE_REMOVE_SCRIPT_DEF;

            static inline const confusepp::path saned_envs = C_SANED_ENVS;
            static constexpr char saned_envs_def[] = C_SANED_ENVS_DEF;

            static inline const confusepp::path timeout = C_TIMEOUT;
            static constexpr int timeout_def = C_TIMEOUT_DEF;

            static inline const confusepp::path pidfile = C_PIDFILE;
            static constexpr char pidfile_def[] = C_PIDFILE_DEF;

            static inline const confusepp::path environment = C_ENVIRONMENT;

            static inline const confusepp::path function = C_FUNCTION;
            static constexpr char function_def[] = C_FUNCTION_DEF;

            static inline const confusepp::path action = C_ACTION;
            static constexpr char action_def[] = C_ACTION_DEF;

            static inline const confusepp::path global = C_GLOBAL;
            static inline const confusepp::path device = C_DEVICE;

            static inline const confusepp::path include = C_INCLUDE;
        };

       private:
        inline static std::unique_ptr<confusepp::Config> _config;
        inline static std::mutex _config_mutex;
    };

    template<typename T>
    std::optional<T> Config::get(const confusepp::path& element_path) const {
        std::lock_guard<std::mutex> config_guard{_config_mutex};

        if (!_config) {
            return std::optional<T>{};
        }

        auto ret = _config->get<T>(element_path);

        return ret;
    }

    std::experimental::filesystem::path make_script_path_absolute(
        const std::experimental::filesystem::path& script_path);

    std::vector<std::string> environment();
}  // namespace scanbdpp
