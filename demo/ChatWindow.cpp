#include "ChatWindow.h"

#include "PlottingMiddleware.h"

#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

using plotting::PlottingClient;

namespace Ui {
// 手工实现一个轻量级的 UI 类，避免依赖 .ui 文件。
class ChatWindow {
public:
    QTextEdit *history {nullptr};
    QLineEdit *host {nullptr};
    QLineEdit *port {nullptr};
    QLineEdit *input {nullptr};
    QPushButton *connect {nullptr};
    QPushButton *send {nullptr};

    void setupUi(QMainWindow *window) {
        window->setWindowTitle(QStringLiteral("标绘对话 Demo"));
        window->resize(520, 420);

        auto *central = new QWidget(window);
        auto *layout = new QVBoxLayout(central);

        history = new QTextEdit(central);
        history->setReadOnly(true);

        auto *connectionRow = new QWidget(central);
        auto *connectionLayout = new QHBoxLayout(connectionRow);
        connectionLayout->setContentsMargins(0, 0, 0, 0);
        host = new QLineEdit(connectionRow);
        host->setPlaceholderText(QStringLiteral("服务器 IP"));
        host->setText(QStringLiteral("127.0.0.1"));
        port = new QLineEdit(connectionRow);
        port->setPlaceholderText(QStringLiteral("端口"));
        port->setText(QStringLiteral("5000"));
        connect = new QPushButton(QStringLiteral("连接"), connectionRow);
        connectionLayout->addWidget(host);
        connectionLayout->addWidget(port);
        connectionLayout->addWidget(connect);

        input = new QLineEdit(central);
        input->setPlaceholderText(QStringLiteral("请输入指令或问题..."));
        send = new QPushButton(QStringLiteral("发送"), central);

        auto *sendRow = new QWidget(central);
        auto *sendLayout = new QHBoxLayout(sendRow);
        sendLayout->setContentsMargins(0, 0, 0, 0);
        sendLayout->addWidget(input);
        sendLayout->addWidget(send);

        layout->addWidget(history);
        layout->addWidget(connectionRow);
        layout->addWidget(sendRow);

        window->setCentralWidget(central);
    }
};
} // namespace Ui

ChatWindow::ChatWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::ChatWindow>()) {
    ui->setupUi(this);

    connect(ui->send, &QPushButton::clicked, this, &ChatWindow::onSendClicked);
    connect(ui->connect, &QPushButton::clicked, this, &ChatWindow::onConnectClicked);
}

ChatWindow::~ChatWindow() = default;

void ChatWindow::appendMessage(const QString &sender, const QString &text) {
    const auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->history->append(QStringLiteral("[%1] %2: %3").arg(timestamp, sender, text));
}

void ChatWindow::onConnectClicked() {
    const auto host = ui->host->text().trimmed();
    const auto port = ui->port->text().toUShort();
    client_ = std::make_unique<PlottingClient>(host.toStdString(), port);
    if (!client_->connect()) {
        QMessageBox::warning(this, QStringLiteral("连接失败"), QStringLiteral("无法连接到服务器，请检查地址和网络。"));
        return;
    }
    appendMessage(QStringLiteral("系统"), QStringLiteral("连接成功，您可以开始对话"));
}

void ChatWindow::onSendClicked() {
    if (!client_ || !client_->isConnected()) {
        QMessageBox::information(this, QStringLiteral("未连接"), QStringLiteral("请先连接到服务器。"));
        return;
    }

    const auto text = ui->input->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    appendMessage(QStringLiteral("我"), text);
    ui->input->clear();

    // Demo：发送文本并等待一条回复。
    auto response = client_->requestResponse(text.toStdString());
    if (response) {
        appendMessage(QStringLiteral("服务器"), QString::fromStdString(*response));
    } else {
        appendMessage(QStringLiteral("服务器"), QStringLiteral("未收到回复，可能网络中断。"));
    }
}

