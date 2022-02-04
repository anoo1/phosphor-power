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
#pragma once

#include "xyz/openbmc_project/Logging/Entry/server.hpp"

#include <sdbusplus/bus.hpp>

#include <map>
#include <string>

/**
 * This namespace contains utility functions to simplify logging UPS errors.
 */
namespace phosphor::power::ibm_ups::error_logging
{
using namespace sdbusplus::xyz::openbmc_project::Logging::server;

/**
 * Log an error indicating that the UPS battery is discharging due to a utility
 * failure.
 *
 * @param bus D-Bus bus object
 * @param devicePath path to the UPS device
 */
void logBatteryDischarging(sdbusplus::bus::bus& bus,
                           const std::string& devicePath);

/**
 * Log an error indicating that the UPS battery level is low.
 *
 * @param bus D-Bus bus object
 * @param devicePath path to the UPS device
 */
void logBatteryLow(sdbusplus::bus::bus& bus, const std::string& devicePath);

/**
 * Log an error using the D-Bus Create method.
 *
 * If logging fails, a message is written to the journal but an exception is
 * not thrown.
 *
 * @param bus D-Bus bus object
 * @param message Message property of the error log entry
 * @param severity Severity property of the error log entry
 * @param additionalData AdditionalData property of the error log entry
 */
void logError(sdbusplus::bus::bus& bus, const std::string& message,
              Entry::Level severity,
              std::map<std::string, std::string>& additionalData);

} // namespace phosphor::power::ibm_ups::error_logging
