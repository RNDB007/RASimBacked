#include "CommunicationLayer.h"

CommunicationLayer::CommunicationLayer(QObject* parent) : QObject(parent)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &CommunicationLayer::onNewConnection);
}

bool CommunicationLayer::startListening(quint16 port)
{
    if (m_server->isListening()) {
        qWarning() << "Already listening on port" << m_server->serverPort() << ", stop first";
        return false;
    }
    bool isSuccess = m_server->listen(QHostAddress::Any, port);
    if (isSuccess) {
        qInfo() << "Start listening on port" << port;
    }
    else {
        qCritical() << "Failed to listen on port" << port << ":" << m_server->errorString();
    }
    return isSuccess;
}

void CommunicationLayer::stopListening()
{
    if (m_server->isListening()) {
        m_server->close();
        qInfo() << "Stop listening";
    }
    QList<ClientSession*> clientList = m_clients.values();
    for (ClientSession* session : clientList) {
        session->deleteLater();
    }
    m_clients.clear();
}

void CommunicationLayer::sendToClient(const QString& clientId, const QJsonObject& json)
{
    ClientSession* targetClient = m_clients.value(clientId);
    if (targetClient) {
        targetClient->sendJson(json);
    }
}

void CommunicationLayer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* newSocket = m_server->nextPendingConnection();
        if (!newSocket) continue;

        qintptr socketDesc = newSocket->socketDescriptor();
        ClientSession* clientSession = new ClientSession(socketDesc, this);
        QString clientId = clientSession->getClientId();

        if (m_clients.contains(clientId)) {
            m_clients.take(clientId)->deleteLater();
        }

        m_clients.insert(clientId, clientSession);

        connect(clientSession, &ClientSession::dataReceived, this, &CommunicationLayer::onClientDataReceived);
        connect(clientSession, &ClientSession::disconnected, this, &CommunicationLayer::onClientDisconnected);

        emit clientConnected(clientId);
        qInfo() << "New client connected, ID:" << clientId << ", total clients:" << m_clients.size();
    }
}

void CommunicationLayer::onClientDataReceived(const QString& id, const QJsonObject& json, QTcpSocket* socket)
{
    emit messageReceived(id, json, socket);
}

void CommunicationLayer::onClientDisconnected(const QString& id)
{
    ClientSession* disconnectedClient = m_clients.take(id);
    if (disconnectedClient) {
        disconnectedClient->deleteLater();
        qInfo() << "Client disconnected, ID:" << id << ", total clients:" << m_clients.size();
        emit clientDisconnected(id);
    }
}
