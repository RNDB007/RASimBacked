#pragma once
#include "communicationlayer_global.h"
#include "ClientSession.h"
#include <QTcpServer>
#include <QMap>

class CommunicationLayer : public QObject
{
    Q_OBJECT
public:
    explicit CommunicationLayer(QObject* parent = nullptr);
    bool startListening(quint16 port);
    void stopListening();
    void sendToClient(const QString& clientId, const QJsonObject& json);

signals:
    void messageReceived(const QString& clientId, const QJsonObject& json, QTcpSocket* socket);
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);

private slots:
    void onNewConnection();
    void onClientDataReceived(const QString& id, const QJsonObject& json, QTcpSocket* socket);
    void onClientDisconnected(const QString& id);

private:
    QTcpServer* m_server;
    QMap<QString, ClientSession*> m_clients;
};
