#ifndef LOGGER_HPP
#define LOGGER_HPP
#include "timeex.hpp"

#include <string>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <cstdio> // std::snprintf()
#include <stdexcept>
#include <assert.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#endif

namespace cpp_streamer
{

#define LOGGER_BUFFER_SIZE (2*1024*1024)

enum LOGGER_LEVEL {
    LOGGER_DEBUG_LEVEL,
    LOGGER_INFO_LEVEL,
    LOGGER_WARN_LEVEL,
    LOGGER_ERROR_LEVEL
};

#define LogError(logger, data) LogErrorfEx(logger, __FUNCTION__, __FILE__, __LINE__, data)
#define LogWarn(logger, data)  LogWarnfEx(logger, __FUNCTION__, __FILE__, __LINE__, data)
#define LogInfo(logger, data)  LogInfofEx(logger, __FUNCTION__, __FILE__, __LINE__, data)
#define LogDebug(logger, data) LogDebugfEx(logger, __FUNCTION__, __FILE__, __LINE__, data)
#define LogErrorf(logger, fmt, ...) LogErrorfEx(logger, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LogWarnf(logger, fmt, ...)  LogWarnfEx(logger, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LogInfof(logger, fmt, ...)  LogInfofEx(logger, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LogDebugf(logger, fmt, ...) LogDebugfEx(logger, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LogInfoData(logger, data, len, dscr) LogInfoDataEx(logger, __FUNCTION__, __FILE__, __LINE__, data, len, dscr)

class Logger
{
public:
    Logger(const std::string filename = "", enum LOGGER_LEVEL level = LOGGER_INFO_LEVEL):filename_(filename)
    , level_(level)
    {
        buffer_ = new char[buffer_len_];
    }
    ~Logger()
    {
        delete[] buffer_;
        buffer_ = nullptr;
    }

public:
    void SetFilename(const std::string& filename) {
        filename_ = filename;
    }
    void SetLevel(enum LOGGER_LEVEL level) {
        level_ = level;
    }

    enum LOGGER_LEVEL GetLevel() {
        return level_;
    }
    void AllocBuffer(size_t len) {
        if (buffer_) {
            delete[] buffer_;
        }
        buffer_ = new char[len];
        buffer_len_ = len;
    }
    char* GetBuffer() {
        return buffer_;
    }
    size_t BufferSize() {
        return buffer_len_;
    }

    void Logf(const char* level, const char* func, const char* file, int line, const char* buffer) {
        std::stringstream ss;
        unsigned long long tid = 0;
#ifdef _WIN32
        tid = static_cast<unsigned long long>(::GetCurrentThreadId());
#elif defined(__linux__)
        tid = static_cast<unsigned long long>(::syscall(SYS_gettid));
#else
        tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
#endif
        const char* filename = file;
        const char* slash1 = strrchr(file, '/');
        const char* slash2 = strrchr(file, '\\');
        if (slash1 && slash2)
            filename = (slash1 > slash2) ? slash1 + 1 : slash2 + 1;
        else if (slash1)
            filename = slash1 + 1;
        else if (slash2)
            filename = slash2 + 1;
        ss << "[" << level << "]" << "[" << get_now_str() << "]"
           << "[TID:" << tid << "]"
           << "[" << func << "]"
           << buffer
           << "[" << filename << ":" << line << "] " << "\r\n";
        if (filename_.empty()) {
            std::cout << ss.str();
        } else {
            FILE* fp;
#ifdef _WIN64
            errno_t err = fopen_s(&fp, filename_.c_str(), "ab+");
            if (err == 0 && fp != nullptr) {
                fwrite(ss.str().c_str(), ss.str().length(), 1, fp);
                fclose(fp);
            }
#else
            fp = fopen(filename_.c_str(), "ab+");
            if (fp != nullptr) {
                fwrite(ss.str().c_str(), ss.str().length(), 1, fp);
                fclose(fp);
            }
#endif
        }
    }

private:
    std::string filename_;
    enum LOGGER_LEVEL level_;
    char* buffer_ = nullptr;
    size_t buffer_len_ = LOGGER_BUFFER_SIZE;
};

inline void LogErrorfEx(Logger* logger, const char* func, const char* file, int line, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_ERROR_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
    va_start(ap, fmt);
    int ret_len = vsnprintf(buffer, bsize, fmt, ap);
    buffer[ret_len] = 0;
    va_end(ap);
    logger->Logf("E", func, file, line, buffer);
}

inline void LogWarnfEx(Logger* logger, const char* func, const char* file, int line, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_WARN_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, bsize, fmt, ap);
    va_end(ap);
    logger->Logf("W", func, file, line, buffer);
}

inline void LogInfofEx(Logger* logger, const char* func, const char* file, int line, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, bsize, fmt, ap);
    va_end(ap);
    logger->Logf("I", func, file, line, buffer);
}

inline void LogDebugfEx(Logger* logger, const char* func, const char* file, int line, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_DEBUG_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
    va_start(ap, fmt);
    int ret_len = vsnprintf(buffer, bsize, fmt, ap);
    buffer[ret_len] = 0;
    va_end(ap);
    logger->Logf("D", func, file, line, buffer);
}

inline void LogInfoDataEx(Logger* logger, const char* func, const char* file, int line, const uint8_t* data, size_t len, const char* dscr) {
    if (!logger || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    const size_t print_buffer_size = 500 * 1024;
    char* print_data = new char[print_buffer_size];

    if (print_data == nullptr) {
        return;
    }
    size_t print_len = 0;
    const int MAX_LINES = 500;
    int nline = 0;
    int index = 0;
    print_len += snprintf(print_data, print_buffer_size, "%s:", dscr);
    for (index = 0; index < (int)len; index++) {
        if ((index%16) == 0) {
            print_len += snprintf(print_data + print_len, print_buffer_size - print_len, "\r\n");
            if (++nline > MAX_LINES) {
                break;
            }
        }
        print_len += snprintf(print_data + print_len, print_buffer_size - print_len,
            " %02x", *(static_cast<const uint8_t*>(data + index)));
    }

    logger->Logf("I", func, file, line, print_data);

    delete[] print_data;
}

class CppStreamException : public std::exception
{
public:
    explicit CppStreamException(const char* description)
    {
        desc_ = description;
    }

    virtual const char* what() const noexcept { return desc_.c_str(); } 

private:
    std::string desc_;
};

#define CSM_THROW_ERROR(desc, ...) \
    do \
    { \
        char exp_buffer[1024]; \
        std::snprintf(exp_buffer, sizeof(exp_buffer), desc, ##__VA_ARGS__); \
        throw CppStreamException(exp_buffer); \
    } while (false)

}
#endif //LOGGER_HPP