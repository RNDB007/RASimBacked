#pragma once
#include "WsfFrameStepSimulation.hpp"
class RASimFrameStepSimulation : public WsfFrameStepSimulation
{
public:
    RASimFrameStepSimulation(WsfScenario& aScenario, unsigned int aRunNumber);
    void SetFrameTime(double frameTime);
protected:
    virtual double AdvanceFrame() override;
protected:
    double m_preFrameCount = 0;
};
