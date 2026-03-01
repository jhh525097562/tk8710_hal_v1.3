/**
 * @file tk8710_log.c
 * @brief TK8710 Driver 日志系统实现
 */

#include "../inc/driver/tk8710_log.h"
#include "../inc/driver/tk8710.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* 全局日志配置 */
TK8710LogConfig_t g_logConfig = {
    .level = TK8710_LOG_WARN,           /* 默认警告级别 */
    .module_mask = TK8710_LOG_MODULE_ALL, /* 默认所有模块 */
    .callback = NULL,                    /* 默认无回调 */
    .enable_timestamp = 1,               /* 默认启用时间戳 */
    .enable_module_name = 1              /* 默认启用模块名 */
};

/* 模块名称映射 */
static const char* g_moduleNames[] = {
    [TK8710_LOG_MODULE_CORE]   = "CORE",
    [TK8710_LOG_MODULE_CONFIG] = "CONFIG",
    [TK8710_LOG_MODULE_IRQ]    = "IRQ",
    [TK8710_LOG_MODULE_HAL]    = "HAL",
    [TK8710_LOG_MODULE_ALL]    = "ALL"
};

/* 级别名称映射 */
static const char* g_levelNames[] = {
    [TK8710_LOG_NONE]  = "NONE",
    [TK8710_LOG_ERROR] = "ERROR",
    [TK8710_LOG_WARN]  = "WARN",
    [TK8710_LOG_INFO]  = "INFO",
    [TK8710_LOG_DEBUG] = "DEBUG",
    [TK8710_LOG_TRACE] = "TRACE",
    [TK8710_LOG_ALL]   = "ALL"
};

/**
 * @brief 默认日志输出函数
 */
static void default_log_output(TK8710LogLevel level, TK8710LogModule module,
                               const char* file, int line, const char* func,
                               const char* fmt, va_list args)
{
    char buffer[512];
    int offset = 0;
    
    /* 时间戳 */
    if (g_logConfig.enable_timestamp) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%04d-%02d-%02d %02d:%02d:%02d", 
                         tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " ");
    }
    
    /* 级别 */
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s] ", 
                     TK8710LogGetLevelName(level));
    
    /* 模块名 */
    if (g_logConfig.enable_module_name) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s] ", 
                         TK8710LogGetModuleName(module));
    }
    
    /* 文件和行号 (仅在DEBUG及以上级别) */
    if (level >= TK8710_LOG_DEBUG) {
        const char* filename = strrchr(file, '/');
        if (!filename) filename = strrchr(file, '\\');
        if (!filename) filename = file;
        else filename++;
        
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "[%s:%d:%s] ", 
                         filename, line, func);
    }
    
    /* 用户消息 */
    offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    
    /* 添加换行符 */
    if (offset < sizeof(buffer) - 1) {
        buffer[offset] = '\n';
        buffer[offset + 1] = '\0';
    }
    
    /* 输出到标准输出 */
    printf("%s", buffer);
}

/**
 * @brief 初始化日志系统
 */
int TK8710LogInit(const TK8710LogConfig_t* config)
{
    if (config == NULL) {
        return 1; /* 错误：参数为空 */
    }
    
    /* 复制配置 */
    memcpy(&g_logConfig, config, sizeof(TK8710LogConfig_t));
    
    /* 如果没有设置回调，使用默认输出 */
    if (g_logConfig.callback == NULL) {
        /* 这里不能直接设置default_log_output，因为参数类型不匹配 */
        /* 需要一个包装函数 */
    }
    
    return 0; /* 成功 */
}

/**
 * @brief 设置日志级别
 */
void TK8710LogSetLevel(TK8710LogLevel level)
{
    g_logConfig.level = level;
}

/**
 * @brief 获取当前日志级别
 */
TK8710LogLevel TK8710LogGetLevel(void)
{
    return g_logConfig.level;
}

/**
 * @brief 设置模块掩码
 */
void TK8710LogSetModuleMask(uint32_t module_mask)
{
    g_logConfig.module_mask = module_mask;
}

/**
 * @brief 获取当前模块掩码
 */
uint32_t TK8710LogGetModuleMask(void)
{
    return g_logConfig.module_mask;
}

/**
 * @brief 设置日志回调函数
 */
void TK8710LogSetCallback(TK8710LogCallback callback)
{
    g_logConfig.callback = callback;
}

/**
 * @brief 启用/禁用时间戳
 * @param enable 是否启用时间戳
 */
void TK8710LogEnableTimestamp(uint8_t enable)
{
    g_logConfig.enable_timestamp = enable;
}

/**
 * @brief 启用/禁用模块名
 * @param enable 是否启用模块名
 */
void TK8710LogEnableModuleName(uint8_t enable)
{
    g_logConfig.enable_module_name = enable;
}

/**
 * @brief 输出日志
 */
void TK8710LogOutput(TK8710LogLevel level, TK8710LogModule module,
                     const char* file, int line, const char* func,
                     const char* fmt, ...)
{
    va_list args;
    
    /* 检查级别和模块掩码 */
    if (level > g_logConfig.level || !(g_logConfig.module_mask & module)) {
        return;
    }
    
    va_start(args, fmt);
    
    if (g_logConfig.callback) {
        /* 使用用户回调 */
        g_logConfig.callback(level, module, file, line, func, fmt, args);
    } else {
        /* 使用默认输出 */
        default_log_output(level, module, file, line, func, fmt, args);
    }
    
    va_end(args);
}

/**
 * @brief 获取模块名称
 */
const char* TK8710LogGetModuleName(TK8710LogModule module)
{
    if (module >= sizeof(g_moduleNames)/sizeof(g_moduleNames[0])) {
        return "UNKNOWN";
    }
    return g_moduleNames[module];
}

/**
 * @brief 获取级别名称
 */
const char* TK8710LogGetLevelName(TK8710LogLevel level)
{
    if (level >= sizeof(g_levelNames)/sizeof(g_levelNames[0])) {
        return "UNKNOWN";
    }
    return g_levelNames[level];
}

/**
 * @brief 简化的日志初始化函数
 * @param level 日志级别
 * @param module_mask 模块掩码
 * @return 0-成功, 1-失败
 */
int TK8710LogConfig(TK8710LogLevel level, uint32_t module_mask)
{
    TK8710LogConfig_t logConfig = {
        .level = level,
        .module_mask = module_mask,
        .callback = NULL,
        .enable_timestamp = 1,
        .enable_module_name = 1
    };
    
    return TK8710LogInit(&logConfig);
}
