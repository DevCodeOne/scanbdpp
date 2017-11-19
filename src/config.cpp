#include <experimental/filesystem>
#include <memory>

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

        if (_config) {
            return;
        }

        using namespace confusepp;

        auto action_structure = Multisection("action").values(
                Option<std::string>("filter"),
                Section("numerical-trigger").values(Option<int>("from-value"), Option<int>("to-value")),
                Section("string-trigger").values(Option<std::string>("from-value"), Option<std::string>("to-value")),
                Option<std::string>("desc"), Option<std::string>("script"));
        auto function_structure = Multisection("function").values(Option<std::string>("filter"),
                Option<std::string>("desc"), Option<std::string>("env"));

        // TODO add device Multisection
        ConfigFormat config_structure{
            Section("global").values(
            Option<bool>("debug"),
            Option<int>("debug-level"),
            Option<std::string>("user"),
            Option<std::string>("group"),
            Option<std::string>("saned"),
            Option<List<std::string>>("saned_opt"),
            Option<List<std::string>>("saned_env"),
            Option<std::string>("scriptdir"),
            Option<std::string>("device_insert_script"),
            Option<std::string>("device_remove_script"),
            Option<int>("timeout").default_value(500),
            Option<std::string>("pidfile"),
            Section("environment").values(Option<std::string>("device"), Option<std::string>("action")),
            function_structure,
            Option<bool>("multiple_actions").default_value(true),
            action_structure),
            Multisection("device").values(
                    Option<std::string>("filter").default_value("^fujitsu.*"),
                    Option<std::string>("desc").default_value("The description goes here"),
                    action_structure,
                    function_structure),
            Function("include", cfg_include)
            };

        auto conf = confusepp::Config::parse(run_config.config_path(), std::move(config_structure));

        if (conf) {
            _config = std::make_unique<confusepp::Config>(std::move(*conf));
        }
    }

    // TODO check if method does the correct thing
    std::experimental::filesystem::path make_script_path_absolute(std::experimental::filesystem::path script_path) {
        Config conf;
        std::experimental::filesystem::path absolute_path = "";

        if (script_path.is_absolute()) {
            absolute_path = script_path;
        } else {
            if (conf) {
                auto script_dir = conf.get<confusepp::Option<std::string>>("scriptdir");
                if (script_dir) {
                    std::experimental::filesystem::path script_dir_path = script_dir->value();
                    if (script_dir_path.empty() == 0) {
                        absolute_path = SCANBD_CFG_DIR / script_path;
                    } else if (script_dir_path.is_absolute()) {
                        absolute_path = script_dir_path / script_path;
                    } else {
                        absolute_path =
                            std::experimental::filesystem::path(SCANBD_CFG_DIR) / script_dir_path / script_path;
                    }
                }
            }
        }

        return absolute_path;
    }

}  // namespace scanbdpp
