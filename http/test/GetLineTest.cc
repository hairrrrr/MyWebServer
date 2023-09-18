#include "../TcpServer.hpp"
#include "../HttpServer.hpp"

int main()
{
    TcpServer* tcp = TcpServer::GetInstance(8080);

    tcp->Init();
    
    while(true)
    {
        int sock = tcp->GetNewLink();

        std::string str;
        do 
        {
            str.clear();

            if( !HttpUtil::GetLine(sock, &str) )
            break;
        
            if( str.empty() )
                std::cout << "empty line" << std::endl;
            else 
                std::cout << str << std::endl;
        }while(str.size());
        
        HttpUtil::GetBlock(sock, &str, 10);

        std::cout << "body: " << str << std::endl;

        close(sock);
    }

    return 0;
}