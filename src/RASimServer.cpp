#include "RASimServer.h"
#include "RASimApp.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include "WsfSensorMode.hpp"
#include "WsfSensorTracker.hpp"
#include "WsfTrackList.hpp"
#include "WsfLocalTrack.hpp"
#include "qjsonobject.h"
#include "qjsonarray.h"

// ========== RASimServer 实现 ==========
RASimServer::RASimServer(const QString& sceneFile, QObject* parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
{
    m_worker = new RASimWorker(sceneFile);
    m_worker->moveToThread(m_workerThread);

    connect(this, &RASimServer::startRequested, m_worker, &RASimWorker::startSimulation);
    connect(this, &RASimServer::nextFrameRequested, m_worker, &RASimWorker::nextFrame);
    connect(this, &RASimServer::nextFramesRequested, m_worker, &RASimWorker::nextFramesBatch);
    connect(this, &RASimServer::pauseRequested, m_worker, &RASimWorker::pauseSimulation);
    connect(this, &RASimServer::resumeRequested, m_worker, &RASimWorker::resumeSimulation);
    connect(this, &RASimServer::stopRequested, m_worker, &RASimWorker::stopSimulation);
    connect(this, &RASimServer::speedSet, m_worker, &RASimWorker::setSpeed);
    connect(this, &RASimServer::fpsSet, m_worker, &RASimWorker::setFPS);

    connect(m_worker, &RASimWorker::simulationStarted, this, &RASimServer::simulationStarted);
    connect(m_worker, &RASimWorker::simulationFinished, this, &RASimServer::simulationFinished);
    connect(m_worker, &RASimWorker::frameCompleted, this, &RASimServer::frameCompleted);
    connect(m_worker, &RASimWorker::frameFinished, this, &RASimServer::frameFinished);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread->setPriority(QThread::HighPriority);
    m_workerThread->start();
}

RASimServer::~RASimServer()
{
    stopServer();
    m_workerThread->quit();
    if (!m_workerThread->wait(5000))
    {
        m_workerThread->terminate();
        m_workerThread->wait();
    }
}

void RASimServer::startServer() { emit startRequested(); }
void RASimServer::nextFrame() { emit nextFrameRequested(); }
void RASimServer::nextFrames(int count) { emit nextFramesRequested(count); }
void RASimServer::pause() { emit pauseRequested(); }
void RASimServer::resume() { emit resumeRequested(); }
void RASimServer::stop() { emit stopRequested(); }
void RASimServer::stopServer() { emit stopRequested(); }
void RASimServer::setSpeed(double v) { emit speedSet(v); }
void RASimServer::setFPS(double v) { emit fpsSet(v); }

int RASimServer::getObsDim()
{
    // 基础版本：返回平台数 * 观测维度
    // 此方法从 Worker 线程获取数据
    if (m_worker) {
        return m_worker->getObsDim();
    }
    return 0;
}

int RASimServer::getActionDim()
{
    if (m_worker) {
        return m_worker->getActionDim();
    }
    return 0;
}

QJsonObject RASimServer::getObservation()
{
    if (m_worker) {
        return m_worker->getObservation();
    }
    return QJsonObject();
}

// ========== RASimWorker 实现 ==========
RASimWorker::RASimWorker(const QString& sceneFile, QObject* parent)
    : QObject(parent), m_sceneFile(sceneFile)
{
}

RASimWorker::~RASimWorker()
{
    stopSimulation();
}

int RASimWorker::getObsDim() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    if (m_simApp && m_simApp->getFrameStepSim()) {
        int platformCount = m_simApp->getFrameStepSim()->GetPlatformCount();
        if (platformCount > 0) {
            return platformCount * 15;
        }
    }
    return 0;
}

int RASimWorker::getActionDim() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    if (m_simApp && m_simApp->getFrameStepSim()) {
        int platformCount = m_simApp->getFrameStepSim()->GetPlatformCount();
        if (platformCount == 0) return 0;
        return 4; // 基础版本：只有飞控维度
    }
    return 0;
}

void RASimWorker::startSimulation()
{
    m_simApp = new RASimApp(m_sceneFile);
    if (!m_simApp) {
        std::cerr << "RASimServer: 无法创建 RASimApp 实例\n";
        emit simulationFinished();
        return;
    }

    if (!m_simApp->init()) {
        std::cerr << "RASimServer: 仿真应用初始化失败\n";
        delete m_simApp;
        m_simApp = nullptr;
        emit simulationFinished();
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_requestStop = false;
    m_isPaused = false;
    m_isFrameRunning = false;
    frameCount = 0;
    locker.unlock();

    emit simulationStarted();
}

void RASimWorker::nextFrame()
{
    QMutexLocker locker(&m_mutex);
    if (m_requestStop || m_isPaused || m_isFrameRunning || !m_simApp) {
        return;
    }

    m_isFrameRunning = true;
    locker.unlock();

    bool stepSuccess = m_simApp->step();
    if (!stepSuccess) {
        stopSimulation();
        locker.relock();
        m_isFrameRunning = false;
        emit simulationFinished();
        return;
    }

    frameCount++;
    processFrameLogic();

    locker.relock();
    m_isFrameRunning = false;
    locker.unlock();

    emit frameFinished();
    emit frameCompleted(frameCount);
}

void RASimWorker::nextFramesBatch(int count)
{
    QMutexLocker locker(&m_mutex);
    if (m_requestStop || m_isPaused || m_isFrameRunning || !m_simApp) {
        return;
    }
    m_isFrameRunning = true;
    locker.unlock();

    for (int i = 0; i < count; ++i)
    {
        if (!m_simApp->step()) {
            stopSimulation();
            break;
        }
        frameCount++;
    }

    locker.relock();
    m_isFrameRunning = false;
    locker.unlock();

    emit frameFinished();
}

void RASimWorker::pauseSimulation()
{
    QMutexLocker locker(&m_mutex);
    m_isPaused = true;
}

void RASimWorker::resumeSimulation()
{
    QMutexLocker locker(&m_mutex);
    m_isPaused = false;
    m_cond.wakeOne();
}

void RASimWorker::stopSimulation()
{
    QMutexLocker locker(&m_mutex);
    if (m_requestStop) return;

    m_requestStop = true;
    m_isPaused = false;

    if (m_simApp) {
        m_simApp->control(RASimApp::E_Stop);
        delete m_simApp;
        m_simApp = nullptr;
    }

    emit simulationFinished();
}

void RASimWorker::setSpeed(double v)
{
    QMutexLocker locker(&m_mutex);
    if (m_simApp) {
        m_simApp->setSpeed(v);
    }
}

void RASimWorker::setFPS(double v)
{
    QMutexLocker locker(&m_mutex);
    if (m_simApp) {
        m_simApp->setFPS(static_cast<int>(v));
    }
}

void RASimWorker::processFrameLogic()
{
    if (!m_simApp || !m_simApp->getFrameStepSim()) {
        return;
    }
    // 基础版本：帧处理逻辑为空
    // 可在此处添加数据收集、传感器处理等
}

QJsonObject RASimWorker::getObservation()
{
    QJsonObject obsJson;
    QMutexLocker locker(&m_mutex);

    if (!m_simApp || !m_simApp->getFrameStepSim()) {
        return obsJson;
    }

    obsJson["sim_time"] = m_simApp->getAdvanceTime();

    QJsonArray platformsArray;
    int count = m_simApp->getFrameStepSim()->GetPlatformCount();

    for (int i = 0; i < count; ++i) {
        auto plt = m_simApp->getFrameStepSim()->GetPlatformEntry(i);
        if (!plt) continue;

        QJsonObject pltJson;

        QString pName = QString::fromStdString(plt->GetName());
        QString side = QString::fromStdString(plt->GetSide());
        if (!side.isEmpty()) side[0] = side[0].toUpper();

        pltJson["name"] = pName;
        pltJson["side"] = side;
        pltJson["type"] = "platform";

        double pos[3] = { 0 };
        plt->GetLocationWCS(pos);
        WsfGeoPoint geoPoint(pos);

        pltJson["lat"] = geoPoint.GetLat();
        pltJson["lon"] = geoPoint.GetLon();
        pltJson["alt"] = geoPoint.GetAlt();

        double psi = 0.0, theta = 0.0, phi = 0.0;
        plt->GetOrientationNED(psi, theta, phi);

        pltJson["heading"] = psi * 57.29578;
        pltJson["pitch"] = theta * 57.29578;
        pltJson["roll"] = phi * 57.29578;

        pltJson["speed"] = plt->GetSpeed();

        double velNED[3] = {0};
        plt->GetVelocityNED(velNED);
        pltJson["vx"] = velNED[0];
        pltJson["vy"] = velNED[1];
        pltJson["vz"] = velNED[2];

        pltJson["mass"] = plt->GetMass();

        platformsArray.append(pltJson);
    }
    obsJson["platforms"] = platformsArray;

    return obsJson;
}

void RASimWorker::printPlatformInfo(WsfPlatform* plm)
{
    if (!plm) return;
    // 基础版本：不打印详细信息
}
