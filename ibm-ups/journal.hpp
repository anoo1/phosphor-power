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

#include <phosphor-logging/log.hpp>

#include <string>

/**
 * This namespace contains utility functions to simplify writing messages to the
 * system journal.
 */
namespace phosphor::power::ibm_ups::journal
{

/**
 * Logs an error message in the system journal.
 *
 * @param message message to log
 */
inline void logError(const std::string& message)
{
    using namespace phosphor::logging;
    log<level::ERR>(message.c_str());
}

/**
 * Logs an informational message in the system journal.
 *
 * @param message message to log
 */
inline void logInfo(const std::string& message)
{
    using namespace phosphor::logging;
    log<level::INFO>(message.c_str());
}

} // namespace phosphor::power::ibm_ups::journal
