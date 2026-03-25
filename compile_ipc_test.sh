#!/bin/bash

# 简化的编译脚本 - 仅编译TestTRMmain_IPC
echo "编译TestTRMmain_IPC..."

# 检查交叉编译器
if ! command -v arm-buildroot-linux-gnueabihf-gcc &> /dev/null; then
    echo "错误: 未找到交叉编译器"
    exit 1
fi

# 编译选项
CFLAGS="-Wall -Wextra -Wno-unused-parameter -O2 -DPLATFORM_RK3506"
INCLUDES="-I./inc -I./inc/driver -I./inc/trm -I./port -I./port/rk3506 -I../../../核间通信/0323/0323/spi/inc -I../../../核间通信/0323/0323"

BUILD_DIR="build_rk3506"
mkdir -p ${BUILD_DIR}

# 编译核间通信模块
echo "编译核间通信模块..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c test/example/tk8710_ipc_comm.c \
    -o ${BUILD_DIR}/tk8710_ipc_comm.o

if [ $? -eq 0 ]; then
    echo "✅ tk8710_ipc_comm.c 编译成功"
else
    echo "❌ tk8710_ipc_comm.c 编译失败"
    exit 1
fi

# 编译验证器模块
echo "编译验证器模块..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c test/example/trm_tx_validator.c \
    -o ${BUILD_DIR}/trm_tx_validator.o

if [ $? -eq 0 ]; then
    echo "✅ trm_tx_validator.c 编译成功"
else
    echo "❌ trm_tx_validator.c 编译失败"
    exit 1
fi

# 编译TestTRMmain_IPC
echo "编译TestTRMmain_IPC..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    test/example/TestTRMmain_IPC.c \
    ${BUILD_DIR}/tk8710_ipc_comm.o \
    ${BUILD_DIR}/trm_tx_validator.o \
    -L${BUILD_DIR} -ltk8710_hal_complete \
    -lpthread -lgpiod -ljson-c -lmbedtls -lmbedx509 -lmbedcrypto \
    -o ${BUILD_DIR}/TestTRMmain_IPC

if [ $? -eq 0 ]; then
    echo "✅ TestTRMmain_IPC 编译成功"
else
    echo "❌ TestTRMmain_IPC 编译失败"
    exit 1
fi

echo "编译完成！"
