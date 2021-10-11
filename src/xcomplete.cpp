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

    nl::json interpreter::complete_request_impl(const std::string&  raw_code,
                                                     int cursor_pos)
    {
        nl::json result;

        nl::json matches = nl::json::array();

        // first we get  a substring from string[0:curser_pos+1]std
        // and discard the right side of the curser pos
        const auto code = raw_code.substr(0, cursor_pos);

        // there are two modes for matching:
        // 1) member matching
        // 2) keyword matches + globals
        
        // atm we only implement the kw part of the second mode 

        // keyword matches
        // ............................
        {
            auto pos = -1;
            for(int i=code.size()-1; i>=0; --i)
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

}