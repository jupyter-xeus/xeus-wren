/***************************************************************************
* Copyright (c) 2021,                                          
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "nlohmann/json.hpp"

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"

#include "xeus/xinterpreter.hpp"
#include "xeus-wren/xinterpreter.hpp"
#include "xstring.hpp"


#ifdef XEUS_WREN_EMSCRIPTEN_WASM_BUILD
#include <emscripten.h>
#endif

namespace nl = nlohmann;


#ifdef XEUS_WREN_EMSCRIPTEN_WASM_BUILD

EM_JS(char *, async_get_input_function, (const char* str), {
  return Asyncify.handleAsync(function () {
    return self.async_get_input_function( UTF8ToString(str))
    .then(function (jsString) {
      var lengthBytes = lengthBytesUTF8(jsString)+1;
      var stringOnWasmHeap = _malloc(lengthBytes);
      stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
      return stringOnWasmHeap;
    });
  });
});

#endif


namespace xeus_wren
{

    void write_fn(WrenVM* /*vm*/, const char* text)
    {
        dynamic_cast<interpreter&>(xeus::get_interpreter()).write_handler(text);
    }
    
    void blocking_input_request(WrenVM* vm)
    {   
        #ifdef XEUS_WREN_EMSCRIPTEN_WASM_BUILD
        char* str = async_get_input_function("");
        const std::string s(str);
        free(str);
        #else 
        const auto s = xeus::blocking_input_request("", false);
        #endif
        wrenSetSlotString(vm, 0, s.c_str());
    }

}