#include "httpclientconnection.h"

#include <QTcpSocket>
#include <QTextStream>
#include <QRegularExpression>

#include "httpserver.h"

HttpClientConnection::HttpClientConnection(QTcpSocket &socket, HttpServer &httpServer) :
    QObject(&httpServer),
    m_socket(socket),
    m_httpServer(httpServer),
    m_state(RequestLine),
    m_bodyLength(-1)
{
    m_socket.setParent(this);

    connect(&m_socket, &QIODevice::readyRead, this, &HttpClientConnection::readyRead);
    connect(&m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
}

void HttpClientConnection::sendResponse(const HttpResponse &response)
{
    if(m_state != WaitingForResponse)
    {
        qCritical() << "sending a response now is not allowed!";
        return;
    }

    QTextStream stream(&m_socket);
    stream << response.protocol << ' ' << int(response.statusCode) << ' ' << statusString(response.statusCode) << endl;

    for(auto iter = response.headers.constBegin(); iter != response.headers.constEnd(); iter++)
        stream << iter.key() << ": " << iter.value() << endl;

    stream << endl;
}

void HttpClientConnection::sendResponse(HttpResponse response, const QByteArray &byteArray)
{
    if(m_state != WaitingForResponse)
    {
        qCritical() << "sending a response now is not allowed!";
        return;
    }

    response.headers.insert(QStringLiteral("Content-Length"), QString::number(byteArray.length()));
    sendResponse(response);
    m_socket.write(byteArray);
    m_state = RequestLine;
}

void HttpClientConnection::sendResponse(HttpResponse response, const QString &string)
{
    if(m_state != WaitingForResponse)
    {
        qCritical() << "sending a response now is not allowed!";
        return;
    }

    sendResponse(response, string.toUtf8());
    m_state = RequestLine;
}

void HttpClientConnection::sendResponse(HttpResponse response, std::unique_ptr<QIODevice> &&device)
{
    if(m_state != WaitingForResponse)
        throw std::runtime_error("sending a response now is not allowed!");

    if(!device->isReadable())
        throw std::runtime_error("device is not readable");

    if(device->isSequential())
        throw std::runtime_error("sequental device not supported yet");

    m_sendingDeivce = std::move(device);

    response.headers.insert(QStringLiteral("Content-Length"), QString::number(m_sendingDeivce->size()));

    sendResponse(response);

    connect(&m_socket, &QIODevice::bytesWritten, this, &HttpClientConnection::bytesWritten);
    bytesWritten();

    m_state = SendingResponse;
}

void HttpClientConnection::readyRead()
{
    m_buffer.append(m_socket.readAll());

    switch(m_state)
    {
    case RequestLine:
    case Headers:
    {
        int index;
        while((index = m_buffer.indexOf(QByteArrayLiteral("\r\n"))) != -1)
        {
            QString line(m_buffer.left(index));
            m_buffer.remove(0, index + 2);

            switch(m_state)
            {
            case RequestLine:
            {
                auto parts = line.split(' ');
                Q_ASSERT(parts.count() == 3);

                m_request.method = parts.at(0);
                m_request.path = parts.at(1);
                m_request.protocol = parts.at(2);

                m_state = Headers;
                continue;
            }
            case Headers:
            {
                if(!line.isEmpty())
                {
                    static const QRegularExpression regex(QStringLiteral("^ *([^ :]+) *: *(.*) *$"));

                    auto match = regex.match(line);
                    if(!match.hasMatch())
                        qWarning() << "ignoring invalid" << line;

                    if(m_request.headers.contains(match.captured(1)))
                        qWarning() << "duplicate header" << match.captured(1);

                    m_request.headers.insert(match.captured(1), match.captured(2));
                }
                else
                {
                    if(m_request.headers.contains(QStringLiteral("Content-Length")))
                    {
                        m_bodyLength = m_request.headers.value(QStringLiteral("Content-Length")).toInt();
                        m_state = RequestBody;
                        goto hatschi;
                    }
                    else
                    {
                        m_state = WaitingForResponse;
                        m_httpServer.handleRequest(this, m_request);
                        clearRequest();
                    }
                }
            }
            }
        }
        return;
    }
    case RequestBody:
    hatschi:
    {
        auto length = qMin(m_bodyLength - m_request.body.count(), m_buffer.count());
        m_request.body.append(m_buffer.left(length));
        m_buffer.remove(0, length);

        if(m_request.body.count() == m_bodyLength)
        {
            if(!m_buffer.isEmpty())
                qCritical() << "received more than expected!";

            m_state = WaitingForResponse;
            m_httpServer.handleRequest(this, m_request);
            clearRequest();
        }
    }
    }
}

void HttpClientConnection::bytesWritten()
{
    if(m_socket.bytesToWrite() >= 1024*1024*4)
        return;

    if(m_socket.bytesToWrite() == 0 && m_sendingDeivce->bytesAvailable() == 0)
    {
        m_state = RequestLine;
        disconnect(&m_socket, &QIODevice::bytesWritten, this, &HttpClientConnection::bytesWritten);
        m_sendingDeivce.reset();
        return;
    }

    auto buffer = m_sendingDeivce->read(1024*1024*4);
    m_socket.write(buffer);
}

void HttpClientConnection::clearRequest()
{
    m_request.headers.clear();
    m_request.body.clear();
}

QString HttpClientConnection::statusString(HttpResponse::StatusCode statusCode)
{
    switch(statusCode) {
    case HttpResponse::StatusCode::Continue: return QStringLiteral("Continue");
    case HttpResponse::StatusCode::SwitchingProtocols: return QStringLiteral("Switching Protocols");
    case HttpResponse::StatusCode::Processing: return QStringLiteral("Processing");
    case HttpResponse::StatusCode::EarlyHints: return QStringLiteral("Early Hints");
    case HttpResponse::StatusCode::OK: return QStringLiteral("OK");
    case HttpResponse::StatusCode::Created: return QStringLiteral("Created");
    case HttpResponse::StatusCode::Accepted: return QStringLiteral("Accepted");
    case HttpResponse::StatusCode::NonAuthoritativeInformation: return QStringLiteral("Non-Authoritative Information");
    case HttpResponse::StatusCode::NoContent: return QStringLiteral("No Content");
    case HttpResponse::StatusCode::ResetContent: return QStringLiteral("Reset Content");
    case HttpResponse::StatusCode::PartialContent: return QStringLiteral("Partial Content");
    case HttpResponse::StatusCode::MultiStatus: return QStringLiteral("Multi-Status");
    case HttpResponse::StatusCode::AlreadyReported: return QStringLiteral("Already Reported");
    case HttpResponse::StatusCode::IMUsed: return QStringLiteral("IM Used");
    case HttpResponse::StatusCode::MultipleChoices: return QStringLiteral("Multiple Choices");
    case HttpResponse::StatusCode::MovedPermanently: return QStringLiteral("Moved Permanently");
    case HttpResponse::StatusCode::Found: return QStringLiteral("Found (Moved Temporarily)");
    case HttpResponse::StatusCode::SeeOther: return QStringLiteral("See Other");
    case HttpResponse::StatusCode::NotModified: return QStringLiteral("Not Modified");
    case HttpResponse::StatusCode::UseProxy: return QStringLiteral("Use Proxy");
    case HttpResponse::StatusCode::SwitchProxy: return QStringLiteral("(reserviert)");
    case HttpResponse::StatusCode::TemporaryRedirect: return QStringLiteral("Temporary Redirect");
    case HttpResponse::StatusCode::PermanentRedirect: return QStringLiteral("Permanent Redirect");
    case HttpResponse::StatusCode::BadRequest: return QStringLiteral("Bad Request");
    case HttpResponse::StatusCode::Unauthorized: return QStringLiteral("Unauthorized");
    case HttpResponse::StatusCode::PaymentRequired: return QStringLiteral("Payment Required");
    case HttpResponse::StatusCode::Forbidden: return QStringLiteral("Forbidden");
    case HttpResponse::StatusCode::NotFound: return QStringLiteral("Not Found");
    case HttpResponse::StatusCode::MethodNotAllowed: return QStringLiteral("Method Not Allowed");
    case HttpResponse::StatusCode::NotAcceptable: return QStringLiteral("Not Acceptable");
    case HttpResponse::StatusCode::ProxyAuthenticationRequired: return QStringLiteral("Proxy Authentication Required");
    case HttpResponse::StatusCode::RequestTimeout: return QStringLiteral("Request Timeout");
    case HttpResponse::StatusCode::Conflict: return QStringLiteral("Conflict");
    case HttpResponse::StatusCode::Gone: return QStringLiteral("Gone");
    case HttpResponse::StatusCode::LengthRequired: return QStringLiteral("Length Required");
    case HttpResponse::StatusCode::PreconditionFailed: return QStringLiteral("Precondition Failed");
    case HttpResponse::StatusCode::RequestEntityTooLarge: return QStringLiteral("Request Entity Too Large");
    case HttpResponse::StatusCode::URITooLong: return QStringLiteral("URI Too Long");
    case HttpResponse::StatusCode::UnsupportedMediaType: return QStringLiteral("Unsupported Media Type");
    case HttpResponse::StatusCode::Requestedrangenotsatisfiable: return QStringLiteral("Requested range not satisfiable");
    case HttpResponse::StatusCode::ExpectationFailed: return QStringLiteral("Expectation Failed");
    case HttpResponse::StatusCode::Imateapot: return QStringLiteral("I’m a teapot");
    case HttpResponse::StatusCode::PolicyNotFulfilled: return QStringLiteral("Policy Not Fulfilled");
    case HttpResponse::StatusCode::MisdirectedRequest: return QStringLiteral("Misdirected Request");
    case HttpResponse::StatusCode::UnprocessableEntity: return QStringLiteral("Unprocessable Entity");
    case HttpResponse::StatusCode::Locked: return QStringLiteral("Locked");
    case HttpResponse::StatusCode::FailedDependency: return QStringLiteral("Failed Dependency");
    case HttpResponse::StatusCode::UnorderedCollection: return QStringLiteral("Unordered Collection");
    case HttpResponse::StatusCode::UpgradeRequired: return QStringLiteral("Upgrade Required");
    case HttpResponse::StatusCode::PreconditionRequired: return QStringLiteral("Precondition Required");
    case HttpResponse::StatusCode::TooManyRequests: return QStringLiteral("Too Many Requests");
    case HttpResponse::StatusCode::RequestHeaderFieldsTooLarge: return QStringLiteral("Request Header Fields Too Large");
    case HttpResponse::StatusCode::NoResponse: return QStringLiteral("No Response");
    case HttpResponse::StatusCode::Therequestshouldberetriedafterdoingtheappropriateaction: return QStringLiteral("The request should be retried after doing the appropriate action");
    case HttpResponse::StatusCode::UnavailableForLegalReasons: return QStringLiteral("Unavailable For Legal Reasons");
    case HttpResponse::StatusCode::ClientClosedRequest: return QStringLiteral("Client Closed Request");
    case HttpResponse::StatusCode::InternalServerError: return QStringLiteral("Internal Server Error");
    case HttpResponse::StatusCode::NotImplemented: return QStringLiteral("Not Implemented");
    case HttpResponse::StatusCode::BadGateway: return QStringLiteral("Bad Gateway");
    case HttpResponse::StatusCode::ServiceUnavailable: return QStringLiteral("Service Unavailable");
    case HttpResponse::StatusCode::GatewayTimeout: return QStringLiteral("Gateway Timeout");
    case HttpResponse::StatusCode::HTTPVersionnotsupported: return QStringLiteral("HTTP Version not supported");
    case HttpResponse::StatusCode::VariantAlsoNegotiates: return QStringLiteral("Variant Also Negotiates");
    case HttpResponse::StatusCode::InsufficientStorage: return QStringLiteral("Insufficient Storage");
    case HttpResponse::StatusCode::LoopDetected: return QStringLiteral("Loop Detected");
    case HttpResponse::StatusCode::BandwidthLimitExceeded: return QStringLiteral("Bandwidth Limit Exceeded");
    case HttpResponse::StatusCode::NotExtended: return QStringLiteral("Not Extended");
    case HttpResponse::StatusCode::NetworkAuthenticationRequired: return QStringLiteral("Network Authentication Required");
    }
}
