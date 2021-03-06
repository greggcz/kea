// Copyright (C) 2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SIMPLE_PARSER6_H
#define SIMPLE_PARSER6_H

#include <cc/simple_parser.h>

namespace isc {
namespace dhcp {

/// @brief SimpleParser specialized for DHCPv6
///
/// This class is a @ref isc::data::SimpleParser dedicated to DHCPv6 parser.
/// In particular, it contains all the default values and names of the
/// parameters that are to be derived (inherited) between scopes.
/// For the actual values, see @file simple_parser6.cc
class SimpleParser6 : public isc::data::SimpleParser {
public:

    /// @brief Sets all defaults for DHCPv6 configuration
    ///
    /// This method sets global, option data and option definitions defaults.
    ///
    /// @param global scope to be filled in with defaults.
    /// @return number of default values added
    static size_t setAllDefaults(isc::data::ElementPtr global);

    // see simple_parser6.cc for comments for those parameters
    static const isc::data::SimpleDefaults OPTION6_DEF_DEFAULTS;
    static const isc::data::SimpleDefaults OPTION6_DEFAULTS;
    static const isc::data::SimpleDefaults GLOBAL6_DEFAULTS;
    static const isc::data::ParamsList INHERIT_GLOBAL_TO_SUBNET6;
};

};
};

#endif
