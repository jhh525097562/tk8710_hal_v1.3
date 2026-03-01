/**
 * @file trm_log.c
 * @brief TRM模块完全独立日志系统实现
 * 
 * 完全独立于TK8710日志系统，无任何耦合
 */

#include "../inc/trm/trm_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* TRM独立日志配置 */
TRMLogConfig g_trmLogConfig = {
    .level = TRM_LOG_INFO,          /* 默认信息级别 */
    .enable_timestamp = 1,          /* 默认启用时间戳 */
    .enable_module_name = 1,        /* 默认启用模块名 */
    .enable_file_info = 1           /* 默认启用文件信息（包含函数名和行数） */
};

/* TRM日志输出回调函数 */
static TRMLogCallback g_trmLogCallback = NULL;

/* TRM日志级别名称映射 */
static const char* g_trmLevelNames[] = {
    [TRM_LOG_NONE]  = "NONE",
    [TRM_LOG_ERROR] = "ERROR",
    [TRM_LOG_WARN]  = "WARN",
    [TRM_LOG_INFO]  = "INFO",
    [TRM_LOG_DEBUG] = "DEBUG",
    [TRM_LOG_TRACE] = "TRACE"
};

/**
 * @brief 获取TRM日志级别名称
 * @param level 日志级别
 * @return 级别名称字符串
 */
const char* TRM_LogGetLevelName(TRMLogLevel level)
{
    if (level >= sizeof(g_trmLevelNames)/sizeof(g_trmLevelNames[0])) {
        return "UNKNOWN";
    }
    return g_trmLevelNames[level];
}

/**
 * @brief 获取当前时间戳字符串
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 实际写入的字符数
 */
static int TRM_GetTimestamp(char* buffer, int size)
{
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    return snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
                   tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

/**
 * @brief 获取文件名（去除路径）
 * @param filepath 完整文件路径
 * @return 文件名指针
 */
static const char* TRM_GetFilename(const char* filepath)
{
    const char* filename = strrchr(filepath, '/');
    if (filename) {
        return filename + 1; /* 跳过'/' */
    }
    filename = strrchr(filepath, '\\');
    if (filename) {
        return filename + 1; /* 跳过'\' */
    }
    return filepath;
}

/**
 * @brief TRM日志输出函数
 * @param level 日志级别
 * @param tag 模块标签
 * @param file 文件名
 * @param line 行号
 * @param func 函数名
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
void TRM_LogOutput(TRMLogLevel level, const char* tag, 
                   const char* file, int line, const char* func,
                   const char* fmt, ...)
{
    va_list args;
    char buffer[1024];
    int offset = 0;
    
    /* 检查日志级别 */
    if (level > g_trmLogConfig.level) {
        return;
    }
    
    /* 时间戳 */
    if (g_trmLogConfig.enable_timestamp) {
        char timestamp[32];
        TRM_GetTimestamp(timestamp, sizeof(timestamp));
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s] ", timestamp);
    }
    
    /* 日志级别 */
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s] ", TRM_LogGetLevelName(level));
    
    /* 模块名 */
    if (g_trmLogConfig.enable_module_name && tag) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s] ", tag);
    }
    
    /* 文件信息 */
    if (g_trmLogConfig.enable_file_info && file && func) {
        const char* filename = TRM_GetFilename(file);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s:%d:%s] ", filename, line, func);
    }
    
    /* 用户消息 */
    va_start(args, fmt);
    offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    va_end(args);
    
    /* 添加换行符 */
    if (offset < sizeof(buffer) - 1) {
        buffer[offset] = '\n';
        buffer[offset + 1] = '\0';
    }
    
    /* 输出日志 */
    if (g_trmLogCallback) {
        /* 使用用户回调 */
        va_start(args, fmt);
        g_trmLogCallback(level, tag, file, line, func, fmt, args);
        va_end(args);
    } else {
        /* 默认输出到标准输出 */
        printf("%s", buffer);
        fflush(stdout);
    }
}

/**
 * @brief 初始化TRM日志系统
 * @param level 日志级别
 */
void TRM_LogInit(TRMLogLevel level)
{
    g_trmLogConfig.level = level;
    g_trmLogConfig.enable_timestamp = 1;
    g_trmLogConfig.enable_module_name = 1;
    g_trmLogConfig.enable_file_info = 1;  /* 启用文件信息（包含函数名和行数） */
    g_trmLogCallback = NULL;
    
    TRM_LogOutput(TRM_LOG_INFO, "TRM", __FILE__, __LINE__, __func__, 
                  "TRM日志系统初始化完成 - 级别:%s", TRM_LogGetLevelName(level));
}

/**
 * @brief 配置TRM日志系统
 * @param level 日志级别
 * @return 0-成功, 1-失败
 */
int TRM_LogConfig(TRMLogLevel level)
{
    g_trmLogConfig.level = level;
    g_trmLogConfig.enable_timestamp = 1;
    g_trmLogConfig.enable_module_name = 1;
    g_trmLogConfig.enable_file_info = 1;
    g_trmLogCallback = NULL;
    
    TRM_LogOutput(TRM_LOG_INFO, "TRM", __FILE__, __LINE__, __func__, 
                  "TRM日志系统配置完成 - 级别:%s", TRM_LogGetLevelName(level));
    
    return 0;
}


/**
 * @brief 设置TRM日志级别
 * @param level 日志级别
 */
void TRM_LogSetLevel(TRMLogLevel level)
{
    g_trmLogConfig.level = level;
}

/**
 * @brief 设置TRM日志输出回调
 * @param callback 回调函数
 */
void TRM_LogSetCallback(TRMLogCallback callback)
{
    g_trmLogCallback = callback;
}

/**
 * @brief 启用/禁用时间戳
 * @param enable 是否启用
 */
void TRM_LogEnableTimestamp(uint8_t enable)
{
    g_trmLogConfig.enable_timestamp = enable;
}

/**
 * @brief 启用/禁用模块名
 * @param enable 是否启用
 */
void TRM_LogEnableModuleName(uint8_t enable)
{
    g_trmLogConfig.enable_module_name = enable;
}

/**
 * @brief 启用/禁用文件信息
 * @param enable 是否启用
 */
void TRM_LogEnableFileInfo(uint8_t enable)
{
    g_trmLogConfig.enable_file_info = enable;
}
