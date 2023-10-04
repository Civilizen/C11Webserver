#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool { // 单例模式
public:
    static SqlConnPool *Instance(); // 返回一个已经创建的连接池，或者创建一个新的，静态方法，通过类调用

    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool(); // 单例模式需要将构造函数私有化
    ~SqlConnPool();

    int MAX_CONN_; // sql连接池额最大支持连接数
    int useCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_; // 连接池队列
    std::mutex mtx_;
    sem_t semId_;
};


#endif // SQLCONNPOOL_H