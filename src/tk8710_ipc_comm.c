#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/epoll.h>
#include <sched.h>
#include "tk8710_ipc_comm.h"
#include "8710_hal_api.h"
#include "tk8710_hal.h"
#include "trm/trm.h"

// 包含核间通信相关的头文件
#include "ipc_smp.h"
#include "mac_msg_parser.h"

// 定义IPC通道常量（如果spi_app.h不可用）
#ifndef IPC_CH_MQTT_TO_SPI
#define IPC_CH_MQTT_TO_SPI "/ipc_mqtt_to_spi"
#define IPC_CH_SPI_TO_MQTT "/ipc_spi_to_mqtt"
#define IPC_RING_SIZE (4u * 1024u * 1024u) // 扩大到4MB以支持批量数据
#define IPC_MAX_MSG_SIZE (sizeof(GwBatchDataUp_t) + 256) // 动态适配结构体大小
#define IPC_BATCH_COUNT 8u
#define IPC_FLUSH_TIMEOUT_MS 20u
#define IPC_MSG_TYPE_MQTT_TO_SPI 0x1001u
#define IPC_MSG_TYPE_SPI_TO_MQTT 0x2001u
#endif

// 全局配置状态标志
static volatile int g_config_received = 0;
static NsConfigDown_t g_received_config;

// 配置处理回调函数类型
typedef int (*ConfigHandler_t)(const NsConfigDown_t* config);
static ConfigHandler_t g_config_handler = NULL;

// 设置配置处理回调函数
void IpcCommSetConfigHandler(ConfigHandler_t handler) {
    g_config_handler = handler;
}

// 检查是否已收到配置
int IpcCommIsConfigReceived(void) {
    return g_config_received;
}

// 获取收到的配置
int IpcCommGetReceivedConfig(NsConfigDown_t* config) {
    if (!g_config_received || !config) {
        return -1;
    }
    memcpy(config, &g_received_config, sizeof(NsConfigDown_t));
    return 0;
}

// 发送配置请求
int IpcCommSendConfigRequest(IpcCommContext *ctx) {
    if (!ctx || !ctx->ch_gw_to_ns) {
        printf("❌ IPC通道未初始化\n");
        return -1;
    }
    
    ConfigReq_t config_req = {
        .msg_type = MSG_TYPE_CONFIG_FEQ,
        .req_data_type = 1  // 1表示请求配置
    };
    
    int rc = ipc_smp_send(ctx->ch_gw_to_ns, IPC_MSG_TYPE_SPI_TO_MQTT, 
                          &config_req, sizeof(config_req), 0);
    if (rc == IPC_SMP_OK) {
        printf("📤 已发送配置请求消息\n");
        ipc_smp_periodic_check(ctx->ch_gw_to_ns);
        return 0;
    } else {
        printf("❌ 发送配置请求失败: %d\n", rc);
        return -1;
    }
}
// 打印消息详情
static void PrintMessageDetail(const char *prefix, const ipc_smp_msg_hdr_t *hdr, const void *payload, uint32_t payload_len) {
    printf("[%s] 消息头: ver=%u type=%u len=%u seq=%u ts_ns=%llu flags=%u crc32=0x%08X\n",
           prefix,
           (unsigned)hdr->version,
           (unsigned)hdr->type,
           (unsigned)hdr->len,
           (unsigned)hdr->seq,
           (unsigned long long)hdr->ts_ns,
           (unsigned)hdr->flags,
           (unsigned)hdr->crc32);
    
    if (payload && payload_len >= sizeof(MacMsgType_e)) {
        MacMsgType_e *msg_type_ptr = (MacMsgType_e *)payload;
        
        switch (*msg_type_ptr) {
            case MSG_TYPE_NS_CONFIG_DOWN: {
                NsConfigDown_t* config = (NsConfigDown_t*)payload;
                printf("收到NS配置下行消息: freq=%u, nwk_num=%d, tdd_num=%d, slot_cfg=%d, rate_num=%d\n",
                       config->freq, config->nwk_num, config->tdd_num, config->slot_cfg, config->rate_num);
                
                // 打印速率配置详情
                for (int i = 0; i < config->rate_num && i < MAX_RATE_CFGS; i++) {
                    printf("  速率配置[%d]: rate=%d, uplink_pkt=%d, downlink_pkt=%d\n",
                           i, config->rate_cfgs[i].rate, config->rate_cfgs[i].uplink_pkt, config->rate_cfgs[i].downlink_pkt);
                }
                
                // 保存配置并设置标志
                memcpy(&g_received_config, config, sizeof(NsConfigDown_t));
                g_config_received = 1;
                
                // 调用配置处理回调函数
                if (g_config_handler) {
                    printf("✅ 配置处理成功\n");
                } else {
                    printf("⚠️  未设置配置处理回调函数\n");
                }
                break;
            }
            case MSG_TYPE_NS_DATA_DOWN: {
                if (payload_len >= offsetof(NsDataDown_t, payload)) {
                    const NsDataDown_t *ns_data = (const NsDataDown_t *)payload;
                    printf("  [NS数据下行] TDD=%d, 速率=%d, 时隙=%d, 载荷长度=%zu\n",
                           ns_data->tdd, ns_data->rate, ns_data->slot, ns_data->payload_len);
                    printf("    载荷数据: ");
                    for (size_t i = 0; i < ns_data->payload_len && i < 16; i++) {
                        printf("%02X ", ns_data->payload[i]);
                    }
                    if (ns_data->payload_len > 16) printf("...");
                    printf("\n");
                }
                break;
            }
            default:
                printf("  [未知消息类型] 类型=%d\n", *msg_type_ptr);
                break;
        }
    }
    printf("\n");
}

// IPC通道初始化（先尝试附加，失败则创建）
static int IpcOpenAttachOrCreate(const ipc_smp_cfg_t *base_cfg, ipc_smp_handle_t **out) {
    ipc_smp_cfg_t cfg = *base_cfg;
    int rc;

    // 尝试附加到现有共享内存
    cfg.create = 0;
    rc = ipc_smp_channel_init(&cfg, out);
    if (rc == IPC_SMP_OK) {
        printf("成功附加到现有共享内存: %s\n", cfg.shm_name);
        return IPC_SMP_OK;
    }

    // 创建新的共享内存
    printf("附加失败，创建新的共享内存: %s\n", cfg.shm_name);
    cfg.create = 1;
    rc = ipc_smp_channel_init(&cfg, out);
    if (rc != IPC_SMP_OK) {
        fprintf(stderr, "创建共享内存失败 %s: %d\n", cfg.shm_name, rc);
    }
    return rc;
}

// 处理从NS接收的消息
static void ProcessIncomingMessages(IpcCommContext *ctx) {
    ipc_smp_msg_t msg;
    int rc;

    while (1) {
        rc = ipc_smp_recv(ctx->ch_ns_to_gw, &msg, 0);  // 非阻塞接收
        if (rc == IPC_SMP_ERR_EMPTY) {
            break;  // 没有更多消息
        }
        if (rc != IPC_SMP_OK) {
            fprintf(stderr, "IPC接收失败: %d\n", rc);
            break;
        }

        // 获取消息头用于打印
        const ipc_smp_msg_hdr_t *hdr = (const ipc_smp_msg_hdr_t *)
            ((const uint8_t *)msg.payload - sizeof(ipc_smp_msg_hdr_t));
        
        PrintMessageDetail("RX: NS->网关", hdr, msg.payload, msg.len);

        // 处理不同类型的消息
        MacMsgType_e *msg_type_ptr = (MacMsgType_e *)msg.payload;
        switch (*msg_type_ptr) {
            case MSG_TYPE_NS_CONFIG_DOWN: {
                // NS配置下行消息 - 用于hal_init配置
                if (msg.len >= sizeof(NsConfigDown_t)) {
                    const NsConfigDown_t *config = (const NsConfigDown_t *)msg.payload;
                    printf("收到NS配置消息，应用新配置...\n");
                    
                    // 保存配置并设置标志
                    memcpy(&g_received_config, config, sizeof(NsConfigDown_t));
                    g_config_received = 1;
                    
                    // 调用配置处理回调函数
                    if (g_config_handler) {
                        int ret = g_config_handler(config);
                        if (ret == 0) {
                            printf("✅ 配置处理成功\n");
                        } else {
                            printf("❌ 配置处理失败: %d\n", ret);
                        }
                    } else {
                        printf("⚠️  未设置配置处理回调函数\n");
                    }
                    
                }
                break;
            }
            case MSG_TYPE_NS_DATA_DOWN: {
                // NS数据下行消息 - 调用hal_sendData发送
                if (msg.len >= offsetof(NsDataDown_t, payload)) {
                    const NsDataDown_t *ns_data = (const NsDataDown_t *)msg.payload;
                    printf("收到NS下行数据，调用hal_sendData发送...\n");
                    
                    // 调用HAL接口发送数据
                    TK8710_HalError halRet = hal_sendData(
                        ns_data->slot == 1 ? TK8710_DOWNLINK_A : TK8710_DOWNLINK_B,  // slot=1->A, slot=3->B
                        0,                  // 用户ID（示例）
                        ns_data->payload,         // 数据载荷
                        ns_data->payload_len,     // 数据长度
                        35,                       // 发射功率
                        0,                        // 帧号
                        ns_data->rate,            // 目标速率模式
                        ns_data->tdd == 255 ? TK8710_DATA_TYPE_DED : TK8710_DATA_TYPE_BRD  // tdd=255->DED, tdd=0->BRD
                        // TK8710_DATA_TYPE_BRD // tdd=255->DED, tdd=0->BRD
                    );
                    
                    if (halRet == TK8710_HAL_OK) {
                        printf("hal_sendData调用成功\n");
                    } else {
                        printf("hal_sendData调用失败: %d\n", halRet);
                    }
                }
                break;
            }
            default:
                printf("收到未知类型消息: %d\n", *msg_type_ptr);
                break;
        }

        // 确认消息处理完成
        ipc_smp_ack(ctx->ch_ns_to_gw, msg.seq);
    }
}

// 核间通信线程主函数
static void* IpcCommThread(void* arg) {
    IpcCommContext *ctx = (IpcCommContext*)arg;
    
    printf("核间通信线程启动\n");
    
    // 创建epoll用于事件处理
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return NULL;
    }

    // 添加接收通道的事件通知FD
    int notify_fd = ipc_smp_get_eventfd(ctx->ch_ns_to_gw);
    if (notify_fd >= 0) {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = notify_fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, notify_fd, &ev);
    }

    printf("核间通信线程进入主循环\n");
    
    struct epoll_event events[4];
    while (ctx->running) {
        // 等待事件，超时100ms
        int nfds = epoll_wait(epfd, events, 4, 100);

        if (nfds > 0) {
            for (int i = 0; i < nfds; i++) {
                if (events[i].data.fd == notify_fd) {
                    ipc_smp_clear_notify(ctx->ch_ns_to_gw);
                    ProcessIncomingMessages(ctx);
                }
            }
        }

        // 定期检查发送通道（必要时刷新）
        ipc_smp_periodic_check(ctx->ch_gw_to_ns);
    }

    close(epfd);
    printf("核间通信线程结束\n");
    
    return NULL;
}

// 初始化核间通信
int IpcCommInit(IpcCommContext *ctx) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(IpcCommContext));
    ctx->running = 1;

    printf("=== 初始化核间通信 ===\n");

    // 1. 初始化接收通道（NS -> 网关）
    ipc_smp_cfg_t rx_cfg = {0};
    rx_cfg.shm_name = IPC_CH_MQTT_TO_SPI;
    rx_cfg.ring_size = IPC_RING_SIZE;
    rx_cfg.max_msg_size = IPC_MAX_MSG_SIZE;
    rx_cfg.batch_count = IPC_BATCH_COUNT;
    rx_cfg.flush_timeout_ms = IPC_FLUSH_TIMEOUT_MS;
    rx_cfg.role = IPC_SMP_ROLE_CONSUMER;

    if (IpcOpenAttachOrCreate(&rx_cfg, &ctx->ch_ns_to_gw) != IPC_SMP_OK) {
        fprintf(stderr, "初始化NS->网关接收通道失败\n");
        return -1;
    }

    // 2. 初始化发送通道（网关 -> NS）
    ipc_smp_cfg_t tx_cfg = {0};
    tx_cfg.shm_name = IPC_CH_SPI_TO_MQTT;
    tx_cfg.ring_size = IPC_RING_SIZE;
    tx_cfg.max_msg_size = IPC_MAX_MSG_SIZE;
    tx_cfg.batch_count = IPC_BATCH_COUNT;
    tx_cfg.flush_timeout_ms = IPC_FLUSH_TIMEOUT_MS;
    tx_cfg.role = IPC_SMP_ROLE_PRODUCER;

    if (IpcOpenAttachOrCreate(&tx_cfg, &ctx->ch_gw_to_ns) != IPC_SMP_OK) {
        fprintf(stderr, "初始化网关->NS发送通道失败\n");
        return -1;
    }

    printf("核间通信初始化完成\n");
    return 0;
}

// 启动核间通信线程
int IpcCommStart(IpcCommContext *ctx) {
    if (!ctx || !ctx->ch_ns_to_gw || !ctx->ch_gw_to_ns) {
        return -1;
    }

    if (pthread_create(&ctx->thread, NULL, IpcCommThread, ctx) != 0) {
        perror("pthread_create");
        return -1;
    }

    printf("核间通信线程已启动\n");
    return 0;
}

// 停止核间通信
int IpcCommStop(IpcCommContext *ctx) {
    if (!ctx) {
        return -1;
    }
    
    printf("正在停止核间通信...\n");
    
    ctx->running = 0;
    
    // 等待线程结束
    if (ctx->thread) {
        pthread_join(ctx->thread, NULL);
        ctx->thread = 0;
    }
    
    printf("核间通信已停止\n");
    return 0;
}

// 清理核间通信资源
void IpcCommCleanup(IpcCommContext *ctx) {
    if (!ctx) return;
    
    printf("=== 清理核间通信 ===\n");
    
    if (ctx->ch_ns_to_gw) {
        ipc_smp_channel_close(ctx->ch_ns_to_gw);
        ctx->ch_ns_to_gw = NULL;
    }
    
    if (ctx->ch_gw_to_ns) {
        ipc_smp_channel_close(ctx->ch_gw_to_ns);
        ctx->ch_gw_to_ns = NULL;
    }
}

// 发送上行数据给NS
int IpcSendUplinkData(IpcCommContext *ctx, const TRM_RxDataList* rxDataList) {
    if (!ctx || !ctx->ch_gw_to_ns || !rxDataList || rxDataList->userCount == 0) {
        return -1;
    }

    // 使用批量数据上行结构体，准备所有用户数据后一次性发送
    GwBatchDataUp_t batch_data;
    memset(&batch_data, 0, sizeof(GwBatchDataUp_t));
    
    batch_data.msg_type = MSG_TYPE_GW_DATA_UP;
    batch_data.total_count = (rxDataList->userCount < MAX_UP_USER_NUM) ? rxDataList->userCount : MAX_UP_USER_NUM;
    
    // 为每个用户准备数据
    for (uint8_t i = 0; i < batch_data.total_count; i++) {
        const TRM_RxUserData* user = &rxDataList->users[i];
        GwDataUp_t* gw_data = &batch_data.data_list[i];
        
        gw_data->tdd = rxDataList->frameNo;  // 可以根据实际情况设置
        gw_data->rate = user->rateMode;
        gw_data->slot = 0;  // 示例时隙计算
        gw_data->rssi = user->rssi;
        gw_data->snr = user->snr;

        // 复制用户数据
        gw_data->payload_len = user->dataLen;
        if (gw_data->payload_len > MAX_PAYLOAD_LEN) {
            gw_data->payload_len = MAX_PAYLOAD_LEN;
        }
        memcpy(gw_data->payload, user->data, gw_data->payload_len);
    }

    // 计算实际发送长度
    uint32_t len = offsetof(GwBatchDataUp_t, data_list) + 
                   (batch_data.total_count * sizeof(GwDataUp_t));
    
    // 一次性发送批量数据
    int rc = ipc_smp_send(ctx->ch_gw_to_ns, IPC_MSG_TYPE_SPI_TO_MQTT, &batch_data, len, 0);
    if (rc == IPC_SMP_OK) {
        ipc_smp_periodic_check(ctx->ch_gw_to_ns);
        
        ipc_smp_msg_hdr_t hdr;
        if (ipc_smp_get_last_hdr(ctx->ch_gw_to_ns, &hdr) == IPC_SMP_OK) {
            PrintMessageDetail("TX: 网关->NS (批量)", &hdr, &batch_data, len);
        }
        printf("✅ 批量发送 %d 个用户上行数据成功\n", batch_data.total_count);
    } else {
        fprintf(stderr, "IPC发送批量上行数据失败: %d\n", rc);
    }

    return 0;
}
