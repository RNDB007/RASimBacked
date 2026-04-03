#include "RASimApplication.h"
#include "WsfSystemLog.hpp"
#include "UtLog.hpp"
#include "UtStringUtil.hpp"
#include "wsf_extensions.hpp"
#include "wsf_version_defines.hpp"
#include "TimedRegion.hpp"
#include "WsfSystemLog.hpp"
#include "WsfSimulation.hpp"
#include "RASimApp.h"

using namespace std;

RASimApplication::RASimApplication(const std::string& aApplicationName, int argc, char* argv[], const PluginPaths& aPluginPaths)
    : WsfStandardApplication(aApplicationName, argc, argv, aPluginPaths)
{
}

void RASimApplication::beginRunLoop(SimRunLoopState& state, WsfSimulation* aSimPtr, Options aOptions, RASimApp* raSimApp)
{
    UpdateOptionsP(aOptions, aSimPtr);
    state.simPtr = aSimPtr;
    state.opts = std::move(aOptions);
    state.host = raSimApp;
    state.messageInterval = state.opts.mMessageInterval;
    state.previousInterval = state.messageInterval;
    state.lastMsgTime = 0.0;
    state.deferred = state.opts.mDeferredConnectionTime > 0.0;

    profiling::TimedRegion regionSimulation("RunEventLoop", profiling::TimedRegion::Mode::SUBREGION);

    if (state.simPtr->GetState() == WsfSimulation::cPENDING_START)
    {
        regionSimulation.StartSubregion("Starting simulation");
        std::ostringstream oss;
        oss << "start " << state.simPtr->GetRunNumber();
        std::cout << "start " << state.simPtr->GetRunNumber() << std::endl;
        GetSystemLog().WriteLogEntry(oss.str());
        state.simPtr->Start();
    }
    state.simTime = 0.0;
}

bool RASimApplication::stepRunLoop(SimRunLoopState& state)
{
    if (!state.simPtr || !state.simPtr->IsActive())
        return false;

    mLogServer.ProcessMessages();

    if (state.simPtr->IsRealTime())
    {
        if (!state.deferred) {
            state.messageInterval = state.opts.mRealtimeMessageInterval;
        }
        else if (state.simTime >= state.opts.mDeferredConnectionTime) {
            state.messageInterval = state.opts.mRealtimeMessageInterval;
            state.deferred = false;
        }
        else {
            state.messageInterval = state.opts.mMessageInterval;
        }
    }
    else
    {
        state.messageInterval = state.opts.mMessageInterval;
    }
    if (state.messageInterval != state.previousInterval)
    {
        state.previousInterval = state.messageInterval;
        state.lastMsgTime = state.simTime;
        ut::log::info() << "T = " << state.simTime;
    }

    state.simPtr->WaitForAdvanceTime();
    state.simTime = state.simPtr->AdvanceTime();
    ut::log::info() << "T = " << state.simTime;

    return true;
}

WsfStandardApplication::SimulationResult RASimApplication::finishRunLoop(SimRunLoopState& state)
{
    state.simPtr->Complete(state.simTime);

    std::string reason = state.simPtr->GetCompletionReasonString();
    if (state.simPtr->GetCompletionReason() == WsfSimulation::cEND_TIME_REACHED)
        reason = "complete";
    WsfStandardApplication::SimulationResult result;
    result.mResetRequested = (state.simPtr->GetCompletionReason() == WsfSimulation::cRESET);
    return result;
}

double RASimApplication::getSimTime(const SimRunLoopState& state) const
{
    return state.simTime;
}
