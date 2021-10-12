#include "../power_supply.hpp"
#include "mock.hpp"

#include <xyz/openbmc_project/Common/Device/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::power::psu;
using namespace phosphor::pmbus;

using ::testing::_;
using ::testing::Args;
using ::testing::Assign;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

static auto PSUInventoryPath = "/xyz/bmc/inv/sys/chassis/board/powersupply0";
static auto PSUGPIOLineName = "presence-ps0";

class PowerSupplyTests : public ::testing::Test
{
  public:
    PowerSupplyTests() :
        mockedUtil(reinterpret_cast<const MockedUtil&>(getUtils()))
    {
        ON_CALL(mockedUtil, getPresence(_, _)).WillByDefault(Return(false));
    }

    ~PowerSupplyTests() override
    {
        freeUtils();
    }

    const MockedUtil& mockedUtil;
};

TEST_F(PowerSupplyTests, Constructor)
{
    /**
     * @param[in] invpath - String for inventory path to use
     * @param[in] i2cbus - The bus number this power supply is on
     * @param[in] i2caddr - The 16-bit I2C address of the power supply
     * @param[in] gpioLineName - The string for the gpio-line-name to read for
     * presence.
     * @param[in] bindDelay - Time in milliseconds to delay binding the device
     * driver after seeing the presence line go active.
     */
    auto bus = sdbusplus::bus::new_default();

    // Try where inventory path is empty, constructor should fail.
    try
    {
        auto psu =
            std::make_unique<PowerSupply>(bus, "", 3, 0x68, PSUGPIOLineName);
        ADD_FAILURE() << "Should not have reached this line.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty inventoryPath");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // TODO: Try invalid i2c address?

    // Try where gpioLineName is empty.
    try
    {
        auto psu =
            std::make_unique<PowerSupply>(bus, PSUInventoryPath, 3, 0x68, "");
        ADD_FAILURE()
            << "Should not have reached this line. Invalid gpioLineName.";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid empty gpioLineName");
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test with valid arguments
    // NOT using D-Bus inventory path for presence.
    try
    {
        auto psu = std::make_unique<PowerSupply>(bus, PSUInventoryPath, 3, 0x68,
                                                 PSUGPIOLineName);

        EXPECT_EQ(psu->isPresent(), false);
        EXPECT_EQ(psu->isFaulted(), false);
        EXPECT_EQ(psu->hasInputFault(), false);
        EXPECT_EQ(psu->hasMFRFault(), false);
        EXPECT_EQ(psu->hasVINUVFault(), false);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    // Test with valid arguments
    // TODO: Using D-Bus inventory path for presence.
    try
    {
        // FIXME: How do I get that presenceGPIO.read() in the startup to throw
        // an exception?

        // EXPECT_CALL(mockedUtil, getPresence(_,
        // StrEq(PSUInventoryPath)))
        //    .Times(1);
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST_F(PowerSupplyTests, Analyze)
{
    auto bus = sdbusplus::bus::new_default();

    // If I default to reading the GPIO, I will NOT expect a call to
    // getPresence().

    PowerSupply psu{bus, PSUInventoryPath, 4, 0x69, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(0));

    psu.analyze();
    // By default, nothing should change.
    EXPECT_EQ(psu.isPresent(), false);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);

    PowerSupply psu2{bus, PSUInventoryPath, 5, 0x6a, PSUGPIOLineName};
    // In order to get the various faults tested, the power supply needs to
    // be present in order to read from the PMBus device(s).
    MockedGPIOInterface* mockPresenceGPIO2 =
        static_cast<MockedGPIOInterface*>(psu2.getPresenceGPIO());
    ON_CALL(*mockPresenceGPIO2, read()).WillByDefault(Return(1));

    EXPECT_EQ(psu2.isPresent(), false);

    // STATUS_WORD 0x0000 is powered on, no faults (0x0000).
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu2.getPMBus());
    // Presence change from missing to present will trigger in1_input read in
    // an attempt to get CLEAR_FAULTS called.
    EXPECT_CALL(mockPMBus, read(READ_VIN, _)).Times(1).WillOnce(Return(206000));
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // STATUS_WORD input fault/warn
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(status_word::INPUT_FAULT_WARN));
    // Due to the fault bit on in STATUS_WORD, there will also be a read of
    // STATUS_INPUT and STATUS_MFR, so there should be 3 reads total expected.
    // STATUS_INPUT fault bits ... on.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x38));
    // STATUS_MFR don't care
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0));

    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), true);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // STATUS_WORD INPUT/UV fault.
    // First need it to return good status, then the fault
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(2)
        .WillOnce(Return(0x0000))
        .WillOnce(Return(
            (status_word::INPUT_FAULT_WARN | status_word::VIN_UV_FAULT)));
    // STATUS_INPUT fault bits ... on.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x38));
    // STATUS_MFR don't care
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0));

    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), true);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), true);

    // STATUS_WORD MFR fault.
    // First need it to return good status, then the fault
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(2)
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::MFR_SPECIFIC_FAULT));
    // STATUS_INPUT fault bits ... don't care.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x00));
    // STATUS_MFR bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0xFF));

    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), true);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), true);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // Ignore Temperature fault.
    // First STATUS_WORD with no bits set, then with temperature fault.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(2)
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::TEMPERATURE_FAULT_WARN));
    // If STATUS_WORD bits set, should read STATUS_MFR_SPECIFIC and STATUS_INPUT
    // STATUS_INPUT fault bits ... don't care.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x00));
    // STATUS_MFR don't care
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0));

    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // Ignore fan fault
    // First STATUS_WORD with no bits set, then with fan fault.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(2)
        .WillOnce(Return(0x0000))
        .WillOnce(Return(status_word::FAN_FAULT));
    // STATUS_WORD bits set causes read STATUS_MFR_SPECIFIC and STATUS_INPUT.
    // Don't care if bits set or not.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x00));
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0));

    psu2.analyze();
    psu2.analyze();
    EXPECT_EQ(psu2.isPresent(), true);
    EXPECT_EQ(psu2.isFaulted(), false);
    EXPECT_EQ(psu2.hasInputFault(), false);
    EXPECT_EQ(psu2.hasMFRFault(), false);
    EXPECT_EQ(psu2.hasVINUVFault(), false);

    // TODO: ReadFailure
}

TEST_F(PowerSupplyTests, OnOffConfig)
{
    auto bus = sdbusplus::bus::new_default();
    uint8_t data = 0x15;

    // Test where PSU is NOT present
    try
    {
        // Assume GPIO presence, not inventory presence?
        PowerSupply psu{bus, PSUInventoryPath, 4, 0x69, PSUGPIOLineName};

        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        ON_CALL(*mockPresenceGPIO, read()).WillByDefault(Return(0));
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // Constructor should set initial presence, default read returns 0.
        // If it is not present, I should not be trying to write to it.
        EXPECT_CALL(mockPMBus, writeBinary(_, _, _)).Times(0);
        psu.onOffConfig(data);
    }
    catch (...)
    {}

    // Test where PSU is present
    try
    {
        // Assume GPIO presence, not inventory presence?
        PowerSupply psu{bus, PSUInventoryPath, 5, 0x6a, PSUGPIOLineName};
        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        ON_CALL(*mockPresenceGPIO, read()).WillByDefault(Return(1));
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // TODO: expect setPresence call?
        // updatePresence() private function reads gpio, called by analyze().
        psu.analyze();
        // TODO: ???should I check the filename?
        EXPECT_CALL(mockPMBus,
                    writeBinary(_, ElementsAre(0x15), Type::HwmonDeviceDebug))
            .Times(1);
        psu.onOffConfig(data);
    }
    catch (...)
    {}
}

TEST_F(PowerSupplyTests, ClearFaults)
{
    auto bus = sdbusplus::bus::new_default();
    PowerSupply psu{bus, PSUInventoryPath, 13, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // GPIO read return 1 to indicate present.
    ON_CALL(*mockPresenceGPIO, read()).WillByDefault(Return(1));
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    ON_CALL(mockPMBus, read(STATUS_WORD, _)).WillByDefault(Return(0));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);
    // STATUS_WORD with fault bits galore!
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0xFFFF));
    // If STATUS_WORD has any fault bits on, STATUS_MFR_SPECIFIC and
    // STATUS_INPUT will be read.
    // STATUS_INPUT with fault bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0xFF));
    // STATUS_MFR_SPEFIC with bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0xFF));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), true);
    EXPECT_EQ(psu.hasInputFault(), true);
    EXPECT_EQ(psu.hasMFRFault(), true);
    EXPECT_EQ(psu.hasVINUVFault(), true);
    EXPECT_CALL(mockPMBus, read("in1_input", _))
        .Times(1)
        .WillOnce(Return(209000));
    psu.clearFaults();
    EXPECT_EQ(psu.isPresent(), true);
    EXPECT_EQ(psu.isFaulted(), false);
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_EQ(psu.hasMFRFault(), false);
    EXPECT_EQ(psu.hasVINUVFault(), false);

    // TODO: Faults clear on missing/present?
}

TEST_F(PowerSupplyTests, UpdateInventory)
{
    auto bus = sdbusplus::bus::new_default();

    try
    {
        PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        // If it is not present, I should not be trying to read a string
        EXPECT_CALL(mockPMBus, readString(_, _)).Times(0);
        psu.updateInventory();
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }

    try
    {
        PowerSupply psu{bus, PSUInventoryPath, 13, 0x69, PSUGPIOLineName};
        MockedGPIOInterface* mockPresenceGPIO =
            static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
        // GPIO read return 1 to indicate present.
        EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(1));
        psu.analyze();
        MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
        EXPECT_CALL(mockPMBus, readString(_, _)).WillRepeatedly(Return(""));
        psu.updateInventory();

#if IBM_VPD
        EXPECT_CALL(mockPMBus, readString(_, _))
            .WillOnce(Return("CCIN"))
            .WillOnce(Return("PN3456"))
            .WillOnce(Return("FN3456"))
            .WillOnce(Return("HEADER"))
            .WillOnce(Return("SN3456"))
            .WillOnce(Return("FW3456"));
#endif
        psu.updateInventory();
        // TODO: D-Bus mocking to verify values stored on D-Bus (???)
    }
    catch (...)
    {
        ADD_FAILURE() << "Should not have caught exception.";
    }
}

TEST_F(PowerSupplyTests, IsPresent)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    EXPECT_EQ(psu.isPresent(), false);

    // Change GPIO read to return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).Times(1).WillOnce(Return(1));
    psu.analyze();
    EXPECT_EQ(psu.isPresent(), true);
}

TEST_F(PowerSupplyTests, IsFaulted)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 11, 0x6f, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    psu.analyze();
    EXPECT_EQ(psu.isFaulted(), false);
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    // STATUS_WORD with fault bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0xFFFF));
    // If STATUS_WORD has bit(s) on, STATUS_MFR_SPECIFIC and STATUS_INPUT read.
    // STATUS_INPUT with fault bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0xFF));
    // STATUS_MFR_SPECIFIC with faults bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0xFF));
    psu.analyze();
    EXPECT_EQ(psu.isFaulted(), true);
}

TEST_F(PowerSupplyTests, HasInputFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    psu.analyze();
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasInputFault(), false);
    EXPECT_CALL(mockPMBus, read(_, _)).Times(1).WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
    // STATUS_WORD with input fault/warn on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(status_word::INPUT_FAULT_WARN));
    // If STATUS_WORD has bit(s) on, STATUS_MFR_SPECIFIC and STATUS_INPUT read.
    // STATUS_INPUT with an input fault bit on.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x80));
    // STATUS_MFR don't care.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0x00));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), true);
    // STATUS_WORD with no bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasInputFault(), false);
}

TEST_F(PowerSupplyTests, HasMFRFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    psu.analyze();
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasMFRFault(), false);
    // First return STATUS_WORD with no bits on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
    // Next return STATUS_WORD with MFR fault bit on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(status_word::MFR_SPECIFIC_FAULT));
    // If STATUS_WORD has bits on, STATUS_MFR_SPECIFIC and STATUS_INPUT read.
    // STATUS_INPUT don't care
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x00));
    // STATUS_MFR_SPEFIC with bit(s) on.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0xFF));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), true);
    // Back to no bits on in STATUS_WORD
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasMFRFault(), false);
}

TEST_F(PowerSupplyTests, HasVINUVFault)
{
    auto bus = sdbusplus::bus::new_default();

    PowerSupply psu{bus, PSUInventoryPath, 3, 0x68, PSUGPIOLineName};
    MockedGPIOInterface* mockPresenceGPIO =
        static_cast<MockedGPIOInterface*>(psu.getPresenceGPIO());
    // Always return 1 to indicate present.
    EXPECT_CALL(*mockPresenceGPIO, read()).WillRepeatedly(Return(1));
    psu.analyze();
    MockedPMBus& mockPMBus = static_cast<MockedPMBus&>(psu.getPMBus());
    EXPECT_EQ(psu.hasVINUVFault(), false);
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
    // Turn fault on.
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(status_word::VIN_UV_FAULT));
    // Fault bits on in STATUS_WORD causes read of STATUS_MFR_SPECIFIC and
    // STATUS_INPUT.
    // Curious disagreement between PMBus Spec. Part II Figure 16 and 33.
    // Go by Figure 16, and assume bits on in STATUS_INPUT.
    EXPECT_CALL(mockPMBus, read(STATUS_INPUT, _))
        .Times(1)
        .WillOnce(Return(0x18));
    // STATUS_MFR don't care.
    EXPECT_CALL(mockPMBus, read(STATUS_MFR, _)).Times(1).WillOnce(Return(0x00));

    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), true);
    // Back to no fault bits on in STATUS_WORD
    EXPECT_CALL(mockPMBus, read(STATUS_WORD, _))
        .Times(1)
        .WillOnce(Return(0x0000));
    psu.analyze();
    EXPECT_EQ(psu.hasVINUVFault(), false);
}
