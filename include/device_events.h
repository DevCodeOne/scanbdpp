#pragma once

#include <string>

namespace scanbdpp {
    class DeviceEvents {
       public:
        void device_added();
        void device_removed();

       private:
        void hook_device_ex(const std::string &parameter, const std::string &action_name,
                            const std::string &device_name);
        void hook_device_insert(const std::string &device_name);
        void hook_device_remove(const std::string &device_name);

        static constexpr size_t path_max = 512;
    };
}  // namespace scanbdpp
