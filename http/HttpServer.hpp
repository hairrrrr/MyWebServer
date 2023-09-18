#pragma once 

#include "TcpServer.hpp"
#include "ThreadPool.hpp"
#include "HttpTask.hpp"

class HttpServer
{
public:
    HttpServer(uint16_t port) : _port(port) {} 

    void Run() const  
    {
        HttpUtil::SetServerDaemon();

        TcpServer* tcp = TcpServer::GetInstance(_port);
        std::shared_ptr<ThreadPool<HttpTask>> 
        thread_pool = ThreadPool<HttpTask>::GetInstance();
        
        tcp->Init();

        while(true)
        {
            char ip[20];

            int sock = tcp->GetNewLink(ip);
            if( sock < 0 ) continue;
            
            HttpTask task(sock, ip);
            thread_pool->PushTask(task);
        }              
   
        tcp->DestoryInstance();
    }



private:
    uint16_t _port;
};

