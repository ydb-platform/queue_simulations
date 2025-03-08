#pragma once

#include <algorithm>
#include <deque>
#include <optional>
#include <memory>
#include <random>
#include <set>

#include "engine/easy.h"
#include "engine/easy_drawing.h"
#include "engine/easy_sprite.h"

#include "common.h"

// Implements a simple pipeline: queue -> Executor<Processor> -> Executor<Processor> -> queue -> ...

namespace queue_sim {

using namespace arctic;  // NOLINT

// ----------------------------
// FlushController: events should wait all previous events to finish

class FlushController : public ItemBase {
public:
    FlushController(const char* name)
        : Name(name)
        , WaitingTimeUs(Histogram::HistogramWithUsBuckets())
    {
    }

    void Tick(double) override {
        /* do nothing */
    }

    bool IsReadyToPushEvent() const override {
        return true;
    }

    void PushEvent(Event event) override {
        event.StartStage();
        WaitingEvents.insert(event);
    }

    bool IsReadyToPopEvent() const override {
        if (WaitingEvents.empty()) {
            return false;
        }

        size_t lowestId = WaitingEvents.begin()->GetId();
        return lowestId - 1 == FinishedEventsBarrier;
    }

    Event PopEvent() override {
        if (!IsReadyToPopEvent()) {
            throw std::runtime_error("No events ready");
        }

        auto iter = WaitingEvents.begin();
        auto event = *iter;
        WaitingEvents.erase(iter);

        if (event.GetId() - 1 != FinishedEventsBarrier) {
            throw std::runtime_error("Oops, something went wrong with flush controller");
        }

        WaitingTimeUs.AddDuration((int)(event.GetStageDuration() * 1000000));

        FinishedEventsBarrier = event.GetId();

        return event;
    }

public:
    void Draw(Sprite toSprite) override {
        auto width = toSprite.Width();
        auto height = toSprite.Height();

        auto minDimension = std::min(width, height);
        auto yPos = height / 2 - minDimension / 2;

        Vec2F bottomLeft(0, yPos);
        Vec2F blockSize(minDimension, minDimension);

        DrawBlock(toSprite, bottomLeft, blockSize, 10, YDBColorWorker, 2, Rgba(0, 0, 0));

        char text[128];
        snprintf(text, sizeof(text), "%s: %ld\np90: %d us",
                 Name, WaitingEvents.size(), WaitingTimeUs.GetPercentile(90));
        GetFont().Draw(toSprite, text, 10, yPos + minDimension / 2);
    }

private:
    const char* Name;
    Histogram WaitingTimeUs;

    size_t FinishedEventsBarrier = 0; // all events with Id <= barrier are finished
    std::set<Event> WaitingEvents;
};

// ----------------------------
// ClosedPipeLine

// assumes, that the first stage is the input queue. Finished events are pushed back to the input queue
class ClosedPipeLine {
public:
    ClosedPipeLine(Sprite sprite)
        : _Sprite(sprite)
        , EventDurationsUs(Histogram::HistogramWithUsBuckets())
    {
    }

    void AddQueue(const char* name, size_t initialEvents = 0) {
        Stages.emplace_back(new Queue(name, initialEvents));
    }

    void AddFixedTimeExecutor(const char* name, size_t processorCount, double executionTime) {
        Stages.emplace_back(new Executor<FixedTimeProcessor>(name, processorCount, executionTime));
    }

    void AddPercentileTimeExecutor(const char* name, size_t processorCount, PercentileTimeProcessor::Percentiles percentiles) {
        Stages.emplace_back(new Executor<PercentileTimeProcessor>(name, processorCount, percentiles));
    }

    void AddFlushController(const char* name) {
        Stages.emplace_back(new FlushController(name));
    }

    void Tick(double dt) {
        TotalTimePassed += dt;

        for (auto& stage: Stages) {
            stage->Tick(dt);
        }

        if (Stages.size() <= 2) {
            return;
        }

        for (size_t i = Stages.size() - 1; i >= 1; --i) {
            auto& stage = Stages[i - 1];
            auto& nextStage = Stages[i];

            while (stage->IsReadyToPopEvent() && nextStage->IsReadyToPushEvent()) {
                auto event = stage->PopEvent();
                nextStage->PushEvent(event);
            }
        }

        // we need second pass for the case, when there are "instant" stages,
        // user to register the event and finish it in the same tick
        for (size_t i = Stages.size() - 1; i >= 1; --i) {
            auto& stage = Stages[i - 1];
            auto& nextStage = Stages[i];

            while (stage->IsReadyToPopEvent() && nextStage->IsReadyToPushEvent()) {
                auto event = stage->PopEvent();
                nextStage->PushEvent(event);
            }
        }

        auto& inputQueue = Stages.front();
        auto& lastStage = Stages.back();

        while (lastStage->IsReadyToPopEvent() && inputQueue->IsReadyToPushEvent()) {
            auto event = lastStage->PopEvent();

            ++TotalFinishedEvents;
            EventDurationsUs.AddDuration((int)(event.GetDuration() * 1000000));

            auto newEvent = Event::NewEvent();
            inputQueue->PushEvent(newEvent);
        }

        AvgRPS = (size_t)(TotalFinishedEvents / TotalTimePassed);
    }

public:
    void Draw() {
        auto stageCount = Stages.size();
        auto width = _Sprite.Width();
        auto height = _Sprite.Height();

        const Si32 spacing = 5;
        const Si32 widthWithoutSpacing = width - spacing * 2;
        const Si32 heightWithoutSpacing = height - spacing * 2;
        const Si32 footerHeight = 100;

        size_t space_between_stages = 25;
        Si32 stage_width = ((widthWithoutSpacing - space_between_stages * (stageCount - 1))) / stageCount;
        Si32 stage_height = heightWithoutSpacing - footerHeight;

        for (size_t i = 0; i < stageCount; ++i) {
            auto& stage = Stages[i];
            Si32 x = i * (stage_width + space_between_stages) + spacing;
            Si32 y = spacing + footerHeight;
            Sprite stageSprite;
            stageSprite.Reference(_Sprite, x, y, stage_width, stage_height);
            stage->Draw(stageSprite);

            if (i != 0) {
                Si32 prevX = x - space_between_stages;
                Si32 middleY = y + stage_height / 2;
                Vec2F src(prevX, middleY);
                Vec2F dst(x, middleY);
                DrawArrow(_Sprite, src, dst, 5, 20, 10, Rgba(0, 0, 0));
            }
        }

        char text[512];
        snprintf(text, sizeof(text),
            "TimePassed: %.2f s, Events: %ld, AvgRPS: %ld\np10: %d us, p50: %d us, p90: %d us, p99: %d us, p100: %d us",
            TotalTimePassed,
            TotalFinishedEvents,
            AvgRPS,
            EventDurationsUs.GetPercentile(10),
            EventDurationsUs.GetPercentile(50),
            EventDurationsUs.GetPercentile(90),
            EventDurationsUs.GetPercentile(99),
            EventDurationsUs.GetPercentile(100)
        );
        DrawRectangle(
            _Sprite,
            arctic::Vec2Si32(spacing, spacing + footerHeight * 2.5),
            arctic::Vec2Si32(width - spacing, footerHeight * 2.5 + 80),
            YDBColorDarkViolet);
        GetFont().Draw(_Sprite, text, spacing * 2, footerHeight * 2.5 + spacing + 5);
    }

private:
    std::deque<ItemPtr> Stages;

    size_t TotalFinishedEvents = 0;
    double TotalTimePassed = 0;

    Histogram EventDurationsUs;
    size_t AvgRPS = 0;

private:
    Sprite _Sprite;
};

} // namespace queue_sim
