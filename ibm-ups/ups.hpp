/**
 * Copyright © 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "device.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::power::ibm_ups
{

/**
 * @class UPS
 *
 * This class represents an Uninterruptible Power Supply (UPS) device.
 *
 * The UPS must be connected to the system using an IBM System Port Converter
 * Cable.  This USB cable allows for communications from a UPS relay interface
 * card to a BMC USB port.
 *
 * The UPS status is read from the USB cable.  The status is published on D-Bus
 * using the UPower Device interface.
 *
 * The PLDM application uses the D-Bus information to build PLDM state sensors.
 * These state sensors communicate the UPS status to the host operating system.
 *
 * If a UPS is not connected to the system, the IsPresent property of the D-Bus
 * interface is set to false.  The D-Bus object for the UPS always exists.  This
 * is required due to the way PLDM maps the D-Bus interface properties into PLDM
 * state sensors.
 */
class UPS : public DeviceObject
{
  public:
    // Specify which compiler-generated methods we want
    UPS() = delete;
    UPS(const UPS&) = delete;
    UPS(UPS&&) = delete;
    UPS& operator=(const UPS&) = delete;
    UPS& operator=(UPS&&) = delete;
    virtual ~UPS() = default;

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit UPS(sdbusplus::bus::bus& bus);

    /**
     * Refreshes the UPS device status by reading from the UPS cable.
     */
    virtual void refresh() override
    {
        // Not implemented yet
    }

    /**
     * Gets history for the UPS device that is persistent across reboots.
     *
     * This method from the Device interface is not supported.  History is not
     * available from the USB cable interface.
     */
    virtual std::vector<std::tuple<uint32_t, double, uint32_t>>
        getHistory(std::string /*type*/, uint32_t /*timespan*/,
                   uint32_t /*resolution*/) override
    {
        return std::vector<std::tuple<uint32_t, double, uint32_t>>{};
    }

    /**
     * Gets statistics for the UPS device.
     *
     * This method from the Device interface is not supported.  Statistics are
     * not available from the USB cable interface.
     */
    virtual std::vector<std::tuple<double, double>>
        getStatistics(std::string /*type*/) override
    {
        return std::vector<std::tuple<double, double>>{};
    }
};

} // namespace phosphor::power::ibm_ups
