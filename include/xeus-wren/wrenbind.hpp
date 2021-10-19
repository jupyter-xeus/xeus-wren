#include <memory>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <functional>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <unordered_map>
#include <vector>

#include <string>
#include <vector>
#include <sstream>

#include <stdio.h>
#include <string.h>

extern "C" {
#include <wren.h>
}


namespace xwren
{   

    class xforeign_function{
        public:
            xforeign_function(WrenForeignMethodFn f)
            :   m_f(f){
            }
        private:
            WrenForeignMethodFn m_f;
    };

    class xfunction{};  

    class xclass{
        public:
            void add_static(const std::string & signature, const std::string & body)
            {
                m_pure_static_functions[signature] = body;
            }
            void add_static(const xforeign_function & /*xforeign_function*/)
            {
                //m_pure_functions[signature] = body;
            }
        private:
            std::map<std::string, std::string> m_pure_static_functions;
            std::map<std::string, xforeign_function> m_m_foreign_static_functions;
    };

    class xforeign_class{};

    class xmodule{
    public:
        xmodule()
        : m_src(""){}
        xclass & add_class(const std::string & class_name){
            m_classes[class_name] = xclass{};
            return m_classes[class_name];
        }

        void add_src(const std::string & code){
            m_src += "\n";
            m_src += code;
        }
        auto render()const{
            return m_src;
        }
    private:    
        std::string m_src;
        std::map<std::string, xclass>        m_classes;
        std::map<std::string, xforeign_class> m_forein_classes;
    };


    class xvm
    {
    public:
        xvm()
        {
            wrenInitConfiguration(&m_config);
            m_config.userData = this;
            m_config.loadModuleFn = [](WrenVM* vm, const char* name) -> WrenLoadModuleResult {
                auto& self = xvm::get_self(vm);
                return self.load_moulde(name);
            };
            m_config.bindForeignClassFn = [](WrenVM* vm, const char* module, const char* class_name) -> WrenForeignClassMethods {
                auto& self = xvm::get_self(vm);
                return self.bind_foreign_class_fn(module, class_name);
            };
            m_config.bindForeignMethodFn = [](WrenVM* vm, const char* module, const char* class_name, bool is_static, const char* signature) -> WrenForeignMethodFn {
                auto& self = xvm::get_self(vm);
                return self.bind_foreign_method_fn(module, class_name, is_static, signature);
            };
            m_config.writeFn = [](WrenVM* vm, const char* text){
                auto& self = xvm::get_self(vm);
                self.write_fn(text);
            };
            m_config.errorFn = [](WrenVM* vm, WrenErrorType errorType,
             const char* module, const int line,
             const char* msg){
                auto& self = xvm::get_self(vm);
                self.error_fn(errorType, module, line, msg);
            };

            // alloc
            p_vm.reset(wrenNewVM(&m_config));

        }
        xmodule & add_module(const std::string & module_name){
            m_modules[module_name] = xmodule{};
            return m_modules.find(module_name)->second;
        }

        auto interpret(const std::string & src){
            WrenInterpretResult result = wrenInterpret(p_vm.get(), "main", src.c_str());
            return result;
        }
        template<class F>
        void set_write_function(F && f){
            m_write_function = f;
        }
    private:
        static xvm & get_self(WrenVM* vm){
            return *reinterpret_cast<xvm*>(wrenGetUserData(vm));
        }
        void write_fn(const char* txt){
            if(m_write_function)
            {
                m_write_function(std::string(txt));
            }
        }

        void error_fn(WrenErrorType errorType,
             const char* module, const int line,
             const char* msg)
        {
            std::stringstream ss;
            switch (errorType)
            {
                case WREN_ERROR_COMPILE:
                {
                    ss << "WREN_ERROR_COMPILE\n";
                    ss<<"["<< module <<" "<< line <<"] [Error] "<< msg<<"\n";
                } break;

                case WREN_ERROR_STACK_TRACE:
                {
                    ss << "WREN_ERROR_STACK_TRACE\n";
                    ss<<"["<< module <<" "<< line <<"] [Error] "<< msg<<"\n";
                } break;

                case WREN_ERROR_RUNTIME:
                {
                    ss << "WREN_ERROR_RUNTIME\n";
                    ss<<"[Runtime Error] "<<msg<<"\n";
                } break;
            }
            std::cout<<"stderr: "<< ss.str() <<"\n";
        }


        WrenLoadModuleResult load_moulde(const char* name){
            WrenLoadModuleResult result = {0,0,0};
            if(auto find_res = m_modules.find(std::string(name)); find_res != m_modules.end())
            {
                const auto & src = find_res->second.render();
                auto buffer = new char[src.size() + 1];
                std::memcpy(buffer, &src[0], src.size() + 1);
                result.source = buffer;
            }
            return result;
        }


        WrenForeignClassMethods bind_foreign_class_fn(const char* module, const char* class_name){
            std::cout<<"bind_foreign_class_fn: "<<module<<" "<<class_name<<"\n";
            WrenForeignClassMethods methods;
            // Unknown class.
            methods.allocate = NULL;
            methods.finalize = NULL;
            return methods;
        }

        WrenForeignMethodFn bind_foreign_method_fn(
            const char* module,
            const char* class_name,
            bool is_static,
            const char* signature)
        {
            std::cout<<"bind_foreign_method_fn: "<<module<<" "<<class_name<<" "<<is_static<<" "<<signature<<"\n";

            return nullptr;
        }

        //Custom Resource deleter
        struct Deleter {
            void operator()(WrenVM* vm) {
            if(vm != nullptr)
                wrenFreeVM(vm);
            }
        };

        explicit operator WrenVM*() const { return p_vm.get(); }

        std::unique_ptr<WrenVM, Deleter>  p_vm;
        WrenConfiguration m_config;

        std::map<std::string, xmodule>        m_modules;

        std::function<void(const std::string &)> m_write_function;
    };   
};