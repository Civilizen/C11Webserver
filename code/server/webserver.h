#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer { // 单例模式
public:
    WebServer(
        int port, // 服务端口
        int trigMode, // 触发模式
        int timeoutMS, // timeouts
        bool OptLinger, // 优雅关闭
        int sqlPort, // mysql 端口
        const char* sqlUser, // mysql用户名称
        const  char* sqlPwd, // mysql密码
        const char* dbName, // 数据库名称
        int connPoolNum, // 连接池数量
        int threadNum, // 线程池数
        bool openLog, // 日志开关
        int logLevel, // 日志等级
        int logQueSize //日志异步队列容量
        );

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536; 

    static int SetFdNonblock(int fd);

    int port_; // 服务端口
    bool openLinger_; /* 优雅关闭: 直到所剩数据发送完毕或超时 */
    int timeoutMS_;  /* 毫秒MS */
    bool isClose_;  // 服务是否关闭
    int listenFd_; // 监听的socket
    char* srcDir_; // webserver根目录
    
    uint32_t listenEvent_; // 监听模式
    uint32_t connEvent_; // 连接模式
   
    std::unique_ptr<HeapTimer> timer_; // 计时器
    std::unique_ptr<ThreadPool> threadpool_; // 线程池
    std::unique_ptr<Epoller> epoller_; 
    std::unordered_map<int, HttpConn> users_; //socket:http连接
};


#endif //WEBSERVER_H