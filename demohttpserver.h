#pragma once

#include <QObject>

#include "httpserver.h"

class DemoHttpServer : public HttpServer
{
    Q_OBJECT

public:
    explicit DemoHttpServer(QObject *parent = nullptr);

protected:
    void handleRequest(HttpClientConnection *connection, const HttpRequest &request) Q_DECL_OVERRIDE;
};
