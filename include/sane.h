#pragma once

#include <atomic>
#include <experimental/filesystem>
#include <memory>
#include <regex>
#include <thread>
#include <variant>
#include <vector>

#include "confusepp.h"

#include "sanepp.h"

namespace scanbdpp {
    namespace detail {
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

            template<typename T2>
            bool match(const T2 &other) const {
                return m_value == other;
            }

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

        template<typename T, typename T2>
        bool operator==(const ActionValue<T> &lhs, const T2 &rhs) {
            return lhs.match(rhs);
        }

        template<typename T, typename T2>
        bool operator==(const T2 &lhs, const ActionValue<T> &rhs) {
            return rhs == lhs;
        }

        template<typename T, typename T2>
        bool operator!=(const ActionValue<T> &lhs, const T2 &rhs) {
            return !(lhs.match(rhs));
        }

        template<typename T, typename T2>
        bool operator!=(const T2 &lhs, const ActionValue<T> &rhs) {
            return rhs != lhs;
        }

        class Action {
           public:
            using value_type =
                std::variant<ActionValue<int>, ActionValue<sanepp::Fixed>, ActionValue<bool>, ActionValue<std::string>>;

            Action(const sanepp::OptionInfo &option_info);
            Action(const Action &action) = delete;
            Action(Action &&other);

            void set_trigger();
            void unset_trigger();
            void script(const std::experimental::filesystem::path &new_script);
            void action_name(const std::string &new_action_name);
            void option_info(const sanepp::OptionInfo &new_option_info);
            void current_value(const std::optional<sanepp::Option::value_type> new_current_value);
            void last_value(const std::optional<sanepp::Option::value_type> new_last_value);
            void from_value(const value_type &new_from_value);
            void to_value(const value_type &new_to_value);

            bool is_triggered() const;
            const std::string &action_name() const;
            const std::experimental::filesystem::path &script() const;
            const std::optional<sanepp::Option::value_type> &current_value() const;
            const std::optional<sanepp::Option::value_type> &last_value() const;
            const sanepp::OptionInfo &option_info() const;
            const value_type &from_value() const;
            const value_type &to_value() const;

           private:
            value_type m_from_value;
            value_type m_to_value;
            sanepp::OptionInfo m_option_info;
            std::optional<sanepp::Option::value_type> m_last_value;
            std::optional<sanepp::Option::value_type> m_current_value;
            std::experimental::filesystem::path m_script;
            std::string m_action_name;
            std::atomic_bool m_trigger = false;
        };

        class Function {
           public:
            Function(const sanepp::OptionInfo &option_info);

            Function &option_info(const sanepp::OptionInfo &new_option_info);
            Function &env(const std::string &new_env);

            const sanepp::OptionInfo &option_info() const;
            const std::string &env() const;

           private:
            sanepp::OptionInfo m_option_info;
            std::string m_env;
        };

        class PollHandler {
           public:
            PollHandler(sanepp::Sane instance, sanepp::DeviceInfo device_info);
            PollHandler(const PollHandler &handler) = delete;
            PollHandler(PollHandler &&handler) = delete;
            ~PollHandler() = default;

            PollHandler &operator=(const PollHandler &) = delete;
            PollHandler &operator=(PollHandler &&) = delete;

            void stop();

            void poll_device();
            const sanepp::DeviceInfo &device_info() const;
            const std::atomic_bool &should_stop() const;
            const std::thread &poll_thread() const;
            void trigger_action(const std::string &name);
            std::thread &poll_thread();

           private:
            void find_matching_functions(const sanepp::Device &device, const confusepp::Section &section);
            void find_matching_options(const sanepp::Device &device, const confusepp::Section &section);

            sanepp::Sane m_instance;
            sanepp::DeviceInfo m_device_info;
            std::atomic_bool m_terminate;
            std::vector<Function> m_functions;
            std::vector<Action> m_actions;
            std::thread m_poll_thread;
        };
    }  // namespace detail

    class SaneHandler {
       public:
        SaneHandler();
        ~SaneHandler();

        void start();
        void stop();
        void trigger_action(const std::string &device_name, const std::string &action_name);

       private:
        static inline std::recursive_mutex _instance_mutex;
        static inline std::vector<std::unique_ptr<detail::PollHandler>> _device_threads;
        static inline std::atomic_int _instance_count;
    };

}  // namespace scanbdpp
