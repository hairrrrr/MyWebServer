#pragma once 

#include <algorithm>
#include <vector>
#include <sys/sendfile.h>
#include <functional>

#include "header.h"
#include "StatusCode.hpp"

#define ARGUMENT_SEP "&"
#define KEY_VALUE_SEP "="

class HttpUtil
{
    using read_block_handler = std::function<ssize_t(int, void*, size_t)>;
    using write_block_handler = std::function<ssize_t(int, const void*, size_t)>;
public:
    /*
     * GetLine: 从 input 缓冲区内读取一行数据
     *
     * 不同的浏览器发来的 Http 请求行结尾不同。可能是以下三种情况之一：
     * 1. \r 
     * 2. \n 
     * 3. \r\n
     * 我们存储时，同意将这些字符删去。发送响应时，采用 \r\n 作为行结尾
     */
    static bool GetLine(int sock, std::string* out) 
    {
        out->clear();

        char ch;
        while( true )
        {
            ssize_t cnt = recv(sock, &ch, 1, 0);

            if( cnt == 0)
            {
                LOGMESSAGE(WARNING, LOG_MODE, "client closed on fd [%d]!", sock);
                return false;
            }
            else if( cnt < 0 )
            {
                LOG(ERROR, LOG_MODE, errno);
                return false;
            }

            // 可能是 \r 和 \r\n 中的情况之一。
            // 所以我们需要先窥探一下后面的字节，确定是哪一种情况。
            if( ch == '\r' ) 
            {
                int peek = recv(sock, &ch, 1, MSG_PEEK);
                // input 缓冲为空
                if( peek == 0 ) 
                    break;
                if( peek < 0 )
                {
                    LOG(ERROR, LOG_MODE, errno);
                    return false;
                }
                
                if( ch == '\n' ) 
                   recv(sock, &ch, 1, 0);

                ch = '\n'; 
            }
            
            if( ch == '\n' )
                break;

            *out += ch;
        }

        //std::cout << "GetLine: " << *out << "\n";
        
        return true;
    }

    /*
     * GetBlock: 从 input 缓冲区内读取 total 大小的一块数据
     */
    static bool GetBlock(int sock, std::string* out, size_t total)
    {
        using namespace std::placeholders;
        return ReadBlockFromBuffer(sock, out, total, std::bind(recv, _1, _2, _3, 0));
    }
    

    /**
     * CutString 以 sep 为分隔符对 str 进行切分，并将最终结果输出到 out 中 
     */
    static bool CutString(const std::string& str,  const std::string& sep, 
                   std::vector<std::string>* out )
    {
        size_t sep_pos = 0, start = 0;
        
        while( ( sep_pos = str.find(sep, sep_pos) ) != std::string::npos )
        {
            out->push_back(str.substr(start, sep_pos - start));
            sep_pos += sep.size();
            start = sep_pos;
        }       

        out->push_back(str.substr(start));

        return out->size();
    } 
    

    /**
     * 根据后缀获取响应的 Content-Type 的描述
    */
    static std::string GetSuffixDesc(const std::string& suffix)
    {
        static std::unordered_map<std::string, std::string> desc_table = {
            {"html", "text/html"}, {"htm", "text/html" },
            {"css", "text/css"}  , {"js", "application/javascript"},
            {"jpg", "application/x-jpg"}, 
            {"xml", "application/xml"},
        };

        auto it = desc_table.find(suffix);
        if( it == desc_table.end() )
            return "text/html";
        return it->second;
    }


    /**
     * 根据 uri_path 获取文件大小 
    */
    static size_t GetFileSize(const std::string& uri_path)
    {
        struct stat st;
        stat(uri_path.c_str(), &st);
        return st.st_size;
    }

    
    /**
     * 获取 uri_path 的后缀
    */
    static std::string GetFileSuffix(const std::string& uri_path)
    {
        size_t pos = uri_path.rfind('.');
        if( pos == std::string::npos ) return "html";
        return uri_path.substr(pos + 1);
    }


    /**
     * 根据状态码获取
    */
    static const HttpStatus& GetHttpStatus(int status_code)
    {
        static std::unordered_map<int, HttpStatus> http_status;
        if( http_status.empty() )
        {
            http_status[OK] = {OK, "OK", "wwwroot/index.html"};
            http_status[Bad_Request] = {Bad_Request, "Bad Request", "wwwroot/bad_request.html"};
            http_status[Not_Found] = {Not_Found, "Not Found", "wwwroot/not_found.html"};
            http_status[Internal_Server_Error] = {Internal_Server_Error, "Internal Server Error", "wwwroot/server_error.html"};
        }

        return http_status[status_code];
    }

    /**
     * 向 sock 的输出缓冲区中发送一小块数据
    */
    static bool SendLine(int sock, const std::string& line)
    {
        if( line.empty() )
        {
            LOGMESSAGE(WARNING, LOG_MODE, "SendLine: buffer is empty!");
            return false;
        }
        ssize_t sent = send(sock, line.c_str(), line.size(), 0);
        if( sent <= 0 || sent != line.size() )
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }

        LOGMESSAGE(DEBUG, LOG_MODE, "SendLine: %s", line.c_str());

        return true;
    }


    /**
     * 向 sock 的输出缓冲区中发送大块数据
    */
    static bool SendBlock(int sock, const std::string& block)
    {
        using namespace std::placeholders;
        return WriteBlockToBuffer(sock, block, std::bind(send, _1, _2, _3, 0));
    }


    /**
     * sendfile 零拷贝技术
     * 将文件不经过用户空间，直接从内核页缓冲发到 sock 的输出缓冲区内
    */
    static bool SendPage(int sock, const std::string& uri_path)
    {
        int uri_fd = open(uri_path.c_str(), O_RDONLY);
        if( uri_fd == -1 )
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }

        size_t file_size = HttpUtil::GetFileSize(uri_path);

        ssize_t sent = sendfile(sock, uri_fd, nullptr, file_size);

        LOGMESSAGE(DEBUG, LOG_MODE, "sock: [%d] fd: [%d] file_size: [%d] actually sent: [%d]", 
                   sock, uri_fd, file_size, sent);
        
        if( sent == -1 || sent != file_size )
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }

        close(uri_fd);

        return true;
    }

    
    static bool ReadPipe(int fd, std::string* out, size_t total)
    {
        return ReadBlockFromBuffer(fd, out, total, read);
    }

    static bool WritePipe(int fd, const std::string& block)
    {
        return WriteBlockToBuffer(fd, block, write); 
    }


    static bool SetPipeCapacity(int pipe_fd, size_t size)
    {
        return fcntl(pipe_fd, F_SETPIPE_SZ, size) != -1;
    }

    static bool SetServerDaemon()
    {
        // 不让当前进程变成组长进程
        if( fork() > 0 ) exit(0);

        // 将当前进程变成独立的会话
        setsid();

        int fd = open("/dev/null", O_RDWR);
        if( fd > 0 )
        {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
        }
        close(fd);

        return true;
    }

private:
    
    static bool ReadBlockFromBuffer(int fd, std::string* out, size_t total, read_block_handler handler)
    {
        out->clear();

        char buffer[1024];
        size_t read = 0, left = total;
        size_t buffer_size = sizeof(buffer) - 1;
        int recv_num = 0;
        // 注意接受的字节数应该是缓冲区 buffer 大小与剩余待读取字节数的较小值
        while( (read < total) && 
               (( recv_num = handler(fd, buffer, std::min(buffer_size, left)) ) > 0) )
        {
            buffer[recv_num] = '\0';
            *out += buffer;
            read += recv_num;
            left -= recv_num;
        }

        //LOGMESSAGE(DEBUG, LOG_MODE, "Get Block: total = [%d] read = [%d] left = [%d]", total, read, left);

        if( recv_num < 0 )
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }
        if( recv_num == 0 )
        {
            //LOGMESSAGE(WARNING, LOG_MODE, "fd[%d] close on ", fd);
            return false;
        }   
        if( read < total )
        {
            LOG(ERROR, LOG_MODE, errno);
            return false;
        }

        return true;
    }

    static bool WriteBlockToBuffer(int fd, const std::string& block,  write_block_handler handler)
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
    
};