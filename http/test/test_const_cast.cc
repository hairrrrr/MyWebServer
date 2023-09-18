#include <iostream>

int main()
{
    const int x = 10;

    // const_cast 中的类型必须是指针、引用或指向对象类型成员的指针
    const_cast<int>(x) = 11;

    return 0;
}