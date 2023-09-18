#include <iostream>
#include <cstdlib>
#include <string>

int main()
{
    char* str_c;
    {
        std::string str("METHOD=GET");
        str_c = (char*)str.c_str();
        //putenv(str_c);
        setenv("METHOD", str_c, 1);
    }

    *str_c = 0;    

    const char* env = getenv("METHOD");
    if( env == nullptr )
        std::cout << "METHOD is empty!" << std::endl;
    else
    {
        std::cout << env << std::endl;
    }
    
    env = getenv("ARGUMENTSIZE");
    if( env == nullptr )
        std::cout << "ARGUMENTSIZE is empty!" << std::endl;
    else
        std::cout << env << std::endl; 
}