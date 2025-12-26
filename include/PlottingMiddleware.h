#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
    #ifdef PLOTTING_MIDDLEWARE_EXPORT
        #define PLOTTING_API __declspec(dllexport)
    #else
        #define PLOTTING_API __declspec(dllimport)
    #endif
#else
    #define PLOTTING_API
#endif

namespace plotting {

/**
 * @brief 与标绘大模型服务端通信的简易客户端。
 *
 * 该类提供基本的连接、发送指令/数据以及接收响应的能力，供业务机上的中间件 DLL 暴露接口使用。
 */
class PLOTTING_API PlottingClient {
public:
    /**
     * @brief 构造函数
     * @param host 服务器 IP 或主机名
     * @param port 服务器端口号
     */
    PlottingClient(std::string host, uint16_t port);

    ~PlottingClient();

    /**
     * @brief 与服务端建立 TCP 连接。
     * @return 连接成功返回 true，失败返回 false。
     */
    bool connect();

    /**
     * @brief 主动关闭连接并释放资源。
     */
    void disconnect();

    /**
     * @brief 发送业务指令（如控制模型、查询状态等）。
     * @param command 要发送的指令文本（建议使用 JSON 字符串）。
     * @return 成功发送返回 true，失败返回 false。
     */
    bool sendCommand(const std::string &command);

    /**
     * @brief 发送标绘数据（如模型生成的矢量、渲染参数等）。
     * @param payload 原始二进制或文本数据。
     * @return 成功发送返回 true，失败返回 false。
     */
    bool sendData(const std::vector<uint8_t> &payload);

    /**
     * @brief 请求响应：发送指令并等待一条回复，简化 Demo 的人机对话流程。
     * @param command 要发送的指令
     * @return 若接收到回复则返回字符串内容，未收到则返回 std::nullopt。
     */
    std::optional<std::string> requestResponse(const std::string &command);

    /**
     * @brief 当前是否处于已连接状态。
     */
    bool isConnected() const;

private:
    bool sendMessage(const std::string &tag, const std::vector<uint8_t> &payload);
    std::optional<std::string> receiveMessage();

    std::string host_;
    uint16_t port_;
    int socket_fd_;
    bool connected_ {false};
    std::mutex socket_mutex_;
};

} // namespace plotting

extern "C" {

/**
 * @brief C 接口：创建一个客户端实例并返回指针。
 * @note 业务机若使用 C 语言或其他语言，可通过此接口获取句柄。
 */
PLOTTING_API plotting::PlottingClient *create_client(const char *host, uint16_t port);

/**
 * @brief C 接口：销毁客户端实例。
 */
PLOTTING_API void destroy_client(plotting::PlottingClient *client);

/**
 * @brief C 接口：发送指令。
 */
PLOTTING_API bool client_send_command(plotting::PlottingClient *client, const char *command);

/**
 * @brief C 接口：发送数据。
 */
PLOTTING_API bool client_send_data(plotting::PlottingClient *client, const uint8_t *data, uint32_t length);

/**
 * @brief C 接口：请求回复。
 */
PLOTTING_API const char *client_request_response(plotting::PlottingClient *client, const char *command);

/**
 * @brief C 接口：释放 request_response 返回的内存。
 */
PLOTTING_API void client_free_response(const char *response);
}

