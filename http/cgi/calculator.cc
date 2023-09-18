#include <signal.h>

#include <iostream>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <cmath>

#include "CgiUtil.hpp"

#define RESULT_PAGE ".result.html"
#define EPISLON     1e-8


int main()
{
    std::string argument;
    if( ! CgiUtil::GetArgument(&argument) ) exit(2);
    std::cerr << "CGI: argument =  " << argument << std::endl;

    std::vector<std::string> args;
    if( !HttpUtil::CutString(argument, ARGUMENT_SEP, &args) ) exit(3);

    if( args.size() != 3 ) exit(4);

    std::string value;
    
    if( !CgiUtil::GetArgValue(args[0], &value) ) exit(5);
    double a = std::stod(value);
    
    if( !CgiUtil::GetArgValue(args[1], &value) ) exit(6);
    char op_host = CgiUtil::GetHttpOperator(value);
    if( op_host == ' ' ) exit(7);
    
    if( !CgiUtil::GetArgValue(args[2], &value) ) exit(8);
    double b = std::stod(value);
    
    std::cerr << "CGI: a: " << a << " op: " << op_host << " b: " << b << std::endl;

    double result;
    switch( op_host )
    {
    case '+': 
        result = a + b;
        break;
    case '-':
        result = a - b;
        break;
    case '*':
        result = a * b;
        break;
    case '/':
        if( fabs(b - 0) < EPISLON )
            raise(SIGFPE);
        result = a / b;
        break;
    case '%':
        if( fabs(b - 0) < EPISLON )
            raise(SIGFPE);
        a = (long long)a;
        b = (long long)b;
        result = (long long)a % (long long)b;
        break;
    default:
        exit(9);
    }
    
    std::string result_path = WEBROOT;
    result_path += ".result.html";
    
    // httpserver 调用
    int render_fd = open(result_path.c_str(), O_RDONLY);
    if( render_fd < 0)
    {
        std::cerr << "Cgi: open render file failed " << strerror(errno) << "\n"; 
        exit(10);
    }

    char buffer[1024];
    ssize_t cnt;
    while( ( cnt = read(render_fd, buffer, sizeof(buffer) - 1) ) > 0 )
    {
        buffer[cnt] = 0;
        std::cout << buffer;
    }


    std::cout << "\n<body>";
    std::cout << "\n<h2> " << a << " " << op_host << " " << b << " = "<< result << "</h2>";
    std::cout << "\n</body>";
    std::cout << "\n</html>";

    std::cout.unsetf(std::ios::fixed);

    return 0;
}