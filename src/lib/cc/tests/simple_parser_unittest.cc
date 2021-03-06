// Copyright (C) 2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cc/simple_parser.h>
#include <gtest/gtest.h>

using namespace isc::data;

/// This table defines sample default values. Although these are DHCPv6
/// specific, the mechanism is generic and can be used by any other component.
const SimpleDefaults SAMPLE_DEFAULTS = {
    { "renew-timer",        Element::integer, "900" },
    { "rebind-timer",       Element::integer, "1800" },
    { "preferred-lifetime", Element::integer, "3600" },
    { "valid-lifetime",     Element::integer, "7200" }
};

/// This list defines parameters that can be inherited from one scope
/// to another. Although these are DHCPv6 specific, the mechanism is generic and
/// can be used by any other component.
const ParamsList SAMPLE_INHERITS = {
    "renew-timer",
    "rebind-timer",
    "preferred-lifetime",
    "valid-lifetime"
};

/// @brief Simple Parser test fixture class
class SimpleParserTest : public ::testing::Test {
public:
    /// @brief Checks if specified map has an integer parameter with expected value
    ///
    /// @param map map to be checked
    /// @param param_name name of the parameter to be checked
    /// @param exp_value expected value of the parameter.
    void checkIntegerValue(const ConstElementPtr& map, const std::string& param_name,
                           int64_t exp_value) {

        // First check if the passed element is a map.
        ASSERT_EQ(Element::map, map->getType());

        // Now try to get the element being checked
        ConstElementPtr elem = map->get(param_name);
        ASSERT_TRUE(elem);

        // Now check if it's indeed integer
        ASSERT_EQ(Element::integer, elem->getType());

        // Finally, check if its value meets expectation.
        EXPECT_EQ(exp_value, elem->intValue());
    }
};

// This test checks if the parameters can be inherited from the global
// scope to the subnet scope.
TEST_F(SimpleParserTest, deriveParams) {
    ElementPtr global = Element::fromJSON("{ \"renew-timer\": 1,"
                                          "  \"rebind-timer\": 2,"
                                          "  \"preferred-lifetime\": 3,"
                                          "  \"valid-lifetime\": 4"
                                          "}");
    ElementPtr subnet = Element::fromJSON("{ \"renew-timer\": 100 }");

    // we should inherit 3 parameters. Renew-timer should remain intact,
    // as it was already defined in the subnet scope.
    size_t num;
    EXPECT_NO_THROW(num = SimpleParser::deriveParams(global, subnet,
                                                     SAMPLE_INHERITS));
    EXPECT_EQ(3, num);

    // Check the values. 3 of them are inherited, while the fourth one
    // was already defined in the subnet, so should not be inherited.
    checkIntegerValue(subnet, "renew-timer", 100);
    checkIntegerValue(subnet, "rebind-timer", 2);
    checkIntegerValue(subnet, "preferred-lifetime", 3);
    checkIntegerValue(subnet, "valid-lifetime", 4);
}

// This test checks if global defaults are properly set for DHCPv6.
TEST_F(SimpleParserTest, setDefaults) {

    ElementPtr empty = Element::fromJSON("{ }");
    size_t num = 0;

    EXPECT_NO_THROW(num = SimpleParser::setDefaults(empty, SAMPLE_DEFAULTS));

    // We expect at least 4 parameters to be inserted.
    EXPECT_GE(num, 3);

    checkIntegerValue(empty, "valid-lifetime", 7200);
    checkIntegerValue(empty, "preferred-lifetime", 3600);
    checkIntegerValue(empty, "rebind-timer", 1800);
    checkIntegerValue(empty, "renew-timer", 900);
}

// This test checks if global defaults are properly set for DHCPv6.
TEST_F(SimpleParserTest, setListDefaults) {

    ElementPtr empty = Element::fromJSON("[{}, {}, {}]");
    size_t num;

    EXPECT_NO_THROW(num = SimpleParser::setListDefaults(empty, SAMPLE_DEFAULTS));

    // We expect at least 12 parameters to be inserted (3 entries, with
    // 4 parameters inserted in each)
    EXPECT_EQ(12, num);

    ASSERT_EQ(Element::list, empty->getType());
    ASSERT_EQ(3, empty->size());

    ConstElementPtr first = empty->get(0);
    ConstElementPtr second = empty->get(1);
    ConstElementPtr third = empty->get(2);

    checkIntegerValue(first, "valid-lifetime", 7200);
    checkIntegerValue(first, "preferred-lifetime", 3600);
    checkIntegerValue(first, "rebind-timer", 1800);
    checkIntegerValue(first, "renew-timer", 900);

    checkIntegerValue(second, "valid-lifetime", 7200);
    checkIntegerValue(second, "preferred-lifetime", 3600);
    checkIntegerValue(second, "rebind-timer", 1800);
    checkIntegerValue(second, "renew-timer", 900);

    checkIntegerValue(third, "valid-lifetime", 7200);
    checkIntegerValue(third, "preferred-lifetime", 3600);
    checkIntegerValue(third, "rebind-timer", 1800);
    checkIntegerValue(third, "renew-timer", 900);
}
