#ifndef HARDWARE_FACTORY_HPP
#define HARDWARE_FACTORY_HPP

#include "ILibraryHardware.hpp"
#include "LinuxCharDevHardware.hpp"
#include "SimulatedHardware.hpp"
#include <memory>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace lms {

class HardwareFactory {
public:
    static std::unique_ptr<ILibraryHardware> create(bool forceSimulated = false)
    {
        const char* envForce = std::getenv("LMS_FORCE_SIMULATOR");
        if (envForce && (std::string(envForce) == "1" || std::string(envForce) == "true")) {
            forceSimulated = true;
        }

        if (!forceSimulated) {
            // Check if /dev/libsys_driver is accessible
            if (::access(LIBSYS_DEVICE_PATH, R_OK | W_OK) == 0) {
                auto driver = std::make_unique<LinuxCharDevHardware>(LIBSYS_DEVICE_PATH);
                if (driver->connect()) {
                    return driver;
                }
            }
        }

        // Fallback to simulated mock hardware
        auto sim = std::make_unique<SimulatedHardware>();
        sim->connect();
        return sim;
    }
};

} // namespace lms

#endif // HARDWARE_FACTORY_HPP
