#include <iostream>

#include "confusepp.h"

int main() {
    using namespace confusepp;

    // TODO add device Multisection
    Root config_structure{
        Section("global")
            .values(Option<bool>("debug"),
                    Option<int>("debug-level"),
                    Option<std::string>("user"),
                    Option<std::string>("group"),
                    Option<std::string>("saned"),
                    Option<List<std::string>>("saned_opt"),
                    Option<List<std::string>>("saned_env"),
                    Option<std::string>("scriptdir"),
                    Option<std::string>("device_insert_script"),
                    Option<std::string>("device_remove_script"),
                    Option<int>("timeout"),
                    Option<std::string>("pidfile"),
                    Section("environment")
                        .values(Option<std::string>("device"),
                                Option<std::string>("action")),
                    Multisection("function")
                        .values(Option<std::string>("filter"),
                                Option<std::string>("desc"),
                                Option<std::string>("env")),
                    Option<bool>("multiple_actions"),
                    Multisection("action")
                        .values(Option<std::string>("filter"),
                                Section("numerical-trigger")
                                    .values(Option<int>("from-value"),
                                            Option<int>("to-value")),
                                Section("string-trigger")
                                    .values(Option<std::string>("from-value"),
                                            Option<std::string>("to-value")),
                                Option<std::string>("desc"),
                                Option<std::string>("script")))
    };

    auto conf = Config::parse_config("conf/scanbd.conf", std::move(config_structure));

    if (conf) {
        std::cout << "Config is valid" << std::endl;
    }

    return 0;
}
