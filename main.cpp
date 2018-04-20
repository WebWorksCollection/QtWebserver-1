#include <QCoreApplication>

#include "demohttpserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DemoHttpServer server(&app);
    if(!server.listen(QHostAddress::Any, 8080))
    {
        qCritical() << "webserver could not start listening:" << server.errorString();
        return -2;
    }

    return app.exec();
}
