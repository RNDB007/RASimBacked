#pragma once
#include <memory>
#include <QString>
#include <QUuid>
#include "WsfScenario.hpp"
#include "WsfSimulation.hpp"
#include "WsfSimulationInput.hpp"
#include "WsfStandardApplication.hpp"
#include <QSharedPointer>
#include <QHash>
#include <QList>
#include "RASimApplication.h"
#include "RASimFrameStepSimulation.h"
#include <QMutex>

class RASimApp
{
public:
    enum ControlType { E_Start, E_Pause, E_Resume, E_Stop, E_SetSpeed, E_SetFPS };
    enum State { E_Unknown, E_Initialized, E_Running, E_Pausing, E_Stopped };

    explicit RASimApp(const QString& sceneName);
    ~RASimApp();

    bool init();
    bool step();
    void control(ControlType controlType, double value = 0);
    void setSpeed(double speed);
    void setFPS(int fps);

    const QString& getId()      const { return m_id; }
    double getAdvanceTime();
    RASimFrameStepSimulation* getFrameStepSim() const;
    State getState() const { return m_state; }

private:
    QString                             m_id;
    QString                             m_sceneName;
    State                               m_state = E_Unknown;
    double                              m_speed = 1.0;
    int                                 m_fps = 60;
    WsfStandardApplication::Options     m_options;
    std::unique_ptr<WsfScenario>        m_scenario;
    std::unique_ptr<RASimFrameStepSimulation> m_wsfSim;

    static std::unique_ptr<RASimApplication> s_app;
    double m_advanceTime;
    static QMutex s_appMutex;

    SimRunLoopState m_runLoopState;
};

typedef QSharedPointer<RASimApp>     RASimAppPtr;
typedef QHash<QString, RASimAppPtr>  RASimAppMap;
typedef QList<RASimAppPtr>           RASimAppList;
