#include "PlottingMiddleware.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    using socket_len_t = int;
#else
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
    using socket_len_t = socklen_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace plotting {

namespace {
constexpr uint32_t kSocketTimeoutMs = 5000; // 读写超时时间

// 设置阻塞模式下的超时，确保网络异常时尽快返回。
void set_socket_timeout(int socket_fd) {
#ifdef _WIN32
    DWORD timeout = kSocketTimeoutMs;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
#else
    timeval tv{};
    tv.tv_sec = kSocketTimeoutMs / 1000;
    tv.tv_usec = (kSocketTimeoutMs % 1000) * 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

// 简单的 32bit 大端序列化函数，方便在不同平台间传输。
std::vector<uint8_t> pack_length(uint32_t value) {
    uint32_t network_value = htonl(value);
    auto *ptr = reinterpret_cast<uint8_t *>(&network_value);
    return {ptr, ptr + sizeof(uint32_t)};
}

} // namespace

PlottingClient::PlottingClient(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port), socket_fd_(INVALID_SOCKET) {}

PlottingClient::~PlottingClient() {
    disconnect();
}

bool PlottingClient::connect() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (connected_) {
        return true;
    }

    socket_fd_ = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
    if (socket_fd_ == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
#ifdef _WIN32
        closesocket(socket_fd_);
        WSACleanup();
#else
        close(socket_fd_);
#endif
        return false;
    }

    if (::connect(socket_fd_, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
#ifdef _WIN32
        closesocket(socket_fd_);
        WSACleanup();
#else
        close(socket_fd_);
#endif
        return false;
    }

    set_socket_timeout(socket_fd_);
    connected_ = true;
    return true;
}

void PlottingClient::disconnect() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (!connected_) {
        return;
    }
#ifdef _WIN32
    closesocket(socket_fd_);
    WSACleanup();
#else
    close(socket_fd_);
#endif
    connected_ = false;
}

bool PlottingClient::sendCommand(const std::string &command) {
    std::vector<uint8_t> data(command.begin(), command.end());
    return sendMessage("CMD", data);
}

bool PlottingClient::sendData(const std::vector<uint8_t> &payload) {
    return sendMessage("DAT", payload);
}

std::optional<std::string> PlottingClient::requestResponse(const std::string &command) {
    if (!sendCommand(command)) {
        return std::nullopt;
    }
    return receiveMessage();
}

bool PlottingClient::isConnected() const {
    return connected_;
}

bool PlottingClient::sendMessage(const std::string &tag, const std::vector<uint8_t> &payload) {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (!connected_) {
        return false;
    }
    if (tag.size() != 3) {
        return false;
    }

    // 简单的自定义协议: [3字节类型标签][4字节长度][数据体]
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), tag.begin(), tag.end());
    auto length_bytes = pack_length(static_cast<uint32_t>(payload.size()));
    buffer.insert(buffer.end(), length_bytes.begin(), length_bytes.end());
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    size_t total_sent = 0;
    while (total_sent < buffer.size()) {
        auto sent = ::send(socket_fd_, reinterpret_cast<const char *>(buffer.data() + total_sent),
                           static_cast<int>(buffer.size() - total_sent), 0);
        if (sent == SOCKET_ERROR) {
            connected_ = false;
            return false;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return true;
}

std::optional<std::string> PlottingClient::receiveMessage() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    if (!connected_) {
        return std::nullopt;
    }

    char header[7]{}; // 3 字节标签 + 4 字节长度
    size_t received = 0;
    while (received < sizeof(header)) {
        auto ret = ::recv(socket_fd_, header + received, static_cast<int>(sizeof(header) - received), 0);
        if (ret <= 0) {
            connected_ = false;
            return std::nullopt;
        }
        received += static_cast<size_t>(ret);
    }

    uint32_t length = 0;
    std::memcpy(&length, header + 3, sizeof(uint32_t));
    length = ntohl(length);
    if (length == 0) {
        return std::string();
    }

    std::vector<char> buffer(length);
    received = 0;
    while (received < length) {
        auto ret = ::recv(socket_fd_, buffer.data() + received, static_cast<int>(length - received), 0);
        if (ret <= 0) {
            connected_ = false;
            return std::nullopt;
        }
        received += static_cast<size_t>(ret);
    }
    return std::string(buffer.begin(), buffer.end());
}

} // namespace plotting

extern "C" {

plotting::PlottingClient *create_client(const char *host, uint16_t port) {
    if (!host) {
        return nullptr;
    }
    try {
        return new plotting::PlottingClient(host, port);
    } catch (...) {
        return nullptr;
    }
}

void destroy_client(plotting::PlottingClient *client) {
    delete client;
}

bool client_send_command(plotting::PlottingClient *client, const char *command) {
    if (!client || !command) {
        return false;
    }
    return client->sendCommand(command);
}

bool client_send_data(plotting::PlottingClient *client, const uint8_t *data, uint32_t length) {
    if (!client || !data) {
        return false;
    }
    return client->sendData(std::vector<uint8_t>(data, data + length));
}

const char *client_request_response(plotting::PlottingClient *client, const char *command) {
    if (!client || !command) {
        return nullptr;
    }
    auto result = client->requestResponse(command);
    if (!result) {
        return nullptr;
    }
    // 由调用者通过 client_free_response 释放
    char *buffer = new char[result->size() + 1];
    std::memcpy(buffer, result->c_str(), result->size());
    buffer[result->size()] = '\0';
    return buffer;
}

void client_free_response(const char *response) {
    delete[] response;
}

} // extern "C"

