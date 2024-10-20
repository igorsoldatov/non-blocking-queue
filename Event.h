#pragma once
#include "IEvent.h"

class Event : public IEvent
{
public:
    explicit Event(const int value) : value_(value) {}
    ~Event() override = default;

    Event(const Event &) = delete;

    Event &operator=(const Event &) = delete;

    Event(Event &&) = delete;
    Event &operator=(Event &&) = delete;

    void Process() override;
    
private:
    int value_;
};


void Event::Process() {
    // do something
}