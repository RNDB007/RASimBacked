#ifndef RASIMSERVERMANAGER_H
#define RASIMSERVERMANAGER_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include "RASimServer.h"

class RASimServerManager : public QObject
{
    Q_OBJECT
public:
    static RASimServerManager* getInstance();
    static void deleteInstance();

    RASimServer* createServer(const QString& sceneName, const QString& scriptPath);
    RASimServer* getServer(const QString& sceneName);
    QList<RASimServer*> getAllServers() const;
    int getServerCount() const;

    void nextFrameServer(const QString& sceneName);
    void nextFramesServer(const QString& sceneName, int count);
    void removeServer(const QString& sceneName);
    void pauseServer(const QString& sceneName);
    void resumeServer(const QString& sceneName);
    void stopSimulation(const QString& sceneName);
    void requestStopServer(const QString& sceneName);
    void setSimulationSpeed(const QString& sceneName, double speed);
    void setSimulationFPS(const QString& sceneName, double fps);
    void destroyServer(const QString& uniqueSceneKey);

    QJsonObject getAllObservations(const QString& sceneName);

private:
    explicit RASimServerManager(QObject* parent = nullptr);
    ~RASimServerManager();
    Q_DISABLE_COPY(RASimServerManager)

    static RASimServerManager* m_instance;
    mutable QMutex m_mutex;
    QHash<QString, RASimServer*> m_servers;

signals:
    void serverSimulationStarted(const QString& sceneName);
    void serverSimulationFrame(const QString& sceneName, int simTime);
    void serverSimulationFinished(const QString& sceneName);
};

#endif // RASIMSERVERMANAGER_H
