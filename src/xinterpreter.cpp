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




    class WrenInfo
    {
    public:
        static inline std::array<std::string,21> const keywords =  
        {
            "as",
            "break",
            "class",
            "construct",
            "continue",
            "else",
            "false",
            "for",
            "foreign",
            "if",
            "import",
            "in",
            "is",
            "null",
            "return",
            "static",
            "super",
            "this",
            "true",
            "var",
            "while",
        };

        inline static bool is_identifier(char c)
        {
            return std::isalpha(c) || std::isdigit(c) || c == '_';
        }
    };



    void write_fn(WrenVM* /*vm*/, const char* text) {
        dynamic_cast<interpreter&>(xeus::get_interpreter()).write_handler(text);
    }

    void error_fn(WrenVM* /*vm*/, WrenErrorType errorType,
             const char* module, const int line,
             const char* msg)
    {
        dynamic_cast<interpreter&>(xeus::get_interpreter()).error_handler(
            errorType, module, line, msg
        );
    }


    interpreter::interpreter()
    {
        // create the config that the virtual machine will use
        WrenConfiguration config;

        // fill the config with default values
        wrenInitConfiguration(&config);

        // the custom print and error functions
        config.writeFn = &write_fn;
        config.errorFn = &error_fn;

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


        std::stringstream codestream; 
        codestream << "{var closure = Meta.compileExpression(\""<<code<<"\")}";

        WrenInterpretResult result = wrenInterpret(p_vm, "main",code.c_str());
        //WrenInterpretResult result = wrenInterpret(p_vm, "main",codestream.str().c_str());


        switch (result) {
            case WREN_RESULT_COMPILE_ERROR:
            { 
                kernel_res["status"] = "error";
                kernel_res["ename"] = "Compile Error";
                kernel_res["evalue"] = "Compile Error";
                kernel_res["traceback"] = nl::json::array();
                this->publish_execution_error("Compile Error!", "Compile Error!", std::vector<std::string>());

            } break;

            case WREN_RESULT_RUNTIME_ERROR:
            { 
                kernel_res["status"] = "error";
                kernel_res["ename"] = "Runtime Error";
                kernel_res["evalue"] = "Runtime Error";
                kernel_res["traceback"] = nl::json::array();
                this->publish_execution_error("Runtime Error", "Compile Error!", std::vector<std::string>());

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
        // /wrenInterpret(p_vm, "main","import meta for Meta");
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
    nl::json interpreter::complete_request_impl(const std::string&  raw_code,
                                                     int cursor_pos)
    {
        nl::json result;



        nl::json matches = nl::json::array();

        // first we get  a substring from string[0:curser_pos+1]std
        // and discard the right side of the curser pos
        const auto code = raw_code.substr(0, cursor_pos);

        // keyword matches
        // ............................
        {
            auto pos = -1;
            for(auto i=code.size()-1; i>=0; --i)
            {   
                if(!WrenInfo::is_identifier(code[i]))
                {
                    pos = i;
                    break;
                }
            }
            result["cursor_start"] =  pos == -1 ? 0 : pos +1;
            auto to_match = pos == -1 ? code : code.substr(pos+1, code.size() -(pos+1));

            // check for kw matches
            for(auto kw : WrenInfo::keywords)
            {
                if(startswith(kw, to_match))
                {
                    matches.push_back(kw);
                }
            }

        }


        result["status"] = "ok";
        result["cursor_end"] = cursor_pos;
        result["matches"] =matches;

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
        result["language_info"]["mimetype"] = "text/x-wren";
        result["language_info"]["file_extension"] = "wren";
        result["language_info"]["codemirror_mode"] = "wren";
        return result;
    }


    void interpreter::write_handler(const char* text)
    {
        this->publish_stream("stdout", text);
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
