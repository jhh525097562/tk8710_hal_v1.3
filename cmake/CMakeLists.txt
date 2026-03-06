cmake_minimum_required(VERSION 3.10)
project(tk8710_hal VERSION 1.0.0 LANGUAGES C)

# 编译选项
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译警告
if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# 平台选择
option(PLATFORM_JTOOL "Build for JTOOL platform (Windows)" OFF)
option(PLATFORM_RK3506 "Build for RK3506 platform (Linux ARM)" OFF)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test programs" OFF)

# 检测交叉编译
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/trm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../hal
)

# 公共源文件
set(COMMON_SOURCES
    ../src/driver/tk8710_log.c
)

# Driver源文件
set(DRIVER_SOURCES
    ../src/driver/tk8710_core.c
    ../src/driver/tk8710_spi.c
    ../src/driver/tk8710_irq.c
    ../src/driver/tk8710_config.c
)

# TRM源文件
set(TRM_SOURCES
    ../src/trm/trm_core.c
    ../src/trm/trm_beam.c
    ../src/trm/trm_data.c
)

# 平台移植层源文件
if(PLATFORM_JTOOL)
    set(PORT_SOURCES
        port/jtool/tk8710_port_jtool.c
    )
    add_definitions(-DPLATFORM_JTOOL)
elseif(PLATFORM_RK3506)
    set(PORT_SOURCES
        ../port/rk3506/port_rk3506.c
    )
    add_definitions(-DPLATFORM_RK3506)
else()
    # 默认使用桩实现（用于编译验证）
    set(PORT_SOURCES
        port/tk8710_port_stub.c
    )
    add_definitions(-DPLATFORM_STUB)
endif()

# 静态库
add_library(tk8710_hal STATIC
    ${COMMON_SOURCES}
    ${DRIVER_SOURCES}
    ${TRM_SOURCES}
    ${PORT_SOURCES}
)

target_include_directories(tk8710_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

# RK3506平台需要链接pthread和gpiod
if(PLATFORM_RK3506)
    find_package(Threads REQUIRED)
    target_link_libraries(tk8710_hal PUBLIC Threads::Threads gpiod)
endif()

# 示例程序
if(BUILD_EXAMPLES)
    add_executable(basic_example
        ../test/example/basic_example.c
    )
    target_link_libraries(basic_example tk8710_hal)
    
    # RK3506平台额外链接
    if(PLATFORM_RK3506)
        target_link_libraries(basic_example pthread gpiod)
    endif()
endif()

# 测试程序
if(BUILD_TESTS)
    add_executable(test_driver
        ../test/unit/test_driver.c
    )
    target_link_libraries(test_driver tk8710_hal)
    
    add_executable(test_trm
        ../test/unit/test_trm.c
    )
    target_link_libraries(test_trm tk8710_hal)
endif()

# 安装
install(TARGETS tk8710_hal
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ../inc/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
