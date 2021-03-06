// Copyright (C) 2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cc/simple_parser.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <cc/data.h>
#include <string>

using namespace std;

namespace isc {
namespace data {

std::string
SimpleParser::getString(isc::data::ConstElementPtr scope, const std::string& name) {
    ConstElementPtr x = scope->get(name);
    if (!x) {
        isc_throw(BadValue, "Element " << name << " not found");
    }
    if (x->getType() != Element::string) {
        isc_throw(BadValue, "Element " << name << " found, but is not a string");
    }

    return (x->stringValue());
}

int64_t
SimpleParser::getInteger(isc::data::ConstElementPtr scope, const std::string& name) {
    ConstElementPtr x = scope->get(name);
    if (!x) {
        isc_throw(BadValue, "Element " << name << " not found");
    }
    if (x->getType() != Element::integer) {
        isc_throw(BadValue, "Element " << name << " found, but is not an integer");
    }

    return (x->intValue());
}

bool
SimpleParser::getBoolean(isc::data::ConstElementPtr scope, const std::string& name) {
    ConstElementPtr x = scope->get(name);
    if (!x) {
        isc_throw(BadValue, "Element " << name << " not found");
    }
    if (x->getType() != Element::boolean) {
        isc_throw(BadValue, "Element " << name << " found, but is not a boolean");
    }

    return (x->boolValue());
}

const data::Element::Position&
SimpleParser::getPosition(const std::string& name, const data::ConstElementPtr parent) {
    if (!parent) {
        return (data::Element::ZERO_POSITION());
    }
    ConstElementPtr elem = parent->get(name);
    if (!elem) {
        return (data::Element::ZERO_POSITION());
    }
    return (elem->getPosition());
}

size_t SimpleParser::setDefaults(isc::data::ElementPtr scope,
                                 const SimpleDefaults& default_values) {
    size_t cnt = 0;

    // This is the position representing a default value. As the values
    // we're inserting here are not present in whatever the config file
    // came from, we need to make sure it's clearly labeled as default.
    const Element::Position pos("<default-value>", 0, 0);

    // Let's go over all parameters we have defaults for.
    BOOST_FOREACH(SimpleDefault def_value, default_values) {

        // Try if such a parameter is there. If it is, let's
        // skip it, because user knows best *cough*.
        ConstElementPtr x = scope->get(string(def_value.name_));
        if (x) {
            // There is such a value already, skip it.
            continue;
        }

        // There isn't such a value defined, let's create the default
        // value...
        switch (def_value.type_) {
        case Element::string: {
            x.reset(new StringElement(def_value.value_, pos));
            break;
        }
        case Element::integer: {
            int int_value = boost::lexical_cast<int>(def_value.value_);
            x.reset(new IntElement(int_value, pos));
            break;
        }
        case Element::boolean: {
            bool bool_value;
            if (def_value.value_ == string("true")) {
                bool_value = true;
            } else if (def_value.value_ == string("false")) {
                bool_value = false;
            } else {
                isc_throw(BadValue, "Internal error. Boolean value specified as "
                          << def_value.value_ << ", expected true or false");
            }
            x.reset(new BoolElement(bool_value, pos));
            break;
        }
        case Element::real: {
            double dbl_value = boost::lexical_cast<double>(def_value.value_);
            x.reset(new DoubleElement(dbl_value, pos));
            break;
        }
        default:
            // No default values for null, list or map
            isc_throw(BadValue, "Internal error. Incorrect default value type.");
        }

        // ... and insert it into the provided Element tree.
        scope->set(def_value.name_, x);
        ++cnt;
    }

    return (cnt);
}

size_t
SimpleParser::setListDefaults(isc::data::ElementPtr list,
                              const SimpleDefaults& default_values) {
    size_t cnt = 0;
    BOOST_FOREACH(ElementPtr entry, list->listValue()) {
        cnt += setDefaults(entry, default_values);
    }
    return (cnt);
}

size_t
SimpleParser::deriveParams(isc::data::ConstElementPtr parent,
                           isc::data::ElementPtr child,
                           const ParamsList& params) {
    if ( (parent->getType() != Element::map) ||
         (child->getType() != Element::map)) {
        return (0);
    }

    size_t cnt = 0;
    BOOST_FOREACH(string param, params) {
        ConstElementPtr x = parent->get(param);
        if (!x) {
            // Parent doesn't define this parameter, so there's
            // nothing to derive from
            continue;
        }

        if (child->get(param)) {
            // Child defines this parameter already. There's
            // nothing to do here.
            continue;
        }

        // Copy the parameters to the child scope.
        child->set(param, x);
        cnt++;
    }

    return (cnt);
}

}; // end of isc::dhcp namespace
}; // end of isc namespace
