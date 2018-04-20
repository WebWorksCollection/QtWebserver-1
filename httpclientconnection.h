#pragma once

#include <QObject>

#include "httpcontainers.h"

class QTcpSocket;

class HttpClientConnection : public QObject
{
    Q_OBJECT

public:
    explicit HttpClientConnection(QTcpSocket *socket, QObject *parent = nullptr);

public Q_SLOTS:
    void sendResponse(const HttpResponse &response);

Q_SIGNALS:
    void requestReceived(const HttpRequest &request);

private Q_SLOTS:
    void readyRead();

private:
    void clearRequest();
    static QString statusString(HttpResponse::StatusCode statusCode);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    enum { RequestLine, Headers, RequestBody } m_state;
    int m_bodyLength;

    HttpRequest m_request;
};
