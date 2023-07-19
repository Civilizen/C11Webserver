@startuml Epoller

class Epoller {
  - epollFd_ : int
  - events_ : vector<epoll_event>
  +Epoller(int maxEvent = 1024)
  +~Epoller()
  +AddFd(int fd, uint32_t events) : bool
  +ModFd(int fd, uint32_t events): bool
  +DelFd(int fd): bool
  +Wait(int timeoutMs = -1): int
  +GetEventFd(size_t i) : int
  +GetEvents(size_t i): uint32_t
}

class SqlConnPool{
  - MAX_CONN_ : int
  -  useCount : int
  - freeCount : int
  - connQue_ : queue<MYSQL*>
  - mtx_ : mutex
  - SqlConnPool()
  - ~SqlConnPool()
  + {static} Instance() : SqlConnPool*
  + GetConn() : MYSQL*
  + FreeConn(MYSQL * conn): void
  + Init() : void
  + ClosePool() : void
}

class HttpConn{
  - fd_:int
  - addr_:sockaddr_in
  - isClose_:bool
  - iovCnt_:int
  - iov_: iovec[2]
  - readBuff_:Buffer
  - writeBuff_:Buffer
  - request_:HttpRequest
  - response_:HttpResponse
  + {static} isET:bool
  + {static} srcDir:const char*
  + {static} userCount: atomic<int>
  + HttpConn()
  + ~HttpConn()
  + init(int sockFd, const sockaddr_in& addr) : void
  + read(int* saveErrno):ssize_t
  + write(int* saveErrno):ssize_t
  + Close():void
  + GetFd():int
  + GetPort():int
  + GetIP():const char*
  + GetAddr():sockaddr_in
  + process():bool
  + ToWriteBytes():int
  + IsKeepAlive():bool
}
@enduml
