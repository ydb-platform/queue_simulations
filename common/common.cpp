#include "common.h"

namespace queue_sim {

static double CurrentTimeSeconds = 0;

// ----------------------------
// our global time

double Now() {
    return CurrentTimeSeconds;
}

void AdvanceTime(double dt) {
    CurrentTimeSeconds += dt;
}

// ----------------------------
// helpers

arctic::Font& GetFont() {
    static arctic::Font font;
    static bool loaded = false;
    if (!loaded) {
        font.Load("data/JetBrainsMono.fnt");
        loaded = true;
    }

    return font;
}

std::string NumToStrWithSuffix(size_t num) {
    if (num < 1000) {
        return std::to_string(num);
    } else if (num < 1000000) {
        return std::to_string(num / 1000) + "K";
    } else if (num < 1000000000) {
        return std::to_string(num / 1000000) + "M";
    } else {
        return std::to_string(num / 1000000000) + "G";
    }
}

// ----------------------------
// Histogram
//

Histogram Histogram::HistogramWithUsBuckets() {
    return Histogram(
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            16, 24, 32, 40, 48, 50, 54, 62, 70,
            80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
            250, 300, 350, 450, 500, 750, 1000, 1250, 1500, 1750, 2000,
            2250, 2500, 2750, 3000, 3250, 3500, 3750, 4000, 4250, 4500, 4750, 5000,
            6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000, 20000,
            24000, 32000, 40000, 48000, 56000, 64000,
            128000, 256000, 512000,
            1000000, 1500000, 2000000, 3000000, 4000000
        });
}

// ----------------------------
// Event
//

Event Event::NewEvent() {
    return Event();
}

Event Event::NewEvent(size_t src, size_t dst) {
    return Event(src, dst);
}

size_t Event::EventCounter = 0;

// ----------------------------
// ItemBase
//

size_t ItemBase::ItemCounter = 0;

} // namespace queue_sim
