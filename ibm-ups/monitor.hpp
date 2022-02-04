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
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>

namespace phosphor::power::ibm_ups
{

using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

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
     * Constructor.
     *
     * Monitoring is enabled by default, polling the UPS device for status.
     * Call disable() to disable monitoring.
     *
     * @param bus D-Bus bus object
     * @param event event object
     */
    explicit Monitor(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * Disables monitoring of the UPS device.
     *
     * The device will not be polled to obtain the current status.
     */
    void disable()
    {
        isEnabled = false;
        stopTimer();
    }

    /**
     * Enables monitoring of the UPS device.
     *
     * The device will be polled to obtain the current status.
     */
    void enable()
    {
        isEnabled = true;
        startTimer();
    }

  private:
    /**
     * Start the timer that polls the UPS device for status.
     */
    void startTimer()
    {
        // Start timer with repeating 1 second interval
        timer.restart(std::chrono::seconds(1));
    }

    /**
     * Stop the timer that polls the UPS device for status.
     */
    void stopTimer()
    {
        // Disable timer
        timer.setEnabled(false);
    }

    /**
     * Timer expired callback method.
     *
     * Polls the UPS device for status.
     */
    void timerExpired()
    {
        ups.refresh();
    }

    /**
     * D-Bus bus object.
     */
    sdbusplus::bus::bus& bus;

    /**
     * Event object to loop on.
     */
    const sdeventplus::Event& eventLoop;

    /**
     * D-Bus object manager.
     *
     * Causes this application to implement the
     * org.freedesktop.DBus.ObjectManager interface.
     */
    sdbusplus::server::manager_t manager;

    /**
     * UPS device.
     */
    UPS ups;

    /**
     * Indicates whether monitoring is enabled.
     *
     * When monitoring is enabled, the UPS device will be polled to obtain the
     * current status.
     */
    bool isEnabled{true};

    /**
     * Event timer that polls the UPS device for status.
     */
    Timer timer;
};

} // namespace phosphor::power::ibm_ups
