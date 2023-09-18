#include "../Util.hpp"

int main()
{
    std::string str1 = "GET /a/b/c?a=100 HTTP/1.1";
    std::string str2 = "Content-Length: 392";

    std::vector<std::string> vec1, vec2;

    HttpUtil::CutString(str1, " ", &vec1);
    HttpUtil::CutString(str2, ": ", &vec2);            

    for(auto& str : vec1)
        std::cout << str << "\n";

    for(auto& str : vec2)
        std::cout << str << "\n";

    return 0;
}