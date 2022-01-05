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

#include <org/freedesktop/UPower/Device/server.hpp>
#include <sdbusplus/server/object.hpp>

/**
 * This file contains data types and constants related to the
 * org.freedesktop.UPower.Device interface.
 */

namespace phosphor::power::ibm_ups
{

/**
 * Define simple name for the generated C++ class that implements the
 * org.freedesktop.UPower.Device interface.
 */
using DeviceInterface = sdbusplus::org::freedesktop::UPower::server::Device;

/**
 * Define simple name for the sdbusplus object_t class that implements all
 * the necessary D-Bus interfaces via templates/multiple inheritance.
 */
using DeviceObject = sdbusplus::server::object_t<DeviceInterface>;

namespace device
{

/**
 * Defined values for the Type property.
 */
namespace type
{
constexpr auto Unknown = 0;
constexpr auto LinePower = 1;
constexpr auto Battery = 2;
constexpr auto Ups = 3;
constexpr auto Monitor = 4;
constexpr auto Mouse = 5;
constexpr auto Keyboard = 6;
constexpr auto Pda = 7;
constexpr auto Phone = 8;
} // namespace type

/**
 * Defined values for the State property.
 */
namespace state
{
constexpr auto Unknown = 0;
constexpr auto Charging = 1;
constexpr auto Discharging = 2;
constexpr auto Empty = 3;
constexpr auto FullyCharged = 4;
constexpr auto PendingCharge = 5;
constexpr auto PendingDischarge = 6;
} // namespace state

/**
 * Defined values for the BatteryLevel property.
 */
namespace battery_level
{
constexpr auto Unknown = 0;
constexpr auto None = 1;
constexpr auto Low = 3;
constexpr auto Critical = 4;
constexpr auto Normal = 6;
constexpr auto High = 7;
constexpr auto Full = 8;
} // namespace battery_level

} // namespace device

} // namespace phosphor::power::ibm_ups
