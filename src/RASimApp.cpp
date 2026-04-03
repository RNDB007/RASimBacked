#include "RASimApp.h"
#include <QCoreApplication>
#include <iostream>
#include "UtException.hpp"
#include "UtLog.hpp"
#include "wsf_extensions.hpp"
#include "wsf_version_defines.hpp"

std::unique_ptr<RASimApplication> RASimApp::s_app = nullptr;
QMutex RASimApp::s_appMutex;

RASimApp::RASimApp(const QString& sceneName)
    : m_id(QUuid::createUuid().toString(QUuid::Id128))
    , m_sceneName(sceneName)
{
}

RASimApp::~RASimApp()
{
    if (m_state != E_Stopped)
        control(E_Stop);
}

bool RASimApp::init()
{
    QMutexLocker locker(&s_appMutex);
    if (!s_app)
    {
        ut::SetupApplicationLog("rasimapp", WSF_VERSION, "rasimapp-exception.log");
        auto appName = QCoreApplication::instance()->arguments()[0].toStdString();
        char* argv[1];
        argv[0] = const_cast<char*>(appName.c_str());
        s_app = std::make_unique<RASimApplication>("RASimBackend", 1, argv);

        RegisterBuiltinExtensions(*s_app);
        RegisterOptionalExtensions(*s_app);
        // 注：xio_interface 扩展可能不存在于所有 SDK 版本
        // WSF_REGISTER_EXTENSION(*s_app, xio_interface);
    }

    m_scenario = std::make_unique<WsfScenario>(*s_app);
    m_options.mSimType = WsfStandardApplication::cFRAME_STEPPED;
    m_options.mInputFiles.push_back(m_sceneName.toStdString());

    try {
        s_app->ProcessInputFiles(*m_scenario, m_options.mInputFiles);
    }
    catch (const WsfApplication::Exception& e) {
        ut::log::fatal() << "Cannot load scenario file: " << e.what();
        return false;
    }

    m_wsfSim = std::make_unique<RASimFrameStepSimulation>(*m_scenario, 1);
    m_wsfSim->SetRealtime(0, false);
    m_wsfSim->SetClockRate(m_speed);
    m_wsfSim->SetFrameTime(1.0 / m_fps);
    m_wsfSim->SetAuxData("appId", m_id.toStdString());

    if (!s_app->InitializeSimulation(m_wsfSim.get()))
        return false;

    m_state = E_Initialized;
    return true;
}

bool RASimApp::step()
{
    QMutexLocker locker(&s_appMutex);
    if (m_state == E_Initialized) {
        s_app->beginRunLoop(m_runLoopState, m_wsfSim.get(), m_options, this);
        m_state = E_Running;
    }

    bool stillRunning = s_app->stepRunLoop(m_runLoopState);
    if (!stillRunning) {
        s_app->finishRunLoop(m_runLoopState);
        m_state = E_Stopped;
    }

    return stillRunning;
}

void RASimApp::control(ControlType ct, double value)
{
    switch (ct)
    {
    case E_Start:
        if (m_state == E_Initialized) m_state = E_Running;
        break;
    case E_Pause:
        if (m_state == E_Running) { m_wsfSim->Pause(); m_state = E_Pausing; }
        break;
    case E_Resume:
        if (m_state == E_Pausing) { m_wsfSim->Resume(); m_state = E_Running; }
        break;
    case E_Stop:
        if (m_state == E_Running || m_state == E_Pausing)
        {
            m_wsfSim->SetEndTime(m_wsfSim->GetSimTime());
            m_wsfSim->Complete(m_wsfSim->GetSimTime());
            m_state = E_Stopped;
        }
        break;
    case E_SetSpeed:
        setSpeed(value);
        break;
    case E_SetFPS:
        setFPS(static_cast<int>(value));
        break;
    }
}

void RASimApp::setSpeed(double speed)
{
    if (speed > 0 && std::abs(m_speed - speed) > 1e-6)
    {
        m_speed = speed;
        m_wsfSim->SetClockRate(m_speed);
    }
}

void RASimApp::setFPS(int fps)
{
    if (fps > 0 && m_fps != fps)
    {
        m_fps = fps;
        m_wsfSim->SetFrameTime(1.0 / m_fps);
    }
}

double RASimApp::getAdvanceTime()
{
    m_advanceTime = s_app->getSimTime(m_runLoopState);
    return m_advanceTime;
}

RASimFrameStepSimulation* RASimApp::getFrameStepSim() const
{
    return m_wsfSim.get();
}
