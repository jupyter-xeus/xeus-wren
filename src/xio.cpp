/***************************************************************************
* Copyright (c) 2021,                                          
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"

#include "xeus-wren/xinterpreter.hpp"

#include "xstring.hpp"

namespace xeus_wren
{
    void write_fn(WrenVM* /*vm*/, const char* text)
    {
        dynamic_cast<interpreter&>(xeus::get_interpreter()).write_handler(text);
    }
    
    void blocking_input_request(WrenVM* vm)
    {   
        const auto s = xeus::blocking_input_request("", false);
        wrenSetSlotString(vm, 0, s.c_str());
    }
}
