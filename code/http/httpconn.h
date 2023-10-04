#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t read(int* saveErrno);

    ssize_t write(int* saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;
    
    sockaddr_in GetAddr() const;
    
    bool process();

    // 返回要发送数据的报文长度（包括头部和数据）
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }
    // 是否保持长连接
    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET; //是否是边缘触发
    static const char* srcDir; //用户目录
    static std::atomic<int> userCount; // 原子操作的连接用户数
    
private:
   
    int fd_;// 当前socket连接在系统中的文件描述符
    struct  sockaddr_in addr_; // 当前连接对应的客户套接字

    bool isClose_;// 当前连接是否存活
    
    int iovCnt_;
    struct iovec iov_[2];// I/O区 ivo[0]是报文首部 ivo[1]是报文数据
    
    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};


#endif //HTTP_CONN_H