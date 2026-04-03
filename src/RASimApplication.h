#pragma once
#include "WsfStandardApplication.hpp"
#include "TimedRegion.hpp"
class RASimApp;

struct SimRunLoopState
{
    WsfSimulation* simPtr = nullptr;
    WsfStandardApplication::Options opts;
    RASimApp* host = nullptr;
    double messageInterval = 0.0;
    double previousInterval = 0.0;
    double lastMsgTime = 0.0;
    bool deferred = false;
    double simTime = 0.0;
};

class RASimApplication : public WsfStandardApplication
{
public:
    RASimApplication(const std::string& aApplicationName,
        int               argc,
        char* argv[],
        const PluginPaths& aPluginPaths = PluginPaths());

    void beginRunLoop(SimRunLoopState& state, WsfSimulation* aSimPtr, Options aOptions, RASimApp* raSimApp);
    bool stepRunLoop(SimRunLoopState& state);
    WsfStandardApplication::SimulationResult finishRunLoop(SimRunLoopState& state);
    double getSimTime(const SimRunLoopState& state) const;
};
