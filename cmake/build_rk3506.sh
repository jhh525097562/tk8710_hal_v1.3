#!/bin/bash
# 完整编译脚本 - 编译所有可编译的模块

echo "TK8710 RK3506 完整编译脚本"
echo "=========================="

# 检查交叉编译器
if ! command -v arm-buildroot-linux-gnueabihf-gcc &> /dev/null; then
    echo "错误: 未找到交叉编译器"
    echo "请运行: source ~/arm-buildroot-linux-gnueabihf_sdk-buildroot/environment-setup"
    exit 1
fi

echo "交叉编译器: $(arm-buildroot-linux-gnueabihf-gcc --version | head -n1)"

# 创建构建目录
BUILD_DIR="build_rk3506"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}

# 编译选项
CFLAGS="-Wall -Wextra -Wno-unused-parameter -O2 -DPLATFORM_RK3506"
INCLUDES="-I./inc -I./inc/driver -I./inc/trm -I./port -I./port/rk3506"

echo ""
echo "编译所有模块..."

# 编译公共模块
echo "编译公共模块..."
files=("src/driver/tk8710_log.c")
for file in "${files[@]}"; do
    echo "编译 $file..."
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -c $file -o ${BUILD_DIR}/$(basename $file .c).o
    if [ $? -eq 0 ]; then
        echo "✅ $file 编译成功"
    else
        echo "❌ $file 编译失败"
        exit 1
    fi
done

# 编译驱动模块（跳过有问题的文件）
echo "编译驱动模块..."
driver_files=("src/driver/tk8710_config.c" "src/driver/tk8710_irq.c" "src/driver/tk8710_core.c")
for file in "${driver_files[@]}"; do
    echo "编译 $file..."
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -c $file -o ${BUILD_DIR}/$(basename $file .c).o
    if [ $? -eq 0 ]; then
        echo "✅ $file 编译成功"
    else
        echo "❌ $file 编译失败，跳过..."
    fi
done

# 编译TRM模块
echo "编译TRM模块..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c src/trm/trm_core.c \
    -o ${BUILD_DIR}/trm_core.o

if [ $? -eq 0 ]; then
    echo "✅ src/trm/trm_core.c 编译成功"
else
    echo "❌ src/trm/trm_core.c 编译失败"
    exit 1
fi

arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c src/trm/trm_beam.c \
    -o ${BUILD_DIR}/trm_beam.o

if [ $? -eq 0 ]; then
    echo "✅ src/trm/trm_beam.c 编译成功"
else
    echo "❌ src/trm/trm_beam.c 编译失败"
    exit 1
fi

arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c src/trm/trm_data.c \
    -o ${BUILD_DIR}/trm_data.o

if [ $? -eq 0 ]; then
    echo "✅ src/trm/trm_data.c 编译成功"
else
    echo "❌ src/trm/trm_data.c 编译失败"
    exit 1
fi

arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
    -c src/trm/trm_log.c \
    -o ${BUILD_DIR}/trm_log.o

if [ $? -eq 0 ]; then
    echo "✅ src/trm/trm_log.c 编译成功"
else
    echo "❌ src/trm/trm_log.c 编译失败"
    exit 1
fi

# 尝试编译RK3506 port
echo "编译RK3506 port..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -c port/tk8710_rk3506.c -o ${BUILD_DIR}/tk8710_rk3506.o
if [ $? -eq 0 ]; then
    echo "✅ tk8710_rk3506.c 编译成功"
else
    echo "❌ tk8710_rk3506.c 编译失败，跳过..."
fi

# 编译HAL API模块
echo "编译HAL API模块..."
arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -c src/8710_hal_api.c -o ${BUILD_DIR}/8710_hal_api.o
if [ $? -eq 0 ]; then
    echo "✅ 8710_hal_api.c 编译成功"
else
    echo "❌ 8710_hal_api.c 编译失败"
    exit 1
fi

# 创建静态库
echo ""
echo "创建静态库..."
ar rcs ${BUILD_DIR}/libtk8710_hal_complete.a \
    ${BUILD_DIR}/tk8710_core.o \
    ${BUILD_DIR}/tk8710_config.o \
    ${BUILD_DIR}/tk8710_irq.o \
    ${BUILD_DIR}/tk8710_log.o \
    ${BUILD_DIR}/tk8710_rk3506.o \
    ${BUILD_DIR}/trm_core.o \
    ${BUILD_DIR}/trm_beam.o \
    ${BUILD_DIR}/trm_data.o \
    ${BUILD_DIR}/trm_log.o \
    ${BUILD_DIR}/8710_hal_api.o

if [ $? -eq 0 ]; then
    echo "✅ 静态库创建成功"
else
    echo "❌ 静态库创建失败"
    exit 1
fi

# 创建示例程序
echo ""
echo "创建示例程序..."

# 创建 test8710main_3506
if [ -f "test/example/test8710main_3506.c" ]; then
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
        test/example/test8710main_3506.c \
        -L${BUILD_DIR} -ltk8710_hal_complete \
        -lpthread -lgpiod \
        -o ${BUILD_DIR}/test8710main_3506
    
    if [ $? -eq 0 ]; then
        echo "✅ test8710main_3506 创建成功"
    else
        echo "❌ test8710main_3506 创建失败"
    fi
else
    echo "⚠️  test8710main_3506 源文件不存在"
fi

# 创建 TestTRMmain
if [ -f "test/example/TestTRMmain.c" ]; then
    # 先编译验证器模块
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
    
    # 编译测试程序并链接验证器
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port \
        test/example/TestTRMmain.c \
        ${BUILD_DIR}/trm_tx_validator.o \
        -L${BUILD_DIR} -ltk8710_hal_complete \
        -lpthread -lgpiod \
        -o ${BUILD_DIR}/TestTRMmain
    
    if [ $? -eq 0 ]; then
        echo "✅ TestTRMmain 创建成功"
    else
        echo "❌ TestTRMmain 创建失败"
    fi
else
    echo "⚠️  TestTRMmain 源文件不存在"
fi

# 创建 Test8710Slave
if [ -f "test/DriverTest/Test8710Slave.c" ]; then
    echo "编译 Test8710Slave..."
    # 先编译验证器模块
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
    
    # 编译Test8710Slave并链接验证器
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port -I./test/example \
        test/DriverTest/Test8710Slave.c \
        ${BUILD_DIR}/trm_tx_validator.o \
        -L${BUILD_DIR} -ltk8710_hal_complete \
        -lpthread -lgpiod \
        -o ${BUILD_DIR}/Test8710Slave
    
    if [ $? -eq 0 ]; then
        echo "✅ Test8710Slave 创建成功"
    else
        echo "❌ Test8710Slave 创建失败"
    fi
else
    echo "⚠️  Test8710Slave 源文件不存在"
fi

# 创建 Test8710RxSensitivity
if [ -f "test/DriverTest/Test8710RxSensitivity.c" ]; then
    echo "编译 Test8710RxSensitivity..."
    # 先编译验证器模块
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
    
    # 编译Test8710RxSensitivity并链接验证器
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port -I./test/example \
        test/DriverTest/Test8710RxSensitivity.c \
        ${BUILD_DIR}/trm_tx_validator.o \
        -L${BUILD_DIR} -ltk8710_hal_complete \
        -lpthread -lgpiod \
        -o ${BUILD_DIR}/Test8710RxSensitivity
    
    if [ $? -eq 0 ]; then
        echo "✅ Test8710RxSensitivity 创建成功"
    else
        echo "❌ Test8710RxSensitivity 创建失败"
    fi
else
    echo "⚠️  Test8710RxSensitivity 源文件不存在"
fi

# 创建 Test8710P2P
if [ -f "test/DriverTest/Test8710P2P.c" ]; then
    echo "编译 Test8710P2P..."
    # 先编译验证器模块
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
    
    # 编译Test8710P2P并链接验证器
    arm-buildroot-linux-gnueabihf-gcc ${CFLAGS} ${INCLUDES} -I./port -I./test/example \
        test/DriverTest/Test8710P2P.c \
        ${BUILD_DIR}/trm_tx_validator.o \
        -L${BUILD_DIR} -ltk8710_hal_complete \
        -lpthread -lgpiod \
        -o ${BUILD_DIR}/Test8710P2P
    
    if [ $? -eq 0 ]; then
        echo "✅ Test8710P2P 创建成功"
    else
        echo "❌ Test8710P2P 创建失败"
    fi
else
    echo "⚠️  Test8710P2P 源文件不存在"
fi


# 显示结果
echo ""
echo "=========================="
echo "编译完成！"
echo "输出文件:"
ls -la ${BUILD_DIR}/
echo ""
echo "库文件信息:"
if [ -f "${BUILD_DIR}/libtk8710_hal_complete.a" ]; then
    echo "静态库大小: $(stat -c%s ${BUILD_DIR}/libtk8710_hal_complete.a) 字节"
    echo "包含的目标文件:"
    arm-buildroot-linux-gnueabihf-ar t ${BUILD_DIR}/libtk8710_hal_complete.a
fi
echo "=========================="

# 检查是否可以运行（在ARM设备上）
echo ""
echo "注意：生成的程序适用于ARM平台，需要在RK3506设备上运行。"
