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

#include "xeus-wren/xinterpreter.hpp"



namespace nl = nlohmann;

namespace xeus_wren
{
 


    void writeFn(WrenVM* vm, const char* text) {
        std::cout<<"stdout: "<< text <<"\n";
        xeus::get_interpreter().publish_stream("stdout", text);
    }

    void errorFn(WrenVM* vm, WrenErrorType errorType,
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
        xeus::get_interpreter().publish_execution_error(errorTypeStr, ss.str(), std::vector<std::string>());
    }


    interpreter::interpreter()
    {
        // create the config that the virtual machine will use
        WrenConfiguration config;

        // fill the config with default values
        wrenInitConfiguration(&config);

        // the custom print and error functions
        config.writeFn = &writeFn;
        config.errorFn = &errorFn;

        // alloc
        p_vm = wrenNewVM(&config);

      

        xeus::register_interpreter(this);
    }

    interpreter::~interpreter()
    {
        wrenFreeVM(p_vm);
    }

    nl::json interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const std::string& code, // Code to execute
                                                      bool /*silent*/,
                                                      bool /*store_history*/,
                                                      nl::json /*user_expressions*/,
                                                      bool /*allow_stdin*/)
    {
        nl::json kernel_res;
        kernel_res["payload"] = nl::json::array();
        kernel_res["user_expressions"] = nl::json::object();

        WrenInterpretResult result = wrenInterpret(p_vm, "main",code.c_str());


        switch (result) {
            case WREN_RESULT_COMPILE_ERROR:
            { 
                kernel_res["status"] = "error";
                this->publish_execution_error("Compile Error!", "Compile Error!", std::vector<std::string>());

            } break;

            case WREN_RESULT_RUNTIME_ERROR:
            { 
                kernel_res["status"] = "error";
                this->publish_execution_error("Runtime Error!", "Compile Error!", std::vector<std::string>());

            } break;

            case WREN_RESULT_SUCCESS:
            { 
                
                kernel_res["status"] = "ok";
            } break;
        }


        nl::json pub_data;
        //pub_data["text/plain"] = code;
        publish_execution_result(execution_counter, std::move(pub_data), nl::json());


        return kernel_res;

    }

    void interpreter::configure_impl()
    {
        // Perform some operations
    }

    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        nl::json result;
        result["status"] = "complete";
        if (code.compare("incomplete") == 0)
        {
            result["status"] = "incomplete";
            result["indent"] = "   ";
        }
        else if(code.compare("invalid") == 0)
        {
            result["status"] = "invalid";
            result["indent"] = "   ";
        }
        return result;
    }
    nl::json interpreter::complete_request_impl(const std::string&  code,
                                                     int cursor_pos)
    {
        nl::json result;

        // Code starts with 'H', it could be the following completion
        if (code[0] == 'H')
        {
            result["status"] = "ok";
            result["matches"] = {
                std::string("Hello"), 
                std::string("Hey"), 
                std::string("Howdy")
            };
            result["cursor_start"] = 5;
            result["cursor_end"] = cursor_pos;
        }
        // No completion result
        else
        {
            result["status"] = "ok";
            result["matches"] = nl::json::array();
            result["cursor_start"] = cursor_pos;
            result["cursor_end"] = cursor_pos;
        }

        return result;
    }

    nl::json interpreter::inspect_request_impl(const std::string& code,
                                                      int /*cursor_pos*/,
                                                      int /*detail_level*/)
    {
        nl::json result;
        result["status"] = "ok";
        result["found"] = true;
        
        result["data"] = {{std::string("text/plain"), std::string("hello!")}};
        result["metadata"] = {{std::string("text/plain"), std::string("hello!")}};
         
        return result;
    }

   
    void interpreter::shutdown_request_impl() {
        std::cout << "Bye!!" << std::endl;
    }


    nl::json interpreter::kernel_info_request_impl()
    {
        nl::json result;
        result["implementation"] = "xwren";
        result["implementation_version"] = XEUS_WREN_VERSION;
        result["banner"] = "xwren";
        result["language_info"]["name"] = "wren";
        result["language_info"]["version"] = "0.4.0";
        result["language_info"]["mimetype"] = "text/plain";
        result["language_info"]["file_extension"] = "wren";
        return result;
    }

}
