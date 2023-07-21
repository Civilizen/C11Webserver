/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 
#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};
// 根据socket连接文件描述符和客户地址初始化http连接
void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.RetrieveAll();// 清空读缓冲区
    readBuff_.RetrieveAll();// 清空写缓冲区
    isClose_ = false; // 激活连接
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}
// 关闭连接
void HttpConn::Close() {
    response_.UnmapFile();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}
// 获取连接的文件描述符
int HttpConn::GetFd() const {
    return fd_;
};
// 获取套接字
struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}
// 获取IP
const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}
// 获取端口号
int HttpConn::GetPort() const {
    return addr_.sin_port;
}

// 读取socket传输过来的数据
ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);// 将数据读入读缓冲区
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

// 往socket中写入数据，即传输数据给客户
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_); // 非阻塞SOCKET的IO，一次不一定能全部写入
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) { // 第一个块已经被全部传输完毕
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            // 清空第一个iov块中的信息
            if(iov_[0].iov_len) { 
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else { // 第一个块传输的东西没传输完
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isET || ToWriteBytes() > 10240);
    return len;
}
// http 处理请求
bool HttpConn::process() {
    request_.Init(); // 初始化request报文类
    if(readBuff_.ReadableBytes() <= 0) { //处理http的发送事件
        return false;
    }
    // 成功解析http报文，创建响应报文
    else if(request_.parse(readBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        // 创建响应报文
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } 
    // 解析报文失败，创建响应报文，返回错误代码
    else {
        response_.Init(srcDir, request_.path(), false, 400);
    }
    // 创建响应报文，并把相应数据存放到写缓冲区
    response_.MakeResponse(writeBuff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}
