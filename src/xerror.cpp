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

namespace nl = nlohmann;



namespace xeus_wren
{
 
    struct xwren_error
    {
        std::string module;
    };


    void error_fn(WrenVM* /*vm*/, WrenErrorType errorType,
             const char* module, const int line,
             const char* msg)
    {
        dynamic_cast<interpreter&>(xeus::get_interpreter()).error_handler(
            errorType, module, line, msg
        );
    }

    void interpreter::error_handler(WrenErrorType errorType,
         const char* module, const int line,
         const char* msg)
    {
        std::string errorTypeStr;
        std::stringstream ss;
        switch (errorType)
        {
            case WREN_ERROR_COMPILE:
            {
                errorTypeStr = "WREN_ERROR_COMPILE";
                ss<<"["<< module <<" "<< line <<"] [Error] "<< msg<<"\n";
            } break;

            case WREN_ERROR_STACK_TRACE:
            {
                errorTypeStr = "WREN_ERROR_STACK_TRACE";
                ss<<"["<< module <<" "<< line <<"] [Error] "<< msg<<"\n";
            } break;

            case WREN_ERROR_RUNTIME:
            {
                errorTypeStr = "WREN_ERROR_RUNTIME";
                ss<<"[Runtime Error] "<<msg<<"\n";
            } break;
        }
        std::cout<<"stderr: "<< ss.str() <<"\n";
        this->publish_execution_error(errorTypeStr, ss.str(), std::vector<std::string>());
    }


}