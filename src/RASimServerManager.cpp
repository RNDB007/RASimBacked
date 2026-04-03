#include "RASimServerManager.h"
#include <QMutexLocker>
#include <iostream>
#include <QDebug>
#include <QJsonObject>
#include <QDir>
#include <QCoreApplication>

RASimServerManager* RASimServerManager::m_instance = nullptr;

RASimServerManager* RASimServerManager::getInstance() {
    static QMutex instMutex;
    if (!m_instance) {
        QMutexLocker lock(&instMutex);
        if (!m_instance) {
            m_instance = new RASimServerManager();
        }
    }
    return m_instance;
}

void RASimServerManager::deleteInstance() {
    static QMutex instMutex;
    QMutexLocker lock(&instMutex);
    delete m_instance;
    m_instance = nullptr;
}

RASimServerManager::RASimServerManager(QObject* parent) : QObject(parent) {}
RASimServerManager::~RASimServerManager() {
    QMutexLocker lock(&m_mutex);
    for (auto srv : m_servers) {
        srv->stopServer();
        srv->deleteLater();
    }
    m_servers.clear();
}

RASimServer* RASimServerManager::createServer(const QString& sceneName, const QString& scriptPath) {
    QMutexLocker lock(&m_mutex);
    if (m_servers.contains(sceneName)) {
        auto oldSrv = m_servers.take(sceneName);
        delete oldSrv;
    }
    auto srv = new RASimServer(scriptPath);
    m_servers.insert(sceneName, srv);
    srv->startServer();
    return srv;
}

RASimServer* RASimServerManager::getServer(const QString& sceneName) {
    QMutexLocker lock(&m_mutex);
    return m_servers.value(sceneName, nullptr);
}

QList<RASimServer*> RASimServerManager::getAllServers() const {
    QMutexLocker lock(&m_mutex);
    return m_servers.values();
}

int RASimServerManager::getServerCount() const {
    QMutexLocker lock(&m_mutex);
    return m_servers.size();
}

void RASimServerManager::nextFrameServer(const QString& sceneName) {
    QMutexLocker lock(&m_mutex);
    auto srv = m_servers.value(sceneName);
    if (srv) srv->nextFrame();
}

void RASimServerManager::nextFramesServer(const QString& sceneName, int count) {
    QMutexLocker lock(&m_mutex);
    auto srv = m_servers.value(sceneName);
    if (srv) srv->nextFrames(count);
}

void RASimServerManager::removeServer(const QString& sceneName) {
    QMutexLocker lock(&m_mutex);
    if (m_servers.contains(sceneName)) {
        auto srv = m_servers.take(sceneName);
        delete srv;
    }
}

void RASimServerManager::pauseServer(const QString& sceneName) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->pause();
}

void RASimServerManager::resumeServer(const QString& sceneName) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->resume();
}

void RASimServerManager::stopSimulation(const QString& sceneName) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->stop();
}

void RASimServerManager::requestStopServer(const QString& sceneName) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->stopServer();
}

void RASimServerManager::setSimulationSpeed(const QString& sceneName, double speed) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->setSpeed(speed);
}

void RASimServerManager::setSimulationFPS(const QString& sceneName, double fps) {
    QMutexLocker locker(&m_mutex);
    RASimServer* server = m_servers.value(sceneName, nullptr);
    if (server) server->setFPS(fps);
}

QJsonObject RASimServerManager::getAllObservations(const QString& sceneName) {
    QMutexLocker lock(&m_mutex);
    auto srv = m_servers.value(sceneName, nullptr);
    if (srv) return srv->getObservation();
    return QJsonObject();
}

void RASimServerManager::destroyServer(const QString& uniqueSceneKey) {
    QMutexLocker lock(&m_mutex);
    if (m_servers.contains(uniqueSceneKey)) {
        RASimServer* srv = m_servers.take(uniqueSceneKey);
        delete srv;

        QString tempPath = QCoreApplication::applicationDirPath() + "/TempSim/" + uniqueSceneKey;
        QDir dir(tempPath);
        if (dir.exists()) {
            dir.removeRecursively();
            qDebug() << "Deleted temp directory:" << tempPath;
        }
    }
}
