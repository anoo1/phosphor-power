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

#include "monitor.hpp"

#include <functional>

namespace phosphor::power::ibm_ups
{

/**
 * D-Bus service/bus name for this application.
 */
constexpr auto serviceName = "xyz.openbmc_project.Power.IBMUPS";

Monitor::Monitor(sdbusplus::bus::bus& bus, const sdeventplus::Event& event) :
    bus{bus}, eventLoop{event}, ups{bus},
    timer{event, std::bind(&Monitor::timerExpired, this)}
{
    // Obtain D-Bus service name
    bus.request_name(serviceName);

    // Start timer that polls UPS device for current status
    startTimer();
}

} // namespace phosphor::power::ibm_ups
