/**
 * Copyright © 2022 IBM Corporation
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

#include "error_logging.hpp"

#include "journal.hpp"

#include <sys/types.h>
#include <unistd.h>

#include <sdbusplus/message.hpp>

#include <exception>

namespace phosphor::power::ibm_ups::error_logging
{

void logBatteryDischarging(sdbusplus::bus::bus& bus,
                           const std::string& devicePath)
{
    std::string message{
        "xyz.openbmc_project.Power.UPS.Error.Battery.Discharging"};
    std::map<std::string, std::string> additionalData{};
    additionalData.emplace("UPS_DEVICE_PATH", devicePath);
    logError(bus, message, Entry::Level::Informational, additionalData);
}

void logBatteryLow(sdbusplus::bus::bus& bus, const std::string& devicePath)
{
    std::string message{"xyz.openbmc_project.Power.UPS.Error.Battery.Low"};
    std::map<std::string, std::string> additionalData{};
    additionalData.emplace("UPS_DEVICE_PATH", devicePath);
    logError(bus, message, Entry::Level::Informational, additionalData);
}

void logError(sdbusplus::bus::bus& bus, const std::string& message,
              Entry::Level severity,
              std::map<std::string, std::string>& additionalData)
{
    try
    {
        additionalData.emplace("_PID", std::to_string(getpid()));

        // Call D-Bus method to create error log
        const char* service = "xyz.openbmc_project.Logging";
        const char* objPath = "/xyz/openbmc_project/logging";
        const char* interface = "xyz.openbmc_project.Logging.Create";
        const char* method = "Create";
        auto reqMsg = bus.new_method_call(service, objPath, interface, method);
        reqMsg.append(message, severity, additionalData);
        auto respMsg = bus.call(reqMsg);
    }
    catch (const std::exception& e)
    {
        journal::logError(e.what());
        journal::logError("Unable to log error " + message);
    }
}

} // namespace phosphor::power::ibm_ups::error_logging
