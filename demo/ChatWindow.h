#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

namespace Ui {
class ChatWindow;
}

namespace plotting {
class PlottingClient;
}

/**
 * @brief 简易的人机对话窗口，演示如何使用中间件 DLL 与服务器交互。
 */
class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow();

private slots:
    void onSendClicked();
    void onConnectClicked();

private:
    void appendMessage(const QString &sender, const QString &text);

    std::unique_ptr<Ui::ChatWindow> ui;
    std::unique_ptr<plotting::PlottingClient> client_;
};

