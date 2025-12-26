# IntelligentPlotting

该工程包含两个组件：

1. **动态链接库（plotting_middleware）**：提供 TCP 客户端能力，业务机可通过 DLL 向部署在服务器的标绘大模型发送指令或标绘数据，并接收回复。
2. **Qt Demo（chat_demo）**：运行于业务机端的示例界面，模拟 ChatGPT 式对话流程，展示如何调用 DLL 与服务器交互。

## 目录结构

```
CMakeLists.txt          # 构建入口，可选择是否编译 Demo
include/PlottingMiddleware.h
src/PlottingMiddleware.cpp
Demo/ChatWindow.*
Demo/main.cpp
```

## 功能说明

### DLL（plotting_middleware）
- 封装 TCP 连接、发送指令/标绘数据、等待回复的流程。
- 自定义轻量协议：`[3 字节类型标签][4 字节大端长度][数据体]`，便于服务端解析。
- 提供 C++ 面向对象接口 `plotting::PlottingClient` 以及 C 语言接口（方便其他语言调用）。
- 跨平台考虑：包含 Windows（Winsock）与 Linux（BSD Socket）分支。

### Qt Demo（chat_demo）
- 界面元素：历史对话区、服务器地址/端口输入、连接按钮、输入框和发送按钮。
- 主要流程：
  1. 输入服务器 IP/端口，点击“连接”建立链路；
  2. 在输入框输入指令或问题，点击“发送”；
  3. Demo 通过 DLL 发送指令并等待服务端返回一条回复；
  4. 聊天记录区按时间戳显示“我”和“服务器”的消息。
- 若未找到 Qt5/Qt6，CMake 会跳过 Demo 构建，确保 DLL 仍可生成。

## 编译步骤

```bash
# 生成构建目录
cmake -S . -B build
# 编译（若检测到 Qt，将同时编译 chat_demo）
cmake --build build
```

如需关闭 Demo，可在 CMake 配置时添加：

```bash
cmake -S . -B build -DBUILD_DEMO=OFF
```

在 Windows 平台上，生成的 DLL 位于 `build/` 下（名称类似 `plotting_middleware.dll`）；在 Linux/macOS 上则为 `.so/.dylib`。

## 使用示例

```cpp
#include "PlottingMiddleware.h"

int main() {
    plotting::PlottingClient client("127.0.0.1", 5000);
    if (!client.connect()) {
        return 1;
    }
    client.sendCommand("{\"action\":\"ping\"}");
    auto reply = client.requestResponse("你好，模型！");
    if (reply) {
        // 处理 reply
    }
    client.disconnect();
    return 0;
}
```

C 语言示例：

```c
plotting::PlottingClient *client = create_client("127.0.0.1", 5000);
client_send_command(client, "{\"action\":\"ping\"}");
const char *resp = client_request_response(client, "demo");
if (resp) {
    // 使用 resp
    client_free_response(resp);
}
destroy_client(client);
```

## 注意事项
- Demo 与 DLL 默认使用 TCP，同步阻塞等待回复，适合小规模交互演示。实际接入可根据业务需求改造成异步或多线程模式。
- 自定义协议仅为示例，可与服务器端约定更复杂的报文格式（如 JSON-RPC、gRPC 等）。
- 发送前请确保服务器已启动并监听对应端口，否则连接会失败。

