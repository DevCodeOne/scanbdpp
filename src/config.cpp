#include <memory>

#include "config.h"

namespace scanbdpp {

    void load_config(const std::experimental::filesystem::path &config_file) {
        using namespace confusepp;

        // TODO add device Multisection
        ConfigFormat config_structure{Section("global").values(
            Option<bool>("debug"), Option<int>("debug-level"), Option<std::string>("user"),
            Option<std::string>("group"), Option<std::string>("saned"), Option<List<std::string>>("saned_opt"),
            Option<List<std::string>>("saned_env"), Option<std::string>("scriptdir"),
            Option<std::string>("device_insert_script"), Option<std::string>("device_remove_script"),
            Option<int>("timeout"), Option<std::string>("pidfile"),
            Section("environment").values(Option<std::string>("device"), Option<std::string>("action")),
            Multisection("function")
                .values(Option<std::string>("filter"), Option<std::string>("desc"), Option<std::string>("env")),
            Option<bool>("multiple_actions"),
            Multisection("action").values(
                Option<std::string>("filter"),
                Section("numerical-trigger").values(Option<int>("from-value"), Option<int>("to-value")),
                Section("string-trigger").values(Option<std::string>("from-value"), Option<std::string>("to-value")),
                Option<std::string>("desc"), Option<std::string>("script")))};

        auto conf = Config::parse(config_file, std::move(config_structure));

        if (conf) {
            config = std::make_unique<Config>(std::move(*conf));
        }
    }

    // TODO check if method does the correct thing
    std::experimental::filesystem::path make_script_path_absolute(std::experimental::filesystem::path script_path) {
        std::experimental::filesystem::path absolute_path = "";

        if (script_path.is_absolute()) {
            absolute_path = script_path;
        } else {
            if (config) {
                auto script_dir = config->get<confusepp::Option<std::string>>("scriptdir");
                if (script_dir) {
                    std::experimental::filesystem::path script_dir_path = script_dir->value();
                    if (script_dir_path.empty() == 0) {
                        absolute_path = SCANBD_CFG_DIR / script_path;
                    } else if (script_dir_path.is_absolute()) {
                        absolute_path = script_dir_path / script_path;
                    } else {
                        absolute_path = std::experimental::filesystem::path(SCANBD_CFG_DIR) / script_dir_path / script_path;
                    }
                }
            }
        }

        return absolute_path;
    }
}  // namespace scanbdpp
