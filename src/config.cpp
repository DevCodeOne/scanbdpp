// clang-format off
#include "common.h"
#include <sys/types.h>
#include <pwd.h>
// clang-format on

#include <memory>

#include "spdlog/spdlog.h"

#include "config.h"
#include "run_configuration.h"

namespace scanbdpp {

    Config::Config() { reload_config(); }

    Config::operator bool() const {
        std::lock_guard<std::mutex> config_guard{_config_mutex};

        return _config.get() != nullptr;
    }

    void Config::reload_config() {
        std::lock_guard<std::mutex> config_guard{_config_mutex};

        RunConfiguration run_config;

        using namespace confusepp;

        auto action_structure =
            Multisection(Constants::action)
                .values(
                    Option<std::string>(Constants::filter),
                    Section(Constants::numerical_trigger)
                        .values(Option<int>(Constants::from_value).default_value(Constants::from_value_def_int),
                                Option<int>(Constants::to_value).default_value(Constants::to_value_def_int)),
                    Section(Constants::string_trigger)
                        .values(Option<std::string>(Constants::from_value).default_value(Constants::from_value_def_str),
                                Option<std::string>(Constants::to_value).default_value(Constants::to_value_def_str)),
                    Option<std::string>(Constants::desc), Option<std::string>(Constants::script));
        auto function_structure =
            Multisection(Constants::function)
                .values(Option<std::string>(Constants::filter), Option<std::string>(Constants::desc),
                        Option<std::string>(Constants::env));

        ConfigFormat config_structure{
            Section(Constants::global)
                .values(Option<bool>(Constants::debug), Option<int>(Constants::debug_level),
                        Option<std::string>(Constants::user), Option<std::string>(Constants::group),
                        Option<std::string>(Constants::saned), Option<List<std::string>>(Constants::saned_opts),
                        Option<List<std::string>>(Constants::saned_envs),
                        Option<std::string>(Constants::script_dir).default_value(Constants::script_dir_def),
                        Option<std::string>(Constants::device_insert_script),
                        Option<std::string>(Constants::device_remove_script),
                        Option<int>(Constants::timeout).default_value(Constants::timeout_def),
                        Option<std::string>(Constants::pidfile),
                        Section(Constants::environment)
                            .values(Option<std::string>(Constants::device), Option<std::string>(Constants::action)),
                        Option<bool>(Constants::multiple_actions).default_value(true), function_structure,
                        action_structure),
            Multisection(Constants::device)
                .values(Option<std::string>(Constants::filter).default_value("^fujitsu.*"),
                        Option<std::string>(Constants::desc).default_value(Constants::desc_def), action_structure,
                        function_structure),
            Function(Constants::include, cfg_include)};

        auto conf = confusepp::Config::parse(run_config.config_path(), std::move(config_structure));

        if (conf) {
            _config = std::make_unique<confusepp::Config>(std::move(*conf));
        } else {
            if (!std::experimental::filesystem::exists(run_config.config_path())) {
                spdlog::get("logger")->critical("The provided config doesn't exist");
            } else {
                spdlog::get("logger")->critical("Config failed to parse");
            }
        }
    }

    // TODO check if method does the correct thing
    // also make SCANBD_CFG_DIR a path
    std::experimental::filesystem::path make_script_path_absolute(
        const std::experimental::filesystem::path &script_path) {
        Config conf;
        RunConfiguration run_config;
        std::experimental::filesystem::path absolute_path = "";

        if (script_path.is_absolute()) {
            absolute_path = script_path;
        } else {
            auto script_dir = conf.get<confusepp::Option<std::string>>(Config::Constants::global / Config::Constants::script_dir);

            if (!script_dir) {
                auto directory = run_config.config_path();
                directory.remove_filename();
                script_dir = confusepp::Option<std::string>(directory.c_str());
            }

            confusepp::path script_dir_path = script_dir->value();
            if (script_dir_path.empty()) {
                absolute_path = SCANBD_CFG_DIR / script_path;
            } else if (script_dir_path.is_absolute()) {
                absolute_path = script_dir_path / script_path;
            } else {
                absolute_path = confusepp::path(SCANBD_CFG_DIR) / script_dir_path / script_path;
            }
        }

        return absolute_path;
    }

    std::vector<std::string> environment() {
        std::vector<std::string> env_vars;
        Config config;

        std::string current_env = "PATH";

        if (getenv(current_env.c_str())) {
            env_vars.emplace_back(getenv(current_env.c_str()));
        } else {
            env_vars.emplace_back("/usr/sbin:/usr/bin:/sbin:/bin");
        }

        current_env = "PWD";

        if (getenv(current_env.c_str())) {
            env_vars.emplace_back(getenv(current_env.c_str()));
        } else {
            auto working_directory = std::experimental::filesystem::current_path();
            if (working_directory != std::experimental::filesystem::path{}) {
                env_vars.emplace_back(working_directory.c_str());
            } else {
                spdlog::get("logger")->warn("Couldn't get working directory");
            }
        }

        current_env = "USER";

        if (getenv(current_env.c_str())) {
            env_vars.emplace_back(getenv(current_env.c_str()));
        } else {
            passwd *pwd = getpwuid(geteuid());
            env_vars.emplace_back(pwd->pw_name);
        }

        current_env = "HOME";

        if (getenv(current_env.c_str())) {
            env_vars.emplace_back(getenv(current_env.c_str()));
        } else {
            passwd *pwd = getpwuid(geteuid());
            env_vars.emplace_back(pwd->pw_dir);
        }

        return env_vars;
    }

}  // namespace scanbdpp
