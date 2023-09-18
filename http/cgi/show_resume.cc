#include "CgiUtil.hpp"


#define RESUME_SUFFIX "_resume.html"

int main()
{
    std::string argument;
    if( !CgiUtil::GetArgument(&argument) )
        exit(2);
    std::cerr << "CGI: argument =  " << argument << std::endl;

    std::vector<std::string> args;
    HttpUtil::CutString(argument, ARGUMENT_SEP, &args);
    if( args.size() != 1 )
        exit(3);    

    std::cerr <<  "CGI: " << args[0] << "\n";

    std::string name;
    if ( !CgiUtil::GetArgValue(args[0], &name))
        exit(4);
    
    std::cerr << "CGI: " << name << "\n";
        
    name += RESUME_SUFFIX;
    name = WEBROOT + name;

    int resume = open(name.c_str(), O_RDONLY);
    if( resume < 0)
    {
        std::cerr << "Cgi: open render file failed! file name:" << name  << " " << strerror(errno) << "\n"; 
        exit(5);
    }

    char buffer[1024];
    ssize_t cnt;
    while( ( cnt = read(resume, buffer, sizeof(buffer) - 1) ) > 0 )
    {
        buffer[cnt] = 0;
        std::cerr << buffer;
        std::cout << buffer;
    }        

    std::cerr << "CGI: handler ok!\n";

    return 0;
}