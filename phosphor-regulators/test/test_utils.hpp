/**
 * Copyright © 2020 IBM Corporation
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
#include "action.hpp"
#include "chassis.hpp"
#include "configuration.hpp"
#include "device.hpp"
#include "i2c_interface.hpp"
#include "mock_action.hpp"
#include "presence_detection.hpp"
#include "rail.hpp"
#include "rule.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phosphor::power::regulators::test_utils
{

/**
 * Create an I2CInterface object with hard-coded bus and address values.
 *
 * @return I2CInterface object wrapped in a unique_ptr
 */
inline std::unique_ptr<i2c::I2CInterface> createI2CInterface()
{
    return i2c::create(1, 0x70, i2c::I2CInterface::InitialState::CLOSED);
}

/**
 * Creates a Device object with the specified ID.
 *
 * Creates Rail objects within the Device if railIDs is specified.
 *
 * @param id device ID
 * @param railIDs rail IDs (optional)
 * @return Device object
 */
inline std::unique_ptr<Device>
    createDevice(const std::string& id,
                 const std::vector<std::string>& railIDs = {})
{
    // Create Rails (if any)
    std::vector<std::unique_ptr<Rail>> rails{};
    for (const std::string& railID : railIDs)
    {
        rails.emplace_back(std::make_unique<Rail>(railID));
    }

    // Create Device
    bool isRegulator = true;
    std::string fru = "/system/chassis/motherboard/reg1";
    std::unique_ptr<i2c::I2CInterface> i2cInterface = createI2CInterface();
    std::unique_ptr<PresenceDetection> presenceDetection{};
    std::unique_ptr<Configuration> configuration{};
    return std::make_unique<Device>(id, isRegulator, fru,
                                    std::move(i2cInterface),
                                    std::move(presenceDetection),
                                    std::move(configuration), std::move(rails));
}

/**
 * Creates a Rule object with the specified ID.
 *
 * @param id rule ID
 * @return Rule object
 */
inline std::unique_ptr<Rule> createRule(const std::string& id)
{
    // Create actions
    std::vector<std::unique_ptr<Action>> actions{};
    actions.emplace_back(std::make_unique<MockAction>());

    // Create Rule
    return std::make_unique<Rule>(id, std::move(actions));
}

} // namespace phosphor::power::regulators::test_utils
