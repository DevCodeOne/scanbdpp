#include <iostream>
#include <chrono>
#include <thread>

#include "cxxopts.hpp"

#include "udev.h"

int main(int argc, char *argv[]) {
    cxxopts::Options options("scanbd", "scanbd is a scanner button daemon");

    options.add_options()
        ("m,manager", "start in manager mode")
        ("s,signal", "start in signal mode")
        ("d,debug", "enable debugging", cxxopts::value<int>())
        ("f,foreground", "start in foreground")
        ("c,config", "provide custom config file", cxxopts::value<std::string>())
        ("t,trigger", "add trigger for device", cxxopts::value<int>())
        ("a,action", "trigger action number", cxxopts::value<int>())
        ("h,help", "print this help menu");

    try {
        options.parse(argc, argv);

        if (options.count("help")) {
            std::cout << options.help() << std::endl;
        }
        if (options.count("d")) {
            std::cout << options["d"].as<int>() << std::endl;
        }
    } catch (cxxopts::option_not_exists_exception e) {
        std::cout << e.what() << std::endl;
        std::cout << options.help() << std::endl;
    } catch (cxxopts::option_requires_argument_exception e) {
        std::cout << e.what() << std::endl;
    } catch (cxxopts::argument_incorrect_type e) {
        std::cout << "The provided argument is of the wrong type"<< std::endl;
    }

    return 0;
}
