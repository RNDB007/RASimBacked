#include "SimulationController.h"
#include "ScenarioConverter.h"
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <iostream>

SimulationController::SimulationController(CommunicationLayer* comm, RASimServerManager* manager, QObject* parent)
    : QObject(parent), m_commLayer(comm), m_simManager(manager)
{
    connect(m_commLayer, &CommunicationLayer::messageReceived,
            this, &SimulationController::handleMessageReceived);
    connect(m_commLayer, &CommunicationLayer::clientDisconnected,
            this, &SimulationController::handleClientDisconnected);
}

void SimulationController::handleMessageReceived(const QString& clientId, const QJsonObject& json, QTcpSocket* socket)
{
    QString reqId = json.value("req_id").toString();
    QString command = json.value("cmd").toString();
    QJsonObject params = json.value("params").toObject();

    QJsonObject response;
    QJsonObject responseData;
    QString responseMsg;

    // --- 1. init ---
    if (command == "init")
    {
        if (m_sessions.contains(clientId)) {
            cleanupClientResources(clientId);
        }

        QString scenarioFile = params.value("scenario").toString();
        if (scenarioFile.isEmpty()) {
            sendErrorToClient(clientId, "Missing parameter: scenario");
            return;
        }

        QString appPath = QCoreApplication::applicationDirPath();
        QString scriptPath = QDir::cleanPath(appPath + "/" + scenarioFile);

        if (!QFile::exists(scriptPath)) {
            sendErrorToClient(clientId, "Scenario file not found: " + scriptPath);
            return;
        }

        QString uniqueKey = clientId + "_" + scenarioFile;

        ClientSessionInfo newSession;
        newSession.socket = socket;
        newSession.envIdToSceneKey.insert("0", uniqueKey);

        RASimServer* newServer = m_simManager->createServer(uniqueKey, scriptPath);
        if (!newServer) {
            sendErrorToClient(clientId, "Failed to create simulation server");
            return;
        }

        // 等待仿真初始化完成
        QEventLoop initLoop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        connect(newServer, &RASimServer::simulationStarted, &initLoop, &QEventLoop::quit, Qt::UniqueConnection);
        connect(&timeoutTimer, &QTimer::timeout, &initLoop, &QEventLoop::quit);
        timeoutTimer.start(10000);
        initLoop.exec();

        m_sessions.insert(clientId, newSession);

        responseData["env_ids"] = QJsonArray({0});
        responseMsg = "Init Success";
    }
    // --- 1b. init_scenario ---
    else if (command == "init_scenario")
    {
        if (m_sessions.contains(clientId)) {
            cleanupClientResources(clientId);
        }

        QJsonObject scenarioJson = params.value("scenario").toObject();
        if (scenarioJson.isEmpty()) {
            sendErrorToClient(clientId, "Missing parameter: scenario (JSON object)");
            return;
        }

        // 校验 JSON 结构
        QString validateError;
        if (!ScenarioConverter::validateScenario(scenarioJson, validateError)) {
            sendErrorToClient(clientId, "Invalid scenario: " + validateError);
            return;
        }

        // 转换为 AFSIM txt
        QString afsimContent = ScenarioConverter::convertToAfsimTxt(scenarioJson);
        if (afsimContent.isEmpty()) {
            sendErrorToClient(clientId, "Failed to convert scenario to AFSIM format");
            return;
        }

        // 写入临时文件
        QString uniqueKey = clientId + "_scenario_" +
                            QString::number(QDateTime::currentMSecsSinceEpoch());
        QString tempDir = QCoreApplication::applicationDirPath() + "/TempSim/" + uniqueKey;
        QDir().mkpath(tempDir);

        QString scriptPath = tempDir + "/scenario.txt";
        QFile file(scriptPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            sendErrorToClient(clientId, "Failed to create temp scenario file: " + scriptPath);
            return;
        }
        QTextStream stream(&file);
        stream << afsimContent;
        file.close();

        qInfo() << "Generated AFSIM scenario:" << scriptPath;

        // 创建仿真服务器
        ClientSessionInfo newSession;
        newSession.socket = socket;
        newSession.envIdToSceneKey.insert("0", uniqueKey);

        RASimServer* newServer = m_simManager->createServer(uniqueKey, scriptPath);
        if (!newServer) {
            sendErrorToClient(clientId, "Failed to create simulation server");
            return;
        }

        // 等待仿真初始化完成
        QEventLoop initLoop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        connect(newServer, &RASimServer::simulationStarted,
                &initLoop, &QEventLoop::quit, Qt::UniqueConnection);
        connect(&timeoutTimer, &QTimer::timeout, &initLoop, &QEventLoop::quit);
        timeoutTimer.start(10000);
        initLoop.exec();

        m_sessions.insert(clientId, newSession);

        responseData["env_ids"] = QJsonArray({0});
        responseMsg = "Init Scenario Success";
    }
    // --- 2. step ---
    else if (command == "step")
    {
        if (!m_sessions.contains(clientId)) {
            sendErrorToClient(clientId, "Please init first");
            return;
        }

        ClientSessionInfo& session = m_sessions[clientId];
        int stepsToRun = params.value("steps").toInt(1);
        if (stepsToRun < 1) stepsToRun = 1;

        QString sceneKey = session.envIdToSceneKey.value("0");
        RASimServer* server = m_simManager->getServer(sceneKey);
        if (!server) {
            sendErrorToClient(clientId, "Server not found");
            return;
        }

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        auto conn = connect(server, &RASimServer::frameFinished, &loop, &QEventLoop::quit, Qt::UniqueConnection);
        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeoutTimer.start(qMax(2000, stepsToRun * 500));

        m_simManager->nextFramesServer(sceneKey, stepsToRun);
        loop.exec();
        disconnect(conn);

        QJsonObject obsData = m_simManager->getAllObservations(sceneKey);
        QJsonObject envResult;
        envResult["obs"] = obsData;
        envResult["done"] = false;
        responseData.insert("0", envResult);

        responseMsg = "Step Completed";
    }
    // --- 3. pause ---
    else if (command == "pause")
    {
        if (!m_sessions.contains(clientId)) {
            sendErrorToClient(clientId, "Session not found");
            return;
        }
        ClientSessionInfo& session = m_sessions[clientId];
        bool needPause = params.value("state").toBool(true);
        QString sceneKey = session.envIdToSceneKey.value("0");

        if (needPause) {
            m_simManager->pauseServer(sceneKey);
            responseMsg = "Simulation Paused";
        } else {
            m_simManager->resumeServer(sceneKey);
            responseMsg = "Simulation Resumed";
        }
    }
    // --- 4. stop ---
    else if (command == "stop")
    {
        if (m_sessions.contains(clientId)) {
            ClientSessionInfo& session = m_sessions[clientId];
            QString sceneKey = session.envIdToSceneKey.value("0");
            m_simManager->stopSimulation(sceneKey);
            responseMsg = "Simulation Stopped";
        }
    }
    // --- 5. close ---
    else if (command == "close")
    {
        cleanupClientResources(clientId);
        responseData["status"] = "ok";
        responseMsg = "Environment Closed";
    }
    // --- 6. set_speed ---
    else if (command == "set_speed")
    {
        if (!m_sessions.contains(clientId)) {
            sendErrorToClient(clientId, "Session not found");
            return;
        }
        double speed = params.value("speed").toDouble(1.0);
        QString sceneKey = m_sessions[clientId].envIdToSceneKey.value("0");
        m_simManager->setSimulationSpeed(sceneKey, speed);
        responseMsg = QString("Speed set to %1").arg(speed);
    }
    // --- 7. set_fps ---
    else if (command == "set_fps")
    {
        if (!m_sessions.contains(clientId)) {
            sendErrorToClient(clientId, "Session not found");
            return;
        }
        double fps = params.value("fps").toDouble(60.0);
        QString sceneKey = m_sessions[clientId].envIdToSceneKey.value("0");
        m_simManager->setSimulationFPS(sceneKey, fps);
        responseMsg = QString("FPS set to %1").arg(fps);
    }
    // --- unknown ---
    else
    {
        sendErrorToClient(clientId, "Unknown command: " + command);
        return;
    }

    response["status"] = "ok";
    response["req_id"] = reqId;
    response["msg"] = responseMsg;
    response["data"] = responseData;

    m_commLayer->sendToClient(clientId, response);
}

void SimulationController::handleClientDisconnected(const QString& clientId)
{
    qWarning() << "Client disconnected, cleaning up:" << clientId;
    cleanupClientResources(clientId);
}

void SimulationController::sendErrorToClient(const QString& clientId, const QString& errorMsg)
{
    QJsonObject errorJson;
    errorJson["status"] = "error";
    errorJson["message"] = errorMsg;
    m_commLayer->sendToClient(clientId, errorJson);
}

void SimulationController::cleanupClientResources(const QString& clientId)
{
    if (!m_sessions.contains(clientId)) return;

    ClientSessionInfo session = m_sessions.take(clientId);
    auto it = session.envIdToSceneKey.constBegin();
    while (it != session.envIdToSceneKey.constEnd()) {
        m_simManager->destroyServer(it.value());
        ++it;
    }
    qInfo() << "Cleaned up client resources:" << clientId;
}
