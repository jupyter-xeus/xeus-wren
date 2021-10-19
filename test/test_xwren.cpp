/***************************************************************************
* Copyright (c) 2021, Thorsten Beier                                       *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "doctest/doctest.h"
#include "xeus-wren/wrenbind.hpp"

namespace xeus
{
    
    TEST_SUITE("xwrenbind")
    {
        TEST_CASE("test_write_fn")
        {
            xwren::xvm vm;

            // auto & fubar_mod = vm.add_module("fubar");
            // auto & fubar_cls = fubar_mod.add_class("Fubar");
            // fubar_cls.add_static("bar(a)","return 2*a");


            std::string write_res;
            vm.set_write_function([&](const std::string & txt){
                write_res  += txt;
            });

            auto result = vm.interpret("System.print(\"hello_world\")");
            CHECK_EQ(result, WREN_RESULT_SUCCESS);
            CHECK(write_res == std::string("hello_world\n"));
        }

        TEST_CASE("load_moulde")
        {
            xwren::xvm vm;

            auto & fubar_mod = vm.add_module("fubar");
            fubar_mod.add_src(R"(
            class Fubar{
                static bar(){
                    return "hello"
                }
            })");

            auto result = vm.interpret(R"(import "fubar" for Fubar)");
            CHECK_EQ(result, WREN_RESULT_SUCCESS);
        }
    }
}

