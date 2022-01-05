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

#include "ups.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

namespace phosphor::power::ibm_ups
{

/**
 * @class Monitor
 *
 * Monitors an Uninterruptible Power Supply (UPS) device.
 */
class Monitor
{
  public:
    // Specify which compiler-generated methods we want
    Monitor() = delete;
    Monitor(const Monitor&) = delete;
    Monitor(Monitor&&) = delete;
    Monitor& operator=(const Monitor&) = delete;
    Monitor& operator=(Monitor&&) = delete;
    ~Monitor() = default;

    /**
     * Constructor
     *
     * @param bus D-Bus bus object
     * @param event event object
     */
    explicit Monitor(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

  private:
    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event object to loop on.
     */
    const sdeventplus::Event& eventLoop;

    /**
     * UPS device.
     */
    UPS ups;
};

} // namespace phosphor::power::ibm_ups
