#pragma once 


#include <unistd.h>
#include <sys/wait.h>

#include "header.h"
#include "Protocol.hpp"


#define WEBROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_RESP_VERSION "HTTP/1.0"


class HttpTask 
{
public:
    HttpTask() = default;
    HttpTask(int sock, char* ip) : _sock(sock), _resp(ip) {}

    void operator()(int tid)
    {
        LOGMESSAGE(DEBUG, LOG_MODE, "Thread #%d: sock[%d]", tid, _sock);

        if( !RecvHttpRequest() ) 
        {   
            LOGMESSAGE(WARNING, LOG_MODE, "RecvHttpRequest failed!");
            _resp._status_code = Bad_Request;
            BuildHttpErrorPage();
        }

        if( !ParseHttpRequest() )
        {
            LOGMESSAGE(WARNING, LOG_MODE, "ParseHttpRequest failed!"); 
            BuildHttpErrorPage();
        }

        if( !SendHttpResponse() )
        {
            LOGMESSAGE(WARNING, LOG_MODE, "SendHttpResponse failed!"); 
            _resp._status_code = Internal_Server_Error;
            BuildHttpErrorPage();
        } 
        
        std::cout << " handle ok! " << std::endl;
END:
        DestoryHttpTask();       
    }

    

    ~HttpTask() = default;

private:

    /**********************************
     * 收取 Http 请求，解析请求，构建响应
     */
    bool RecvHttpRequest()
    {
        if( !_req.RecvRequestLine(_sock) )  return false;
        if( !_req.RecvRequestHeader(_sock)) return false;
        if( !_req.RecvRequestBody(_sock))   return false;

        return true;
    }    

    /**
     * 根据请求类型获取到 uri_path 和 请求参数（如果有的话）
     * 默认 Get 方法的请求参数在 uri 内，Post 的请求参数在 Body 内
     * 如果请求的程序为可执行程序或请求带有参数，那么调用 cgi 程序
     * 如果请求的
    */
    bool ParseHttpRequest()
    {
        std::string uri_path;
        std::string argument;

        if(_req._request_line._method == "GET") 
            GetHandler(&uri_path, &argument);
        else if(_req._request_line._method == "POST")
            PostHandler(&uri_path, &argument);
        
        LOGMESSAGE(DEBUG, LOG_MODE, "uri_path: [%s] argument: [%s]", 
                   uri_path.c_str(), argument.c_str());
        
        if( !AdjustUriPath(uri_path) )
        {
            _resp._status_code = Not_Found;
            return false;
        }
               
        LOGMESSAGE(DEBUG, LOG_MODE, "uri_path: [%s] argument: [%s]", 
                   uri_path.c_str(), argument.c_str()); 

        size_t file_size = 0;
        bool isCgi = false;
        if ( !StatUrlPath(uri_path, &isCgi) )
        {
            _resp._status_code = Not_Found;
            return false;
        }
                
        if( !argument.empty() && !isCgi ) isCgi = true;

        LOGMESSAGE(DEBUG, LOG_MODE, "After adjust: uri_path: [%s] file_size: [%d] isCgi: [%d]", 
            uri_path.c_str(), file_size, isCgi);

        if( isCgi ) return CgiHandler(uri_path, argument);
        return NonCgiHandler(uri_path);
    } 


    bool SendHttpResponse()
    {
        if ( !_resp.SendStatusLine(_sock) )     return false;
        if ( !_resp.SendResponseHeader(_sock) ) return false;
        if ( !_resp.SendEmptyLine(_sock) )      return false;
        if ( !_resp.SendBody(_sock) )           return false;

        return true;
    }  
    

    /******************************
     * 提取请求中的 uri 路径 和 参数 
     */
    void GetHandler(std::string* uri_path, std::string* argument)
    {
        const std::string& str = _req._request_line._url;
        
        size_t pos = 0;
        if( ( pos = str.find("?") ) != std::string::npos )
        {
            *uri_path = str.substr(0, pos);
            *argument = str.substr(pos + 1);
        }
        else
            *uri_path = str;

        //LOGMESSAGE(DEBUG, LOG_MODE, "GetHandler: uri_path[%s] argument[%s]", 
                   //uri_path->c_str(), argument->c_str()); 
    }

    void PostHandler(std::string* uri_path, std::string* argument)
    {
        *uri_path = _req._request_line._url;
        if( _req._request_body.size() )
            *argument = _req._request_body;
    }

    /**
     * 根据服务器目录结果调整 uri_path
     * 所有的 uri_path 加上前缀 wwwroot
     * 如果最后一个字符是 /，那么默认请求该目录下的 index.html
    */
    bool AdjustUriPath(std::string& uri_path) 
    {
        if( uri_path.empty() ) return false;
        
        uri_path = WEBROOT + uri_path;
        if( uri_path[uri_path.size() - 1] == '/' )
            uri_path += HOME_PAGE;

        return true;
    }

    /**
     * 通过调用 stat 函数确认 uri_path 是否存在
     * 如果 uri_path 为目录，需要调用 AdjustUriPath 加上后缀
     * 如果 uri_path 为可执行程序，需要将 isCgi 设置为 true
     * 如果 uri_path 为普通文件，打开并设置 file_size
    */
    bool StatUrlPath(std::string& uri_path, bool* isCgi)
    {
        struct stat st;
        int ret = stat(uri_path.c_str(), &st);
        if( ret == -1 )
        {
            LOGMESSAGE(ERROR, LOG_MODE, "StatUrlPath failed! uri_path:[%s] error message:[%s]", 
                        uri_path.c_str(), strerror(errno));
            return false;
        }
        
        // 如果 uri_path 是目录，那么加上 /HOME_PAGE 并重新 stat
        if( S_ISDIR(st.st_mode) )
        {
            uri_path += '/';
            AdjustUriPath(uri_path);
            if( stat(uri_path.c_str(), &st) == -1 )
            {
                LOG(ERROR, LOG_MODE, errno);
                return false;
            }
        }

        // 如果文件是一个可执行程序
        if( (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH) )
        {
            *isCgi = true;
            return true;
        }

        return true;
    }


    /*******************************************************************
     * 处理非 CGI 请求 和 CGI 请求 
     * NonCgi: 没有参数的 Get 和 Post 请求，直接返回资源页面
     * Cgi:    带参数的 Get 和 Post 请求，调用 Cgi 程序，创建子进程，处理请求
     */
    bool NonCgiHandler(const std::string& uri_path) 
    {
        _resp._uri_path = uri_path;

        return SetHttpResponse(OK);
    }

    /*
     * 如果是 Get  带参请求，通过环境变量传递参数
     * 如果是 Post 带参请求，通过建立管道，进行父子进程通信
     * 这样分开处理的原因是：Get 参数通过 url 传递，参数长度有限
     * 而 Post 通过 body 传参，参数长度可能很大。
     * 由于需要调用 execv* 函数，需要对子进程首先进行重定向
     */
    bool CgiHandler(const std::string& uri_path, const std::string& argument)
    {
        LOGMESSAGE(DEBUG, LOG_MODE, "CgiHandler: Start Processing!");
        
        _resp._body_is_page = false;

        const std::string& method = _req._request_line._method;
        
        // pipe_input[0] ：父进程读端 pipe_input[1] ：子进程写端
        // pipe_output[0]：子进程读端 pipe_output[1]：父进程写端
        int pipe_input[2], pipe_output[2]; 
        
        int ret1 = pipe(pipe_input);
        int ret2 = pipe(pipe_output);
        if( ret1 < 0 || ret2 < 0 ) 
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }

        if( !HttpUtil::SetPipeCapacity(pipe_input[0], 65535 * 5) || 
            !HttpUtil::SetPipeCapacity(pipe_output[0], 65535 * 5)   )
        {
            LOGMESSAGE(ERROR, LOG_MODE, "SetPipeCapacity failed! %s", strerror(errno));
            return false;
        }
        
        //std::string arg_env;

        int pid = fork();
        
        /**
         * 子进程工作：
         * 1. 关闭多余文件描述符
         * 2. 设置环境变量中的 METHOD 为 GET 或 POST
         * 3. 如果 method 为 GET  需要将 ARGUMENT 加入环境变量
         *    如果 method 为 POST 是父进程需要进行的工作
         * 4. 进行进程替换，执行 cgi 程序
        */
        if( pid == 0 )
        {
            close(pipe_input[0]);
            close(pipe_output[1]);
            int read_fd  = pipe_output[0];
            int write_fd = pipe_input[1];

            if( setenv("METHOD", method.c_str(), 1) == -1 )
            {
                LOGMESSAGE(ERROR, LOG_MODE, "METHOD add to env failed! %s", strerror(errno));
                _resp._status_code = Internal_Server_Error;
                return false;
            }
            
            LOGMESSAGE(DEBUG, LOG_MODE, "METHOD add to env success!");

            if( method == "GET" )
            {
                if( setenv("ARGUMENT", (char*)argument.c_str(), 1) == -1 )
                {
                    LOGMESSAGE(ERROR, LOG_MODE, "GET: METHOD add to env failed! %s", strerror(errno));
                    _resp._status_code = Internal_Server_Error;
                    return false; 
                }

                LOGMESSAGE(DEBUG, LOG_MODE, "GET: ARGUMENT add to env success %s!", getenv("ARGUMENT"));
            }

            // 将子进程管道重定向标准输入和输出
            dup2(read_fd , 0);
            dup2(write_fd, 1);

            execl(uri_path.c_str(), uri_path.c_str(), nullptr);
            
            log_quit(FATAL, LOG_MODE, errno, EXECV_ERROR);
        }
        
        /**
         * 父进程工作：
         * 1. 如果 method 为 POST，将 argument 写入管道中，并将 body 大小写入 环境变量 中
         * 2. 等待子进程结束，获取结束码，并设置 resp 报文类型
        */
        else if( pid > 0 )
        {
            close(pipe_input[1]);
            close(pipe_output[0]);
            int read_fd  = pipe_input[0];
            int write_fd = pipe_output[1];

            if( method == "POST" )
            {
                if( !HttpUtil::WritePipe(write_fd, argument) )
                    return false;
                LOGMESSAGE(DEBUG, LOG_MODE, "POST WritePipe argument success");
                close(write_fd);
            }

            int child_status = 0;
            pid_t ret = waitpid(pid, &child_status, 0);
            if (ret < 0) 
                log_quit(FATAL, LOG_MODE, errno, WAITPID_ERROR);


            // 检查子进程是否正常退出
            if( WIFEXITED(child_status))
            {
                // 检查子进程退出码是否为 0
                if( WEXITSTATUS(child_status) == 0 )
                {
                    LOGMESSAGE(DEBUG, LOG_MODE, "Child OK!");
                    std::string out;
                    // 由于不知道 cgi 程序处理结果的具体大小，所以逐字节读取
                    // 由于子进程退出，所以已经关闭了管道，最终会读取到 0
                    while( HttpUtil::ReadPipe(read_fd, &out, 1) ) 
                        _resp._response_body += out;                    
                    
                    LOGMESSAGE(DEBUG, LOG_MODE, "Cgi Response Body: %s", _resp._response_body.c_str());

                    _resp._status_code = OK;
                }   
                else
                {  
                    _resp._status_code = Bad_Request;
                    LOGMESSAGE(DEBUG, LOG_MODE, "Bad_Request");
                }
            }
            else
            {
                LOGMESSAGE(DEBUG, LOG_MODE, "Internal_Server_Error!");
                _resp._status_code = Internal_Server_Error;
            }

            close(read_fd);
        }
        // error
        else
            log_quit(FATAL, LOG_MODE, errno, FORK_ERROR); 

        SetHttpResponse(_resp._status_code);

        return _resp._status_code == OK;; 
    }


    bool SetHttpResponse(int status)
    {
        _resp._status_line._status_code = std::to_string(status);
        _resp._status_line._status_value = HttpUtil::GetHttpStatus(status)._desc; 
        _resp._status_line._version = HTTP_RESP_VERSION;

        std::string suffix;
        if( _resp._body_is_page )
        {
            _resp._response_header[CONTENT_LENGTH] = std::to_string( HttpUtil::GetFileSize(_resp._uri_path) );
            suffix = HttpUtil::GetFileSuffix(_resp._uri_path); 
        }
        else
        {
           _resp._response_header[CONTENT_LENGTH] = std::to_string(_resp._response_body.size()); 
           suffix = "html";
        }
        _resp._response_header[CONTENT_TYPE]  = HttpUtil::GetSuffixDesc(suffix);
        _resp._response_header[HOST] = _resp._client_ip;

        return true;
    }

    void BuildHttpErrorPage()
    {
        _resp._uri_path = HttpUtil::GetHttpStatus(_resp._status_code)._page_path;
        _resp._body_is_page = true;
        SetHttpResponse(_resp._status_code);
    }

    void DestoryHttpTask() 
    {
        close(_sock);
    }

private:
    int _sock;
    HttpRequest _req;
    HttpResponse _resp;
    
};