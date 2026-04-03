#include "RASimFrameStepSimulation.h"
#include "WsfSimulationInput.hpp"
#include "WsfComm.hpp"

static constexpr double kEps = 1e-9;

RASimFrameStepSimulation::RASimFrameStepSimulation(WsfScenario& aScenario, unsigned int aRunNumber)
    : WsfFrameStepSimulation(aScenario, aRunNumber)
{
}

void RASimFrameStepSimulation::SetFrameTime(double frameTime)
{
    if (frameTime <= 0) return;
    const_cast<WsfFrameStepSimulationInput*>(mFrameStepInput)->mFrameTime = frameTime;
}

double RASimFrameStepSimulation::AdvanceFrame()
{
    auto advanceFrames = [&](double frames) {
        mFrameCount += frames;
        double delta = mFrameCount - m_preFrameCount;
        mNextFrameTime += delta * GetFrameTime();
        m_preFrameCount = mFrameCount;
    };

    double currentFrameTime = mNextFrameTime;
    WsfObserver::FrameStarting(this)(currentFrameTime);
    advanceFrames(1.0);

    unsigned int i;
    if (MultiThreaded())
    {
        GetMultiThreadManager().UpdatePlatforms(currentFrameTime);
    }
    else
    {
        for (i = 0; i < GetPlatformCount(); ++i)
        {
            GetPlatformEntry(i)->Update(currentFrameTime);
        }
        WsfObserver::FramePlatformsUpdated(this)(currentFrameTime);
    }

    for (i = 0; i < mComms.size(); ++i)
    {
        mComms[i]->Update(currentFrameTime);
    }
    for (i = 0; i < mProcessors.size(); ++i)
    {
        mProcessors[i]->Update(currentFrameTime);
    }
    if (MultiThreaded())
    {
        GetMultiThreadManager().UpdateSensors(currentFrameTime);
    }
    else
    {
        for (i = 0; i < mSensors.size(); ++i)
        {
            mSensors[i]->Update(currentFrameTime);
        }
    }
    AdvanceFrameObjects(currentFrameTime);

    WsfEvent* peekEventPtr = mEventManager.PeekEvent();
    while (peekEventPtr && (peekEventPtr->GetTime() < mNextFrameTime))
    {
        auto eventPtr = mEventManager.PopEvent();
        double originalEventTime = eventPtr->GetTime();
        eventPtr->SetTime(currentFrameTime);
        WsfEvent::EventDisposition disposition = eventPtr->Execute();
        if (disposition == WsfEvent::cRESCHEDULE)
        {
            double newEventTime = originalEventTime + (eventPtr->GetTime() - currentFrameTime);
            if (newEventTime < mNextFrameTime) {
                newEventTime = mNextFrameTime + kEps;
            }
            eventPtr->SetTime(newEventTime);
            mEventManager.AddEvent(std::move(eventPtr));
        }
        peekEventPtr = mEventManager.PeekEvent();
    }

    double clockTime = mClockSourcePtr->GetClock(1.0E+37);
    mRealTime = currentFrameTime;
    double timeLeft = 0.0;
    if (mIsRealTime)
    {
        timeLeft = mNextFrameTime - clockTime;
        mRealTime = clockTime;
    }
    if (timeLeft >= 0.0)
    {
        mTotalFrameUnderTime += timeLeft;
        mTotalFrameUnderCount += 1.0;
        mTimeUntilNextFrame = timeLeft;
    }
    else
    {
        timeLeft = -timeLeft;
        mTotalFrameOverTime += timeLeft;
        mTotalFrameOverCount += 1.0;
        mTimeUntilNextFrame = 0.0;
        if (timeLeft > mWorstFrameOverTime)
        {
            mWorstFrameOverTime = timeLeft;
        }
        int skippedFrameCount = 0;
        if ((timeLeft / GetFrameTime()) > 0.10)
        {
            skippedFrameCount = static_cast<int>((timeLeft + GetFrameTime()) / GetFrameTime());
            advanceFrames(skippedFrameCount);
            mSkippedFrameCount += skippedFrameCount;
        }
        double nextUpdateTime = mNextFrameTime;
        for (i = 0; i < mSensors.size(); ++i)
        {
            mSensors[i]->AdjustNextUpdateTime(nextUpdateTime);
        }
    }
    WsfObserver::FrameComplete(this)(currentFrameTime);
    return currentFrameTime;
}
