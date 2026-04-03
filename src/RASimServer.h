#ifndef RASIMSERVER_H
#define RASIMSERVER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "RASimServer_global.h"

class RASimWorker;
class RASimApp;
class WsfPlatform;

class RASimServer : public QObject
{
    Q_OBJECT
public:
    explicit RASimServer(const QString& sceneFile, QObject* parent = nullptr);
    ~RASimServer() override;

    void startServer();
    void nextFrame();
    void nextFrames(int count);
    void pause();
    void resume();
    void stop();
    void stopServer();
    void setSpeed(double v);
    void setFPS(double v);

    int getObsDim();
    int getActionDim();
    QJsonObject getObservation();

signals:
    void simulationStarted();
    void simulationFinished();
    void frameCompleted(int frameCompleted);

    void startRequested();
    void nextFrameRequested();
    void nextFramesRequested(int count);
    void pauseRequested();
    void resumeRequested();
    void stopRequested();
    void speedSet(double v);
    void fpsSet(double v);

    void frameFinished();

private:
    QString m_sceneFile;
    QThread* m_workerThread;
    RASimWorker* m_worker;
};

class RASimWorker : public QObject
{
    Q_OBJECT
public:
    explicit RASimWorker(const QString& sceneFile, QObject* parent = nullptr);
    ~RASimWorker() override;

    int getObsDim() const;
    int getActionDim() const;
    QJsonObject getObservation();

signals:
    void simulationStarted();
    void simulationFinished();
    void frameCompleted(int frameCompleted);
    void frameFinished();

public slots:
    void startSimulation();
    void nextFrame();
    void nextFramesBatch(int count);
    void pauseSimulation();
    void resumeSimulation();
    void stopSimulation();
    void setSpeed(double v);
    void setFPS(double v);

private:
    void processFrameLogic();
    void printPlatformInfo(class WsfPlatform* plm);

    QString m_sceneFile;
    RASimApp* m_simApp = nullptr;

    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_requestStop = false;
    bool m_isPaused = false;
    bool m_isFrameRunning = false;
    int frameCount = 0;
};

#endif // RASIMSERVER_H
