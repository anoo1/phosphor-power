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

#include "ups.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace phosphor::power::ibm_ups
{

namespace fs = std::filesystem;

/**
 * D-Bus object path for the UPS.
 *
 * The UPower open source package is not used by this application.  However, the
 * UPower Device interface is used to publish the UPS status on D-Bus.  As a
 * result, a UPower-style D-Bus object path is used.
 */
constexpr auto objectPath = "/org/freedesktop/UPower/devices/ups_hiddev0";

/**
 * Directory where the UPS character device file should exist.
 */
const fs::path deviceDirectory = "/dev";

/**
 * Expected prefix of the UPS character device file name.
 */
constexpr auto deviceNamePrefix = "ttyUSB";

/**
 * Number of consecutive device read errors before we close the device.
 *
 * These errors may indicate the UPS has been removed.
 */
constexpr unsigned short maxReadErrorCount{3};

/**
 * Number of consecutive device reads that must return the same modem bit values
 * before the values are considered valid.
 *
 * This provides de-glitching to ignore a transient event where invalid data is
 * read.
 */
constexpr unsigned short requiredMatchingReadCount{3};

UPS::UPS(sdbusplus::bus::bus& bus) :
    DeviceObject{bus, objectPath, true}, bus{bus}
{
    // Set D-Bus properties to initial values indicating the UPS is not present.
    // Skip emitting D-Bus signals until the object has been fully created.
    bool skipSignals{true};
    initializeDBusProperties(skipSignals);

    // Now emit D-Bus signal that object has been created
    emit_object_added();
}

UPS::~UPS()
{
    try
    {
        if (isDeviceOpen())
        {
            closeDevice();
        }
    }
    catch (...)
    {
        // Destructors must not throw exceptions
    }
}

void UPS::refresh()
{
    try
    {
        // Open the UPS device if necessary
        if (!isDeviceOpen())
        {
            if (!openDevice())
            {
                // Unable to open device; may not be present
                return;
            }
        }

        // Read current status from UPS device
        readDevice();
    }
    catch (...)
    {
        // Ignore error in case UPS was removed during the actions above
    }
}

void UPS::closeDevice()
{
    // Close file descriptor
    close(fd);
    fd = INVALID_FD;

    // Clear data members used to open and read the device
    devicePath.clear();
    readErrorCount = 0;
    matchingReadCount = 0;
    prevModemBits = INVALID_MODEM_BITS;

    // Set D-Bus properties to initial values indicating the UPS is not present
    initializeDBusProperties();
}

bool UPS::findDevicePath()
{
    devicePath.clear();
    try
    {
        // Loop through all entries in the directory where the file should exist
        std::string entryName{};
        for (auto const& entry : fs::directory_iterator{deviceDirectory})
        {
            // If entry has expected prefix, exists, and is character device
            entryName = entry.path().filename().native();
            if (entryName.starts_with(deviceNamePrefix) && entry.exists() &&
                entry.is_character_file())
            {
                // Found UPS device path
                devicePath = entry.path();
                break;
            }
        }
    }
    catch (...)
    {
        // Ignore errors; UPS may have been added/removed during loop
    }
    return (!devicePath.empty());
}

void UPS::handleReadDeviceFailure()
{
    // Clear consecutive matching read count and previous modem bits
    matchingReadCount = 0;
    prevModemBits = INVALID_MODEM_BITS;

    // Increment consecutive error count
    if (readErrorCount < maxReadErrorCount)
    {
        ++readErrorCount;
    }

    // If we have reached the maximum number of read errors, close UPS device
    if (readErrorCount >= maxReadErrorCount)
    {
        closeDevice();
    }
}

void UPS::handleReadDeviceSuccess(int modemBits)
{
    // Clear consecutive read error count
    readErrorCount = 0;

    // Check if modem bits have changed since the previous read
    if (modemBits != prevModemBits)
    {
        // Modem bits have changed; set matching read count to 1
        matchingReadCount = 1;
    }
    else
    {
        // Modem bits have not changed.  Increment matching read count.
        if (matchingReadCount < requiredMatchingReadCount)
        {
            ++matchingReadCount;
        }

        // If we have reached the required number of matching reads
        if (matchingReadCount >= requiredMatchingReadCount)
        {
            // Get UPS status from modem bit values.
            bool isOn = static_cast<bool>(modemBits & TIOCM_CAR);
            bool isBatteryLow = static_cast<bool>(modemBits & TIOCM_CTS);
            bool isUtilityFail = static_cast<bool>(modemBits & TIOCM_DSR);

            // Update D-Bus properties with current UPS status
            updateDBusProperties(isOn, isBatteryLow, isUtilityFail);
        }
    }

    // Save the modem bit values for comparison during the next read
    prevModemBits = modemBits;
}

void UPS::initializeDBusProperties(bool skipSignals)
{
    type(device::type::Ups, skipSignals);
    powerSupply(true, skipSignals);
    isPresent(false, skipSignals);
    state(device::state::FullyCharged, skipSignals);
    isRechargeable(true, skipSignals);
    batteryLevel(device::battery_level::Full, skipSignals);
}

bool UPS::openDevice()
{
    bool wasOpened{false};

    // Find UPS device path
    if (findDevicePath())
    {
        // Open device path
        int rc = open(devicePath.c_str(), O_RDONLY);
        if (rc >= 0)
        {
            fd = rc;
            wasOpened = true;
        }
    }

    return wasOpened;
}

void UPS::readDevice()
{
    // Read modem bits from device driver
    int modemBits{0};
    int rc = ioctl(fd, TIOCMGET, &modemBits);
    if (rc < 0)
    {
        handleReadDeviceFailure();
    }
    else
    {
        handleReadDeviceSuccess(modemBits);
    }
}

void UPS::updateDBusProperties(bool isOn, bool isBatteryLow, bool isUtilityFail)
{
    // Set D-Bus IsPresent property.  isOn means UPS is present/functional.
    isPresent(isOn);

    // Set D-Bus State property
    if (isUtilityFail)
    {
        // Utility failure is occurring.  UPS is providing power to the system.
        state(device::state::Discharging);
    }
    else if (isBatteryLow)
    {
        // UPS is not providing power to the system, but the battery is low.
        // Assume the battery is charging.
        state(device::state::Charging);
    }
    else
    {
        // UPS is not providing power to the system, and battery is not low.
        // Assume the battery is fully charged.
        state(device::state::FullyCharged);
    }

    // Set D-Bus BatteryLevel property
    batteryLevel(isBatteryLow ? device::battery_level::Low
                              : device::battery_level::Full);
}

} // namespace phosphor::power::ibm_ups
