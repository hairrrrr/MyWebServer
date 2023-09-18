#include <iostream>
#include <unistd.h>

int main()
{
    char cwd[1024];
    
    if( getcwd(cwd, 1024) != nullptr )
        std::cout << cwd << "\n";
    else 
    {
        perror("getcwd");        
        return 1;
    }

    return 0;
}