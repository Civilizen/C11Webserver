/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    // 清除内存映射
    void UnmapFile();
    // 返回共享内存的起始位置
    char* File();
    // 返回报文数据段的长度
    size_t FileLen() const;
    // 响应报文需要读取的文件发生错误读取，指定错误的页面
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const { return code_; }

private:    
    // 添加响应报文状态行
    void AddStateLine_(Buffer &buff);
    // 添加响应报文头部
    void AddHeader_(Buffer &buff);
    // 添加响应报文体
    void AddContent_(Buffer &buff);
    // 指定错误页面
    void ErrorHtml_();
    // 返回响应报文数据段的文件类型
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;
    
    char* mmFile_; // 文件映射到内存后，文件中的内容
    struct stat mmFileStat_;
    // 文件后缀类型，用于指定响应报文的content-type
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    // 响应状态码
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // 错误代码对应的响应页面
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H