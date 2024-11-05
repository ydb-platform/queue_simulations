#pragma once

#include <string>
#include <vector>

#include "engine/easy.h"
#include "engine/easy_drawing.h"
#include "engine/easy_sprite.h"

namespace queue_sim {

constexpr double Usec = 0.000001;
constexpr double Msec = 0.001;

// ----------------------------
// our global time

double Now();
void AdvanceTime(double dt);

// ----------------------------
// helpers

arctic::Font& GetFont();

std::string NumToStrWithSuffix(size_t num);

// ----------------------------
// Histogram

class Histogram {
private:
    std::vector<int> Buckets;
    std::vector<int> Counts;

public:
    Histogram(const std::vector<int>& bucketThresholds)
        : Buckets(bucketThresholds)
        , Counts(bucketThresholds.size() + 1, 0)
    {
        for (size_t i = 1; i < Buckets.size(); ++i) {
            if (Buckets[i] < Buckets[i - 1]) {
                throw std::runtime_error("Buckets must be sorted.");
            }
        }
    }

    static Histogram HistogramWithUsBuckets();

    void AddDuration(int duration) {
        for (size_t i = 0; i < Buckets.size(); ++i) {
            if (duration < Buckets[i]) {
                ++Counts[i];
                return;
            }
        }
        ++Counts.back();
    }

    int GetPercentile(int percentile) {
        if (percentile < 0 || percentile > 100) {
            throw std::runtime_error("Percentile must be between 0 and 100.");
        }

        int totalCounts = 0;
        for (int count : Counts) {
            totalCounts += count;
        }

        int threshold = (int)((percentile / 100.0) * totalCounts);
        int cumulativeCount = 0;

        for (size_t i = 0; i < Counts.size(); ++i) {
            cumulativeCount += Counts[i];
            if (cumulativeCount >= threshold) {
                return Buckets[i];
            }
        }

        throw std::runtime_error("Something went wrong while calculating the percentile.");
    }
};

// ----------------------------
// Event

struct Event {
private:
    Event()
        : Id(++EventCounter)
        , StartTime(Now())
    {
    }

    Event(size_t src, size_t dst)
        : Event()
    {
        SrcId = src;
        DstId = dst;
    }

public:
    Event(const Event& other) = default;

    static Event NewEvent();
    static Event NewEvent(size_t src, size_t dst);

    bool operator<(const Event& other) const {
        return Id < other.Id;
    }

    double GetDuration() const {
        return Now() - StartTime;
    }

    double GetStageDuration() const {
        return Now() - StageStarted;
    }

    void StartStage() {
        StageStarted = Now();
    }

    size_t GetId() const {
        return Id;
    }

    size_t GetSrc() const {
        return SrcId;
    }

    size_t GetDst() const {
        return DstId;
    }

private:
    size_t Id;

    // optionally used and set
    size_t SrcId = 0;
    size_t DstId = 0;

    double StartTime = 0;
    double StageStarted = 0;

    static size_t EventCounter;
};

// ----------------------------
// ItemBase: any kind of item, where we can push or pop items
// e.g. queue and executor

class ItemBase {
public:
    ItemBase()
        : ItemId(++ItemCounter)
    {
    }

    size_t GetId() const {
        return ItemId;
    }

    virtual ~ItemBase() = default;

    virtual void Tick(double dt) = 0;

    virtual bool IsReadyToPushEvent() const = 0;
    virtual void PushEvent(Event event) = 0;

    virtual bool IsReadyToPopEvent() const = 0;
    virtual Event PopEvent() = 0;

public:
    virtual void Draw(arctic::Sprite toSprite) = 0;

private:
    size_t ItemId;

    static size_t ItemCounter;
};

using ItemPtr = std::unique_ptr<ItemBase>;

// ----------------------------
// Queue

class Queue : public ItemBase {
public:
    Queue(const char* name, size_t initialEvents = 0)
        : Name(name)
        , QueueTimeUs(Histogram::HistogramWithUsBuckets())
    {
        for (size_t i = 0; i < initialEvents; ++i) {
            PushEvent(Event::NewEvent());
        }
    }

    void Tick(double dt) override {
        /* do nothing */
    }

    bool IsReadyToPushEvent() const override {
        // the queue is infinite
        return true;
    }

    void PushEvent(Event event) override {
        event.StartStage();
        Events.push_back(event);
    }

    bool IsReadyToPopEvent() const override {
        return !Events.empty();
    }

    Event PopEvent() override {
        Event event = Events.front();
        QueueTimeUs.AddDuration((int)(event.GetStageDuration() * 1000000));

        Events.pop_front();
        return event;
    }

public:
    void Draw(arctic::Sprite toSprite) override {
        auto width = toSprite.Width();
        auto height = toSprite.Height();

        auto rWidth = width;
        auto rHeight = width / 2;
        auto yPos = height / 2 - rHeight / 2;

        // draw rectangle in the middle of the sprite
        arctic::Vec2Si32 bottomLeft(0, yPos);
        arctic::Vec2Si32 topRight(rWidth, yPos + rHeight);

        DrawRectangle(toSprite, bottomLeft, topRight, arctic::Rgba(255, 255, 255, 255));

        // draw queue length in the middle

        char text[128];
        auto queueLengthS = NumToStrWithSuffix(Events.size());

        snprintf(text, sizeof(text), "%s: %s\np90: %d us",
                 Name, queueLengthS.c_str(), QueueTimeUs.GetPercentile(90));
        GetFont().Draw(toSprite, text, 10, yPos + rHeight / 2 - 20);
    }

private:
    const char* Name;
    std::deque<Event> Events;
    Histogram QueueTimeUs;
};

// ----------------------------
// ProcessorBase

class ProcessorBase {
public:
    virtual void Tick(double dt) = 0;

    virtual void StartWork(Event event) {
        _Event = event;
        _IsWorking = true;
        StartTime = Now();
    }

    bool IsBusy() const {
        return _IsWorking || _IsEventReady;
    }

    bool IsWorking() const
    {
        return _IsWorking;
    }

    bool IsEventReady() const
    {
        return _IsEventReady;
    }

    void Reset() {
        _IsWorking = false;
        _IsEventReady = false;
        StartTime = 0;
        FinishTime = 0;
        _Event.reset();
    }

    Event PopEvent() {
        auto event = *_Event;
        Reset();
        return event;
    }

protected:
    bool _IsWorking = false; // might be false, but with event, when ready to pop
    bool _IsEventReady = false;

    double StartTime = 0;
    double FinishTime = 0;

    std::optional<Event> _Event;
};

// ----------------------------
// FixedTimeProcessor

class FixedTimeProcessor : public ProcessorBase {
public:
    FixedTimeProcessor(double executionTime)
        : ExecutionTime(executionTime)
    {
    }

    void Tick(double) override {
        if (_IsWorking) {
            auto now = Now();
            if (now - StartTime >= ExecutionTime) {
                _IsWorking = false;
                _IsEventReady = true;
                FinishTime = now;
            }
        }
    }
private:
    double ExecutionTime;
};

// ----------------------------
// PercentileTimeProcessor

class PercentileTimeProcessor : public ProcessorBase {
public:
    struct Percentile {
        double Percentile = 0;
        double Value = 0;
    };

    using Percentiles = std::vector<Percentile>;

    PercentileTimeProcessor(Percentiles percentiles)
        : _Percentiles(std::move(percentiles))
        , Rd(std::make_unique<std::random_device>())
        , Gen(std::make_unique<std::mt19937>((*Rd)()))
        , Dis(std::make_unique<std::uniform_real_distribution<>>(0, 100))
    {
        if (_Percentiles.empty()) {
            throw std::runtime_error("Percentiles must not be empty");
        }
    }

    PercentileTimeProcessor(const PercentileTimeProcessor& other) = delete;
    PercentileTimeProcessor(PercentileTimeProcessor&& other) = default;

    void StartWork(Event event) override {
        ProcessorBase::StartWork(event);

        double r = (*Dis)(*Gen);
        for (auto& percentile: _Percentiles) {
            if (r < percentile.Percentile) {
                ExecutionTime = percentile.Value;
                return;
            }
        }

        ExecutionTime = _Percentiles.back().Value;
    }

    void Tick(double) override {
        if (_IsWorking) {
            auto now = Now();
            if (now - StartTime >= ExecutionTime) {
                _IsWorking = false;
                _IsEventReady = true;
                FinishTime = now;
            }
        }
    }

private:
    Percentiles _Percentiles;
    std::unique_ptr<std::random_device> Rd;
    std::unique_ptr<std::mt19937> Gen;
    std::unique_ptr<std::uniform_real_distribution<>> Dis;

    double ExecutionTime;
};

// ----------------------------
// Executor: wraps any processor into Item

template <typename ProcessorType>
class Executor : public ItemBase {
public:

    template<typename... Args>
    Executor(const char* name, size_t processorCount, Args&&... args)
        : Name(name)
        , BusyProcessorCount(0)
    {
        for (size_t i = 0; i < processorCount; ++i) {
            Processors.emplace_back(std::forward<Args>(args)...);
        }
    }

    void Tick(double dt) override {
        BusyProcessorCount = 0;
        ReadyEventsCount = 0;

        for (auto& processor: Processors) {
            processor.Tick(dt);
            if (processor.IsBusy()) {
                ++BusyProcessorCount;
            }
            if (processor.IsEventReady()) {
                ++ReadyEventsCount;
            }
        }
    }

    bool IsReadyToPushEvent() const override {
        return BusyProcessorCount < Processors.size();
    }

    void PushEvent(Event event) override {
        if (!IsReadyToPushEvent()) {
            throw std::runtime_error("Executor is full");
        }

        event.StartStage();

        for (auto& processor: Processors) {
            if (!processor.IsBusy()) {
                processor.StartWork(event);
                if (processor.IsBusy()) {
                    ++BusyProcessorCount;
                }
                return;
            }
        }
    }

    bool IsReadyToPopEvent() const override {
        return ReadyEventsCount > 0;
    }

    Event PopEvent() override {
        if (!IsReadyToPopEvent()) {
            throw std::runtime_error("No events ready");
        }

        for (auto& processor: Processors) {
            if (processor.IsEventReady()) {
                --ReadyEventsCount;
                --BusyProcessorCount;
                return processor.PopEvent();
            }
        }

        throw std::runtime_error("No events ready");
    }

    size_t GetProcessorCount() const {
        return Processors.size();
    }

    size_t GetBusyProcessorCount() const {
        return BusyProcessorCount;
    }

public:
    void Draw(arctic::Sprite toSprite) override {
        auto width = toSprite.Width();
        auto height = toSprite.Height();

        auto minDimension = std::min(width, height);
        auto yPos = height / 2 - minDimension / 2;

        arctic::Vec2Si32 bottomLeft(0, yPos);
        arctic::Vec2Si32 topRight(minDimension, yPos + minDimension);

        DrawRectangle(toSprite, bottomLeft, topRight, arctic::Rgba(255, 255, 255, 255));

        char text[128];
        snprintf(text, sizeof(text), "%s:\n%ld/%ld", Name, BusyProcessorCount, Processors.size());
        GetFont().Draw(toSprite, text, 10, yPos + minDimension / 2);
    }

private:
    const char* Name;

    std::vector<ProcessorType> Processors;
    size_t BusyProcessorCount = 0;
    size_t ReadyEventsCount = 0;
};

} // namespace queue_sim
