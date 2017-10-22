#pragma once

#include <experimental/filesystem>
#include <memory>

#include "confusepp.h"

namespace scanbdpp {
    inline std::unique_ptr<confusepp::Config> config;

    void load_config(const std::experimental::filesystem::path &config_file);
    std::experimental::filesystem::path make_script_path_absolute(std::experimental::filesystem::path script_path);

    #define SCANBD_CFG_DIR "/etc/scanbd.d"
}  // namespace scanbdpp
