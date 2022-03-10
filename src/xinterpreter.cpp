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

    // implemenyed in xdisplay.cpp
    void display_data(WrenVM* vm);
    void clear_output(WrenVM* vm);
    void clear_output_wait(WrenVM* vm);

    // implemented in xjson.cpp
    void json_encode_str(WrenVM* vm);

    // implemented in xio.cpp
    void write_fn(WrenVM* /*vm*/, const char* text);
    void error_fn(WrenVM* /*vm*/, WrenErrorType errorType,
                  const char* module, const int line,
                  const char* msg);
    void blocking_input_request(WrenVM* vm);

    WrenLoadModuleResult load_module_fn(WrenVM* vm, const char* name) 
    {
        WrenLoadModuleResult result = {0};
        if (strcmp(name, "iwren") == 0)
        {
            result.source = R"""(
            class Stdin{
                foreign static readLine()   
            }
            class JsonEncode{
                foreign static encodeStr(str)   

                static encode(obj) {
                    if (obj is Num || obj is Bool) {
                        return obj.toString
                    } else if (obj is String) {
                        return encodeStr(obj)
                    } else if (obj is List) {
                        var encodedItems = obj.map { |o|  encode(o) }
                        return "[" + encodedItems.join(",") + "]"
                    } else if (obj is Map) {
                        var encodedItems = obj.keys.map { |key|
                            return encode(key) + ":" + encode(obj[key])
                        }
                        return "{" + encodedItems.join(",") + "}"
                    } else{
                        Fiber.abort("called encode with object which does not support json encoding")
                    }
                }
            }
            class Display{
                static display(obj){
                    return JsonEncode.encode(obj)
                }

                foreign static display_data(a,b,c)
                foreign static clear_output()
                foreign static clear_output(w)


                static display_mimetype(mimetype, data){
                    var obj = {
                        mimetype : data
                    }
                    display_data(JsonEncode.encode(obj), "{}", "{}")
                }

                static display_html(data){
                   display_mimetype("text/html", data)
                }
                static display_json(data){
                   display_mimetype("application/json", data)
                }
                static display_plain_text(data){
                   display_mimetype("text/plain", data)
                }
                static display_latex(data){
                   display_mimetype("text/latex", data)
                }
            }
            )""";
        }
        return result;
    }


    WrenForeignMethodFn bind_foreign_method_fn(
        WrenVM* /*vm*/,
        const char* module,
        const char* class_name,
        bool isStatic,
        const char* signature)
    {

        auto & self = dynamic_cast<interpreter&>(xeus::get_interpreter());
        auto & forein_methods = self.m_forein_methods;

        if(isStatic)
        {
            if(auto mod_iter = forein_methods.find(module); mod_iter!= forein_methods.end())
            {
                if(auto class_iter = mod_iter->second.find(class_name); class_iter!=mod_iter->second.end())
                {
                    if(auto func_iter = class_iter->second.find(signature); func_iter!=class_iter->second.end())
                    {
                        return func_iter->second;
                    }
                }
            }
        }
        return nullptr;
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
        config.bindForeignMethodFn = &bind_foreign_method_fn;
        config.loadModuleFn = &load_module_fn;
        
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
        codestream << "{var closure = Meta.compileExpression(\"" << code << "\")}";

        WrenInterpretResult result = wrenInterpret(p_vm, "main", code.c_str());
        //WrenInterpretResult result = wrenInterpret(p_vm, "main", codestream.str().c_str());

        switch (result)
	{
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
        m_forein_methods["iwren"]["Stdin"]["readLine()"] = blocking_input_request;
        m_forein_methods["iwren"]["JsonEncode"]["encodeStr(_)"] =  json_encode_str;
        m_forein_methods["iwren"]["Display"]["display_data(_,_,_)"] =  xeus_wren::display_data;
        m_forein_methods["iwren"]["Display"]["clear_output()"] =  xeus_wren::clear_output;
        m_forein_methods["iwren"]["Display"]["clear_output(_)"] =  xeus_wren::clear_output_wait;
        wrenInterpret(p_vm, "main",R"""(
        )""");
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
        std::cerr << "shuting down xwren" << std::endl;
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
        result["status"] = "ok";
        return result;
    }

    void interpreter::write_handler(const char* text)
    {
        this->publish_stream("stdout", text);
    }
}
