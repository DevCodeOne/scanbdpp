#pragma once

#include <memory>
#include <mutex>

#include "confusepp.h"

namespace scanbdpp {

#define SCANBD_CFG_DIR "/etc/scanbd.d"

    class Config {
       public:
        Config();

        template<typename T>
        std::optional<T> get(const std::experimental::filesystem::path& element_path) const;
        void reload_config();

        explicit operator bool() const;

       private:

        inline static std::unique_ptr<confusepp::Config> _config;
        inline static std::mutex _config_mutex;
    };

    template<typename T>
    std::optional<T> Config::get(const std::experimental::filesystem::path& element_path) const {
        std::lock_guard<std::mutex> config_guard{_config_mutex};
        auto ret = _config->get<T>(element_path);

        return ret;
    }

    std::experimental::filesystem::path make_script_path_absolute(std::experimental::filesystem::path script_path);
}  // namespace scanbdpp
