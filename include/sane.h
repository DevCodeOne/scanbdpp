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

    class SaneHandler {
       public:
        void start();
        void stop();
        void trigger_action(const std::string &device_name, const std::string &action_name);

       private:
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
            class Action {
               public:
                using value_type = std::variant<ActionValue<int>, ActionValue<sanepp::Fixed>, ActionValue<bool>,
                                                ActionValue<std::string>>;

                inline Action(const sanepp::OptionInfo &option_info) : m_option_info(option_info) {}
                inline Action(const Action &action) = delete;
                inline Action(Action &&other)
                    : m_from_value(std::move(other.m_from_value)),
                      m_to_value(std::move(other.m_to_value)),
                      m_option_info(std::move(other.m_option_info)),
                      m_last_value(std::move(other.m_last_value)),
                      m_current_value(std::move(other.m_current_value)),
                      m_script(std::move(other.m_script)),
                      m_action_name(std::move(other.m_action_name)),
                      m_trigger((bool)other.m_trigger) {}

                inline void set_trigger() { m_trigger = true; }
                inline void unset_trigger() { m_trigger = false; }
                inline void script(const std::experimental::filesystem::path &new_script) { m_script = new_script; }
                inline void action_name(const std::string &new_action_name) { m_action_name = new_action_name; }
                inline void option_info(const sanepp::OptionInfo &new_option_info) { m_option_info = new_option_info; }
                inline void current_value(const std::optional<sanepp::Option::value_type> new_current_value) {
                    m_current_value = new_current_value;
                }
                inline void last_value(const std::optional<sanepp::Option::value_type> new_last_value) {
                    m_last_value = new_last_value;
                }
                inline void from_value(const value_type &new_from_value) { m_from_value = new_from_value; }
                inline void to_value(const value_type &new_to_value) { m_to_value = new_to_value; }

                inline bool is_triggered() const { return m_trigger; }
                inline const std::string &action_name() const { return m_action_name; }
                inline const std::experimental::filesystem::path &script() const { return m_script; }
                inline const std::optional<sanepp::Option::value_type> &current_value() const {
                    return m_current_value;
                }
                inline const std::optional<sanepp::Option::value_type> &last_value() const { return m_last_value; }
                inline const sanepp::OptionInfo &option_info() const { return m_option_info; }
                inline const value_type &from_value() { return m_from_value; }
                inline const value_type &to_value() { return m_to_value; }

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
                inline Function(const sanepp::OptionInfo &option_info) : m_option_info(option_info) {}

                inline Function &option_info(const sanepp::OptionInfo &new_option_info) {
                    m_option_info = new_option_info;
                    return *this;
                }

                inline Function &env(const std::string &new_env) {
                    m_env = new_env;
                    return *this;
                }

                inline const sanepp::OptionInfo &option_info() const { return m_option_info; }
                inline const std::string &env() const { return m_env; }

               private:
                sanepp::OptionInfo m_option_info;
                std::string m_env;
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
