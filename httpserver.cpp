#include "httpserver.h"

#include <QTcpServer>

#include "httpclientconnection.h"

HttpServer::HttpServer(QObject *parent) :
    QObject(parent),
    m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::acceptError, this, &HttpServer::acceptError);
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::newConnection);
}

bool HttpServer::listen(const QHostAddress &address, quint16 port)
{
    return m_server->listen(address, port);
}

void HttpServer::close()
{
    m_server->close();
}

bool HttpServer::isListening() const
{
    return m_server->isListening();
}

void HttpServer::setMaxPendingConnections(int numConnections)
{
    m_server->setMaxPendingConnections(numConnections);
}

int HttpServer::maxPendingConnections() const
{
    return m_server->maxPendingConnections();
}

quint16 HttpServer::serverPort() const
{
    return m_server->serverPort();
}

QHostAddress HttpServer::serverAddress() const
{
    return m_server->serverAddress();
}

qintptr HttpServer::socketDescriptor() const
{
    return m_server->socketDescriptor();
}

bool HttpServer::setSocketDescriptor(qintptr socketDescriptor)
{
    return m_server->setSocketDescriptor(socketDescriptor);
}

bool HttpServer::hasPendingConnections() const
{
    return m_server->hasPendingConnections();
}

QAbstractSocket::SocketError HttpServer::serverError() const
{
    return m_server->serverError();
}

QString HttpServer::errorString() const
{
    return m_server->errorString();
}

void HttpServer::pauseAccepting()
{
    m_server->pauseAccepting();
}

void HttpServer::resumeAccepting()
{
    m_server->resumeAccepting();
}

#ifndef QT_NO_NETWORKPROXY
void HttpServer::setProxy(const QNetworkProxy &networkProxy)
{
    m_server->setProxy(networkProxy);
}

QNetworkProxy HttpServer::proxy() const
{
    return m_server->proxy();
}
#endif

void HttpServer::newConnection()
{
    auto connection = m_server->nextPendingConnection();
    if(!connection)
        return;

    auto httpConnection = new HttpClientConnection(connection, this);
    connect(httpConnection, &HttpClientConnection::requestReceived, this, [=](const HttpRequest &request){
        handleRequest(httpConnection, request);
    });
}
