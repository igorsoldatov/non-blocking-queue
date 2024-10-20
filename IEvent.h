#pragma once

class IEvent
{
public:
    virtual ~IEvent() = default;
    virtual void Process() = 0;
};
