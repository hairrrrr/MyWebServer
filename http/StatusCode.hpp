#pragma once

#include <string>
#include <unordered_map>

enum 
{
    OK = 200, No_Content = 204, 
    Moved_Permanently = 301,  Found = 302, 
    Bad_Request = 400, Forbidden = 403, Not_Found = 404, 
    Internal_Server_Error = 500, Not_Implemented = 501, 
    Bad_Gateway = 502, Service_Unavailable = 503
};

struct HttpStatus
{
    int _status_code;
    std::string _desc;
    std::string _page_path;
};

