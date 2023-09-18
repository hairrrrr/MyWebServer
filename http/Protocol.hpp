#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>

#include <sys/sendfile.h>


#include "HttpUtil.hpp"
#include "StatusCode.hpp"

#define LINE_SEP " "
#define HEADER_SEP ": "
#define METHOD_SEP "&"
#define LINE_END "\r\n"


#define CONTENT_LENGTH  "Content-Length"
#define CONTENT_TYPE    "Content-Type"
#define ACCEPT_ENCODING "Accept-Encoding"
#define HOST            "Host"


struct RequestLine
{
    std::string _method;
    std::string _url;
    std::string _version;
};


struct StatusLine
{
    std::string _version;
    std::string _status_code;
    std::string _status_value;    
};


class HttpRequest 
{
public:
    RequestLine _request_line;
    std::unordered_map<std::string, std::string> _request_header;
    std::string _request_body;



    bool RecvRequestLine(int sock) 
    {
        
        std::string request_line;
        bool ret = HttpUtil::GetLine(sock, &request_line);
        if(!ret) return false;

        if( !ParseRequestLine(request_line) ) return false;

        std::cout << _request_line._method << " " << _request_line._url 
                  << " " << _request_line._version << "\n";

        return true; 
    }

    bool RecvRequestHeader(int sock)
    {
        std::string header_line;
        while(true)
        {
            header_line.clear();
            if( !HttpUtil::GetLine(sock, &header_line) )
                return false;
            
            if( header_line.empty() )
                break;

            if( !ParseRequestHeader(header_line) ) 
                return false;
        }
        
        for(auto& pair : _request_header)
            std::cout << pair.first << " " << pair.second << std::endl;

        return true;
    }

    bool RecvRequestBody(int sock)
    {
        if( !HasRequestBody() ) return true;
        
        size_t body_size = GetBodySize();
        LOGMESSAGE(DEBUG, LOG_MODE, "Request Body Size: %d", body_size);

        if( !HttpUtil::GetBlock(sock, &_request_body, body_size) )
            return false;

        std::cout << "\n" << _request_body << "\n";

        return true;
    }

private:
    bool ParseRequestLine(const std::string& request_line) 
    {
        std::vector<std::string> vec;
        if( !HttpUtil::CutString(request_line, LINE_SEP, &vec) )
            return false;
        
        // 将请求转换为大写
        _request_line._method.resize(vec[0].size());
        std::transform(vec[0].begin(), vec[0].end(), _request_line._method.begin(), toupper); 
        _request_line._url     = vec[1];
        _request_line._version = vec[2];
        
        return true;
    }

    bool ParseRequestHeader(const std::string& header_line) 
    {
        std::vector<std::string> vec;
        if( !HttpUtil::CutString(header_line, HEADER_SEP, &vec) )
            return false;

        if( vec.size() != 2 )
        {
            LOGMESSAGE(ERROR, LOG_MODE, "header format wrong!");
            for(auto& e : vec)
                std::cout << e << std::endl;
            return false;
        }

        _request_header[vec[0]] = vec[1];
        
        return true;
    }

    bool HasRequestBody() const 
    {
        auto it = _request_header.find(CONTENT_LENGTH);
        if( it == _request_header.end() ) return false;
        const std::string& length = it->second;
        return std::stoi(length) > 0; 
    } 

    size_t GetBodySize()  
    {
        const std::string& length = _request_header[std::string(CONTENT_LENGTH)];
        return std::stoi(length);
    }
};


class HttpResponse 
{
public:
    StatusLine _status_line;
    std::unordered_map<std::string, std::string> _response_header;
    std::string _response_body;
    int _status_code;
    std::string _uri_path;
    std::string _client_ip;
    bool _body_is_page;
    
    HttpResponse() = default;

    HttpResponse(const char* ip) 
    : _status_code(OK), _client_ip(ip), _body_is_page(true)
    {}

    bool SendStatusLine(int sock) 
    {
        std::string status_line;
        status_line += _status_line._version;
        status_line += LINE_SEP;
        status_line += _status_line._status_code;
        status_line += LINE_SEP;
        status_line += _status_line._status_value;
        status_line += LINE_END;

        if( !HttpUtil::SendLine(sock, status_line) )
            return false;

        return true;
    }

    bool SendResponseHeader(int sock)
    {
        for(auto& [key, value] : _response_header)
        {
            std::string line = key + HEADER_SEP + value + LINE_END;
            if ( !HttpUtil::SendLine(sock, line) )
                return false;
        }

        return true;
    }

    bool SendEmptyLine(int sock) 
    {
        if( !HttpUtil::SendLine(sock, "\r\n") )
            return false;

        return true;
    }

    bool SendBody(int sock)
    {
        if( _body_is_page ) 
        {
            if( !HttpUtil::SendPage(sock, _uri_path) )
                return false;
        }
        else 
        {
            LOGMESSAGE(DEBUG, LOG_MODE, "Respinse Body: %s", _response_body.c_str());
            if( !HttpUtil::SendBlock(sock, _response_body) )
                return false;
        }
        
        return true;
    }
};