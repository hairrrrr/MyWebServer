#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <functional>


#include "../../log.hpp"
#include "../TcpServer.hpp"
#define LOG_MODE DEBUG_MODE

using write_block_handler = std::function<ssize_t(int, const void*, size_t)>;

bool WriteBlockToBuffer(int, const std::string&,  write_block_handler);

bool WritePipe(int fd, const std::string& block)
{
   return WriteBlockToBuffer(fd, block, write); 
}

bool SendBlock(int sock, const std::string& block)
{
    using namespace std::placeholders;
    return WriteBlockToBuffer(sock, block, std::bind(send, _1, _2, _3, 0));
}

bool WriteBlockToBuffer(int fd, const std::string& block,  write_block_handler handler)
{
    if( block.empty() )
    {
        LOGMESSAGE(WARNING, LOG_MODE, "buffer is empty!");
        return false;
    }

    const char* buf = block.c_str(); 
    size_t total = block.size();
    size_t sent  = 0;
    ssize_t cnt;

    // cnt = send(sock, buf + sent, total - sent, 0);
    while( (sent < total) && ( (cnt = handler(fd, buf + sent, std::min(4096UL, total - sent))) > 0 ) )
        sent += cnt; 

    if( cnt < 0 || sent < total )
    {
        LOG(ERROR, LOG_MODE, errno);
        return false; 
    }
    
    return true;
}


int main()
{
    TcpServer* tcp = TcpServer::GetInstance(8080);

    tcp->Init();
    
    WritePipe(1, "Tcp Server Start!");
    
    char ip[20];
    int sock = tcp->GetNewLink(ip);
    
    SendBlock(sock, "Hello!~~~");

    tcp->DestoryInstance();

    return 0;
}