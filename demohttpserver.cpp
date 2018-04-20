#include "demohttpserver.h"

#include <QStringBuilder>

#include "httpclientconnection.h"

DemoHttpServer::DemoHttpServer(QObject *parent) :
    HttpServer(parent)
{
}

void DemoHttpServer::handleRequest(HttpClientConnection *connection, const HttpRequest &request)
{
    QString output = "<h1>" % tr("Webserver test") % "</h1>";
    output.append("<p>" % tr("This page outputs everything we know from your request.") % "</p>");

    output.append("<table border=\"1\">");
    output.append("<tr><th>" % tr("Method") % ":</th><td>" % request.method.toHtmlEscaped() % "</td></tr>");
    output.append("<tr><th>" % tr("Protocol") % ":</th><td>" % request.protocol.toHtmlEscaped() % "</td></tr>");
    output.append("<tr><th>" % tr("Path") % ":</th><td>" % request.path.toHtmlEscaped() % "</td></tr>");
    output.append("</table>");

    output.append("<h2>" % tr("Headers") % "</h2>");
    output.append("<table border=\"1\">");
    for(auto iter = request.headers.constBegin(); iter != request.headers.constEnd(); iter++)
        output.append("<tr><th>" % iter.key().toHtmlEscaped() % ":</th><td>" % iter.value().toHtmlEscaped() % "</td></tr>");
    output.append("</table>");

    if(!request.body.isEmpty())
    {
        output.append("<h2>" % tr("Request Body") % "</h2>");
        output.append("<pre>" % QString(request.body).toHtmlEscaped() % "</pre>");
    }

    output.append("<h2>" % tr("Forms") % "</h2>");
    output.append("<form method=\"POST\">"
                  "<input type=\"text\" name=\"textinput1\" />"
                  "<input type=\"text\" name=\"textinput2\" />"
                  "<input type=\"file\" name=\"fileinput\" />"
                  "<button type=\"submit\">Submit</button>"
                  "</form>");
    output.append("<form method=\"POST\" enctype=\"multipart/form-data\">"
                  "<input type=\"text\" name=\"textinput1\" />"
                  "<input type=\"text\" name=\"textinput2\" />"
                  "<input type=\"file\" name=\"fileinput\" />"
                  "<button type=\"submit\">Submit</button>"
                  "</form>");

    HttpResponse response;
    response.protocol = request.protocol;
    response.statusCode = HttpResponse::StatusCode::OK;
    response.body = output.toUtf8();
    response.headers.insert(QStringLiteral("Server"), QStringLiteral("Hatschi Server 1.0"));
    response.headers.insert(QStringLiteral("Content-Type"), QStringLiteral("text/html"));

    connection->sendResponse(response);
}
