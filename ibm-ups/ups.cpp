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

namespace phosphor::power::ibm_ups
{

/**
 * D-Bus object path for the UPS.
 *
 * The UPower open source package is not used by this application.  However, the
 * UPower Device interface is used to publish the UPS status on D-Bus.  As a
 * result, a UPower-style D-Bus object path is used.
 */
constexpr auto objectPath = "/org/freedesktop/UPower/devices/ups_hiddev0";

UPS::UPS(sdbusplus::bus::bus& bus) : DeviceObject{bus, objectPath, true}
{
    // Set D-Bus properties that do not have the correct default value.  Skip
    // emitting D-Bus signals until the object has been fully created.
    bool skipSignal{true};
    type(device::type::Ups, skipSignal);
    powerSupply(true, skipSignal);
    state(device::state::FullyCharged, skipSignal);
    isRechargeable(true, skipSignal);
    batteryLevel(device::battery_level::Full, skipSignal);

    // Now emit signal that object has been created
    emit_object_added();
}

} // namespace phosphor::power::ibm_ups
