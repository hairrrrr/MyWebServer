#include <iostream>
#include <memory>

int main()
{
    std::unique_ptr<int> ptr(new int(10));
    std::unique_ptr<int> ptr2(std::move(ptr));    

    return 0;
}