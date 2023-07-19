/*
 * @Author       : mark
 * @Date         : 2020-06-26
 * @copyleft Apache 2.0
 */ 
#include "buffer.h"

Buffer::Buffer(int initBuffSize) : 
    buffer_(initBuffSize), 
    readPos_(0), 
    writePos_(0) {}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 返回当前读指针所处的位置
const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

// 移动len长度的数据，表示读取了len长度的内容
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());
}

// 清空缓冲区
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
} 

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

// 扩充len长度的数据，并将数据存储到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);// 扩充缓冲区
    std::copy(str, str + len, BeginWrite());// 将str放到缓冲区
    HasWritten(len);// 移动写指针
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace_(len);// 依据len创建大于等于len长度的新空间，并添加到缓冲区中
    }
    assert(WritableBytes() >= len);
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];// 用两个IO向量来读取socket传来的信息
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    // iov[0]中存储的是能够读到缓冲区中的数据，直接与缓冲区绑定了
    iov[0].iov_base = BeginPtr_() + writePos_;// 在缓冲区的起始位置
    iov[0].iov_len = writable;// 可读取的最大长度
    // iov[1]中存储的是溢出缓冲区的数据
    iov[1].iov_base = buff; 
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {// 读发生错误
        *saveErrno = errno;// 将错误代码返回
    }
    // 读取的数据没有溢出缓冲区，直接将数据存储在buffer_中
    else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    }
    // 读取的数据溢出缓冲区,需要对缓冲区处理
    else {
        writePos_ = buffer_.size();// 写缓冲区满了
        Append(buff, len - writable);// 扩充缓冲区大小，然后再将数据保存下来
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

// 创建至少为len可写的新空间（扩展或者移动未读数据）
void Buffer::MakeSpace_(size_t len) {
    /*
    把已读数据的空间加上还未写的空间加起来，看是否能满足要求
    如果不满足要求,则在当前的writepos上加上长度，新缓冲区的大小变为（originallen + len + 1）
    */
    if(WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } 
    // 加上已经读的部分，满足缓冲区的要求
    else {
        size_t readable = ReadableBytes();// 缓冲区中还未被读取的数据长度
        // 把未读取的数据移动到缓冲区首部[0,readable]
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;// 重置读指针
        writePos_ = readPos_ + readable;// 重置写指针
        assert(readable == ReadableBytes());
    }
}