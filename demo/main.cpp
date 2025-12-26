#include "ChatWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ChatWindow window;
    window.show();
    return app.exec();
}

