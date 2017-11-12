#pragma once

#include <atomic>
#include <memory>
#include <regex>
#include <thread>
#include <variant>
#include <vector>

#include "confusepp.h"

#include "sanepp.h"

namespace scanbdpp {
    struct Function {
        sanepp::Option m_option;
        std::string m_env;
    };

    template<typename T>
    struct ActionValue {
        using value_type = T;

        T value;
    };

    template<>
    struct ActionValue<std::string> {
        using value_type = std::string;

        std::string value;
        std::regex regexp;
    };

    class SaneHandler {
       public:
        void start();
        void stop();
        void start_device_polling(const std::string &device_name);
        void stop_device_polling(const std::string &device_name);
        void trigger_action(const std::string &device_name, const std::string action_name);

       private:
        class PollHandler {
           public:
            PollHandler(sanepp::Sane instance, sanepp::DeviceInfo device_info);
            PollHandler(const PollHandler &handler) = delete;
            PollHandler(PollHandler &&handler);
            ~PollHandler() = default;

            PollHandler &operator=(const PollHandler &) = delete;
            PollHandler &operator=(PollHandler &&) = delete;

            void stop();

            void poll_device();
            const sanepp::DeviceInfo &device_info() const;
            const std::atomic_bool &should_stop() const;
            const std::thread &poll_thread() const;
            std::thread &poll_thread();

           private:
            struct Action {
                using value_type = std::variant<ActionValue<int>, ActionValue<sanepp::Fixed>, ActionValue<bool>,
                                                     ActionValue<std::string>>;

                inline Action(const sanepp::Option &option) : m_option(option) {}

                sanepp::Option m_option;
                value_type m_from_value;
                value_type m_to_value;
                std::string m_script;
                std::string m_action_name;
            };

            void find_matching_functions(const sanepp::Device &device, const confusepp::Section &section);
            void find_matching_options(const sanepp::Device &device, const confusepp::Section &section);

            sanepp::Sane m_instance;
            sanepp::DeviceInfo m_device_info;
            std::atomic_bool m_terminate;
            std::vector<Function> m_functions;
            std::vector<Action> m_actions;
            std::thread m_poll_thread;
        };

        static inline std::mutex _device_mutex;
        static inline std::vector<std::unique_ptr<PollHandler>> _device_threads;
    };

}  // namespace scanbdpp
