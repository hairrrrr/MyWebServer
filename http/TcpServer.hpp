#pragma once 

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "header.h"
#include "error_number.h"
#include "../log.hpp"

#define DEFAULT_PORT 8080
#define LISTEN_BACKLOG 20


class TcpServer
{
public:

    static TcpServer* GetInstance(int port) 
    {
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        if( nullptr == _tcp )
        {
            pthread_mutex_lock(&lock);
            
            if( nullptr == _tcp )
                _tcp = new TcpServer(port);

            pthread_mutex_unlock(&lock);
        }

        LOGMESSAGE(INFO, LOG_MODE, "Singleton Tcp Server Create Success! %p", _tcp);
        return _tcp;
    }

    void Init() 
    {
        Socket();
        Bind();
        Listen(LISTEN_BACKLOG);

        signal(SIGPIPE, SIG_IGN);

        LOGMESSAGE(INFO, DEBUG_MODE, "Tcp Server Init Success!");
    }

    int GetNewLink(char ip[20]) const 
    {
        int sock = Accept(ip);
        if( sock < 0 )
        {
            LOG(ERROR, LOG_MODE, errno);
            return -1 ;
        }
        
        return sock;

    }

    void DestoryInstance()
    {
        delete _tcp;
        _tcp = nullptr;
        LOGMESSAGE(INFO, DEBUG_MODE, "Singleton Tcp Server Destroy Success!"); 
    }

    ~TcpServer() 
    { 
        if( _listen_sock >= 0 )
            close(_listen_sock);
    }

private:
    TcpServer(int port = DEFAULT_PORT) 
    : _port(port), _listen_sock(-1)
    {}

private:
    bool Socket() 
    {
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if( _listen_sock < 0 )
            log_quit(FATAL, LOG_MODE, errno, SOCKET_ERROR);
        
        int opt = 1;
        setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

        return true; 
    }

    bool Bind() const
    {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(_port);
        // inet_pton(AF_INET, "0.0.0.0", &sockaddr.sin_addr.s_addr);
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        
        int ret = bind(_listen_sock, (struct sockaddr*)&sockaddr, sizeof sockaddr);
        if( ret < 0 )
            log_quit(FATAL, LOG_MODE, errno, BIND_ERROR);
        
        return true;
    }

    bool Listen(int backlog) const 
    {
        int ret = listen(_listen_sock, backlog);
        if( ret < 0 )
            log_quit(FATAL, LOG_MODE, errno, LISTEN_ERROR);

        return true;
    }
    
    int Accept(char ip[20]) const
    {
        struct sockaddr_in client;
        socklen_t socklen = sizeof(client);
        int sock = accept(_listen_sock, (struct sockaddr*)&client, &socklen);
        
        if( sock < 0 ) return -1;

        uint16_t port = ntohs(client.sin_port);
        inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, 20);

        LOGMESSAGE(INFO, DEBUG_MODE, "Get New Link! IP:[%s] Port:[%d] SOCK:[%d]", ip, port, sock);

        return sock;
    }

private:
    int _port;
    int _listen_sock;
    static TcpServer* _tcp;
};

TcpServer* TcpServer::_tcp = nullptr;