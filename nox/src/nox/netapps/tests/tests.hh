/* Copyright 2008 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CONTROLLER_TESTS_HH
#define CONTROLLER_TESTS_HH 1

#include <string>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>

#include "component.hh"
#include "kernel.hh"
#include "static-deployer.hh"

namespace vigil {
namespace testing {

/*
 * Test cases requiring access to other components should inherit
 * TestComponent class and be registered with the
 * BOOST_AUTO_COMPONENT_TEST_CASE macre below.
 */

class Test_component
    : public container::Component {
public:
    Test_component(const container::Context* c)
        : Component(c) { } 
    
    virtual void run_test() = 0;
};

} // namespace testing
} // namespace vigil

#define BOOST_AUTO_COMPONENT_TEST_CASE(COMPONENT_NAME, COMPONENT_CLASS) \
    BOOST_AUTO_TEST_CASE(COMPONENT_NAME) {                              \
        using namespace vigil::container;                               \
        static Interface_description                                    \
            i(typeid(COMPONENT_CLASS).name());                          \
        static Simple_component_factory<COMPONENT_CLASS> cf(i);         \
        Kernel* kernel = Kernel::get_instance();                        \
        try {                                                           \
            kernel->install(new vigil::Static_component_context         \
                            (kernel, #COMPONENT_NAME, &cf, 0),          \
                            INSTALLED);                                 \
            Component_context* ctxt =                                   \
                (kernel->get(#COMPONENT_NAME, INSTALLED));              \
            BOOST_REQUIRE(ctxt);                                        \
            vigil::testing::Test_component* tc =                        \
                dynamic_cast<vigil::testing::Test_component*>           \
                (ctxt->get_instance());                                 \
            BOOST_REQUIRE(tc);                                          \
            tc->run_test();                                             \
        } catch (const std::runtime_error& e) {                         \
            BOOST_REQUIRE_MESSAGE(0, "Unable to install :"              \
                                  #COMPONENT_NAME);                     \
        }                                                               \
    }

#endif
