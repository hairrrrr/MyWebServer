#include <string>
#include <unordered_map>
#include <iostream>

#define CONTENT_LENGTH "Content-Length"

// class HttpRequest 
// {
// public:
//     std::string _request_line;
//     std::unordered_map<std::string, std::string> _request_header;
//     std::string _request_body;
//     size_t GetBodySize() const 
//     {
//         auto it = _request_header.find(std::string(CONTENT_LENGTH));
//         return std::stoi(it->second);
//     }
// };

#include <algorithm>

int main()
{
    std::string str1 = "ni hao";
    std::string str2;
    str2.resize(str1.size()); 
    std::transform(str1.begin(), str1.end(), str2.begin(), toupper);
    std::cout << str1 << " " << str2 << "\n";

    return 0;
}