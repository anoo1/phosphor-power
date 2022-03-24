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

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <stdplus/signal.hpp>

#include <exception>
#include <iostream>

using namespace phosphor::power::ibm_ups;

int main(int argc, const char* argv[])
{
    int rc{0};
    try
    {
        // Block SIGHUP and SIGCONT signals that may be sent by UPS driver
        stdplus::signal::block(SIGHUP);
        stdplus::signal::block(SIGCONT);

        // Parse command line parameters (if any)
        CLI::App app{"IBM UPS Monitor"};
        bool isPollingDisabled{false};
        app.add_flag("--no-poll", isPollingDisabled,
                     "Do not poll the UPS device for status");
        CLI11_PARSE(app, argc, argv);

        // Create D-Bus bus and event
        auto bus = sdbusplus::bus::new_default();
        auto event = sdeventplus::Event::get_default();
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

        // Create UPS monitor
        Monitor monitor{bus, event};
        if (isPollingDisabled)
        {
            // Disable monitoring/polling of the UPS device
            monitor.disable();
        }

        // Obtain D-Bus service name
        bus.request_name("xyz.openbmc_project.Power.IBMUPS");

        rc = event.loop();
    }
    catch (const std::exception& e)
    {
        rc = 1;
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        rc = 1;
        std::cerr << "Unknown exception occurred." << std::endl;
    }
    return rc;
}
