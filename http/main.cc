#include <iostream>
#include <memory>

#include <assert.h>

#include "HttpServer.hpp"
#include "../log.hpp"

#define MODE DEBUG_MODE

// #define ERROR -1
// #define INFO 1

// #define LOG(err) log(#err, err, __LINE__);

// void log(std::string err_name, int err_code, int line)
// {
//     std::cout << err_name << " " << err_code << " line: " << line << std::endl;
// }

int main()
{
    HttpServer http(8080);

    http.Run();

    return 0;
}