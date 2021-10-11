/***************************************************************************
* Copyright (c) 2021,                                          
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

// #include <string>

#include "nlohmann/json.hpp"
#include "xeus-wren/xinterpreter.hpp"
#include "xeus/xinterpreter.hpp"

extern "C" {
#include <wren.h>
}

namespace nl = nlohmann;

namespace xeus_wren
{   

    void clear_output(WrenVM* vm)
    {   
        auto & self = xeus::get_interpreter();
        self.clear_output(false);
    }
    void clear_output_wait(WrenVM* vm)
    {   
        auto & self = xeus::get_interpreter();
        const bool wait = wrenGetSlotString(vm, 1);
        self.clear_output(wait);
    }
    void display_data(WrenVM* vm)
    {   
        auto & self = xeus::get_interpreter();

        const char * data_str = wrenGetSlotString(vm, 1);
        const char * metadata_str = wrenGetSlotString(vm, 2);
        const char * transient_str = wrenGetSlotString(vm, 3);
        try
        {
            const auto data = nl::json::parse(data_str);
            const auto metadata = nl::json::parse(metadata_str);
            const auto transient = nl::json::parse(transient_str);
            self.display_data(data, metadata, transient);
        }
        catch (nl::json::parse_error& ex)
        {
            self.publish_execution_error("json::parse_error",ex.what(),std::vector<std::string>());

        }
    }

}