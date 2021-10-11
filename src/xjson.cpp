/***************************************************************************
* Copyright (c) 2021,                                          
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

// #include <string>

#include "nlohmann/json.hpp"

extern "C" {
#include <wren.h>
}

namespace nl = nlohmann;

namespace xeus_wren
{   
    void json_encode_str(WrenVM* vm)
    {   
        const char * unencoded = wrenGetSlotString(vm, 1);
        nl::json j = unencoded;
        std::string encoded = j.dump();
        wrenSetSlotString(vm, 0, encoded.c_str());
    }

}