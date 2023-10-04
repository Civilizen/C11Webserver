#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {// 处理请求报文类
public:
    enum PARSE_STATE {
        REQUEST_LINE, // 请求行
        HEADERS, // 头部
        BODY, // 数据
        FINISH,  // 解析完成  
    };

    enum HTTP_CODE { // HTTP 状态码
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;
    // 初始化请求报文
    void Init();
    // 通过缓冲区的完整数据解析出请求报文
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;
    // 返回请求报文是否为keep-alive
    bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    // 解析请求行
    bool ParseRequestLine_(const std::string& line);
    // 解析头部
    bool ParseHeader_(const std::string& line);
    // 解析数据
    void ParseBody_(const std::string& line);
    // 拆分path
    void ParsePath_();
    void ParsePost_();
    // 从post的url中解析出字典，然后存放到post_字典中
    void ParseFromUrlencoded_();
    // 验证用户信息
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    PARSE_STATE state_; // 自动机当前状态
    std::string method_, path_, version_, body_;
    // 字典格式的请求报文头
    std::unordered_map<std::string, std::string> header_;
    // 表单字典
    std::unordered_map<std::string, std::string> post_;
    // 目录路由
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};


#endif //HTTP_REQUEST_H