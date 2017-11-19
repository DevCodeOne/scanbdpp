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
       public:
        using value_type = T;

        ActionValue(const int &value = T{}) : m_value(value) {}

        const T &value() const { return m_value; }

        ActionValue &value(const T &value) {
            m_value = value;
            return *this;
        }

        bool match(const T &other) const { return m_value == other; }

       private:
        T m_value;
    };

    template<>
    struct ActionValue<std::string> {
       public:
        using value_type = std::string;

        inline ActionValue(const std::string &value = std::string{})
            : m_regexp(value, std::regex_constants::extended) {}

        inline ActionValue &regexp(const std::string &value) {
            m_regexp.assign(value, std::regex_constants::extended);
            return *this;
        }

        inline const std::regex &value() const { return m_regexp; }

        inline bool match(const std::string &other) const { return std::regex_match(other, m_regexp); }

       private:
        std::regex m_regexp;
    };

    template<typename T>
    bool operator==(const ActionValue<T> &lhs, const T &rhs) {
        return lhs.match(rhs);
    }

    template<typename T>
    bool operator==(const T &lhs, const ActionValue<T> &rhs) {
        return rhs == lhs;
    }

    template<typename T>
    bool operator!=(const ActionValue<T> &lhs, const T &rhs) {
        return !(lhs.match(rhs));
    }

    template<typename T>
    bool operator!=(const T &lhs, const ActionValue<T> &rhs) {
        return rhs != lhs;
    }

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
                std::optional<sanepp::Option::value_type> m_last_value;
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
