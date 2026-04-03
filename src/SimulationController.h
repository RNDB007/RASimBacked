#ifndef SIMULATIONCONTROLLER_H
#define SIMULATIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "RASimServerManager.h"
#include "CommunicationLayer.h"

struct ClientSessionInfo
{
    QMap<QString, QString> envIdToSceneKey;
    QTcpSocket* socket = nullptr;
};

class SimulationController : public QObject
{
    Q_OBJECT
public:
    explicit SimulationController(CommunicationLayer* comm, RASimServerManager* manager, QObject* parent = nullptr);

private slots:
    void handleMessageReceived(const QString& clientId, const QJsonObject& json, QTcpSocket* socket);
    void handleClientDisconnected(const QString& clientId);

private:
    void sendErrorToClient(const QString& clientId, const QString& errorMsg);
    void cleanupClientResources(const QString& clientId);

    CommunicationLayer* m_commLayer;
    RASimServerManager* m_simManager;
    QMap<QString, ClientSessionInfo> m_sessions;
};

#endif // SIMULATIONCONTROLLER_H
