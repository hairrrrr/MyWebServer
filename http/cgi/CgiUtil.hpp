#pragma once 

#include <string>
#include <iostream>
#include "../HttpUtil.hpp"

#include <string>
#include <unordered_map>

#define WEBROOT "wwwroot/"

class CgiUtil 
{
public:
    static inline bool GetArgument(std::string* argument)
    {
        std::string method;
        if( !GetEnv("METHOD", &method) )
            return false;

        std::cerr << "CGI: METHOD = " << method << "\n";
        
        if( method == "GET" )
        {
            if( !GetEnv("ARGUMENT", argument) )
                return false;
        }   
        else if( method == "POST" )
        {
            std::string out;
            while( HttpUtil::ReadPipe(STDIN_FILENO, &out, 1) ) 
                *argument += out;
            if( argument->empty() )
                return false;
        }

        return true;
    }

    static inline char GetHttpOperator(const std::string& http_format)
    {
        std::unordered_map<std::string, char> operators = {
            {"%2B", '+'}, {"-", '-'}, {"*", '*'}, {"%2F", '/'}, {"%25", '%'}
        };

        if( operators.find(http_format) == operators.end() )
            return ' ';

        return operators[http_format];
    }

    static inline bool GetArgValue(const std::string& arg_pair, std::string* value )
    {
        std::vector<std::string> kv;
        if( !HttpUtil::CutString(arg_pair, KEY_VALUE_SEP, &kv) )
            return false;
        *value = kv[1];
        return true;
    }

private:
    static inline bool GetEnv(const char* env_name, std::string* str)
    {
        const char* env_value = getenv(env_name);
        if( env_value == nullptr )
        {
            std::cerr << "CGI: getenv METHOD failed!\n";
            return false;
        }
        
        *str = env_value;
        
        return true;
    }
};

