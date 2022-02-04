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
#include <filesystem>
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

    /**
     * Constructor.
     *
     * @param bus D-Bus bus object
     */
    explicit UPS(sdbusplus::bus::bus& bus);

    /**
     * Destructor.
     *
     * Closes the UPS device if necessary.
     */
    virtual ~UPS();

    /**
     * Refreshes the UPS device status by reading from the UPS cable.
     */
    virtual void refresh() override;

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

  private:
    /**
     * Close the UPS device.
     */
    void closeDevice();

    /**
     * Find the file system path to the UPS device.
     *
     * If found, the path is stored in the devicePath data member.
     *
     * @return true if file system path was found, false otherwise
     */
    bool findDevicePath();

    /**
     * Handle a failed attempt to read current status from the UPS device.
     */
    void handleReadDeviceFailure();

    /**
     * Handle a successful attempt to read current status from the UPS device.
     *
     * @param modemBits modem bit values read from the device
     */
    void handleReadDeviceSuccess(int modemBits);

    /**
     * Set D-Bus properties to initial values indicating the UPS is not present.
     *
     * @param skipSignals indicates whether to skip emitting D-Bus property
     *                    changed signals
     */
    void initializeDBusProperties(bool skipSignals = false);

    /**
     * Returns whether the UPS device has been opened.
     *
     * @return true if device is open, false otherwise
     */
    bool isDeviceOpen()
    {
        return (fd != INVALID_FD);
    }

    /**
     * Open the UPS device.
     *
     * @return true if device was opened, false otherwise
     */
    bool openDevice();

    /**
     * Read current status from the UPS device.
     */
    void readDevice();

    /**
     * Update D-Bus properties with the current status read from the UPS device.
     *
     * @param isOn specifies whether the UPS device is present/functional
     * @param isBatteryLow specifies whether the UPS battery level is low
     * @param isUtilityFail specifies whether the UPS is providing power
     *                      to the system due to a utility failure
     */
    void updateDBusProperties(bool isOn, bool isBatteryLow, bool isUtilityFail);

    /**
     * Update the error status of the UPS device.
     *
     * Log errors or clear error history based on the current UPS status.
     *
     * @param isBatteryLow specifies whether the UPS battery level is low
     * @param isUtilityFail specifies whether the UPS is providing power
     *                      to the system due to a utility failure
     */
    void updateErrorStatus(bool isBatteryLow, bool isUtilityFail);

    /**
     * Invalid value for a file descriptor.
     */
    static constexpr int INVALID_FD{-1};

    /**
     * Invalid value for the modem bits read from the UPS device.
     */
    static constexpr int INVALID_MODEM_BITS{-1};

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;

    /**
     * File system path to the UPS device.
     */
    std::filesystem::path devicePath{};

    /**
     * File descriptor for the UPS device.
     */
    int fd{INVALID_FD};

    /**
     * Number of consecutive device reads that have failed with an error.
     */
    unsigned short readErrorCount{0};

    /**
     * Number of consecutive device reads that have returned the same modem bit
     * values.
     *
     * This provides de-glitching to ignore a transient event where invalid data
     * is read.
     */
    unsigned short matchingReadCount{0};

    /**
     * Modem bits previously read from the UPS device.
     */
    int prevModemBits{INVALID_MODEM_BITS};

    /**
     * Indicates whether an error has been logged because the UPS battery
     * is discharging due to a utility failure.
     */
    bool hasLoggedBatteryDischarging{false};

    /**
     * Indicates whether an error has been logged because the UPS battery
     * level is low.
     */
    bool hasLoggedBatteryLow{false};
};

} // namespace phosphor::power::ibm_ups
