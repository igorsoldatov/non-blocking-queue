#pragma once

#include <vector>
#include <utility>
#include <stdint.h>

class IEventProcessor
{
public:
    using Integer = int64_t;

    //////////////////////////////////////////////////////////////////////////
    /// --
    struct ReservedEvent
    {
        ReservedEvent();
        ReservedEvent(const Integer sequence_number, void *const event);

        ReservedEvent(const ReservedEvent &) = delete;
        ReservedEvent &operator=(const ReservedEvent &) = delete;

        ReservedEvent(ReservedEvent &&) = delete;
        ReservedEvent &operator=(ReservedEvent &&) = delete;

        Integer GetSequenceNumber() const { return sequence_number_; }
        void *GetEvent() const { return event_; }

        bool IsValid() const { return event_ != nullptr; }

    private:
        Integer sequence_number_;
        void *event_;
    };

    //////////////////////////////////////////////////////////////////////////
    /// --
    struct ReservedEvents
    {
        ReservedEvents(const Integer sequence_number,
                       void *const event,
                       const size_t count,
                       const size_t event_size);

        ReservedEvents(const ReservedEvents &) = default;
        ReservedEvents &operator=(const ReservedEvents &) = delete;

        ReservedEvents(ReservedEvents &&) = default;
        ReservedEvents &operator=(ReservedEvents &&) = delete;

        template <class TEvent, class... Args>
        void Emplace(const size_t index, Args &&...args)
        {
            auto event = reinterpret_cast<TEvent *>(GetEvent(index, sizeof(TEvent)));
            if (event)
                std::construct_at(event, std::forward<Args>(args)...);
        }

        Integer GetSequenceNumber() const { return sequence_number_; }
        void *GetEvent(const size_t index, const size_t size) const { return reinterpret_cast<char*>(events_) + index * size; };

        size_t Count() const { return count_; }

        bool IsValid() const { return events_ != nullptr; }

    private:
        Integer sequence_number_;
        void *events_;
        size_t count_;
        size_t event_size_;
    };

    template <typename T>
    std::pair<size_t, void *const> ReserveEvent();

    //////////////////////////////////////////////////////////////////////////
    /// --
    template <class T, class... Args>
    ReservedEvent Reserve(Args &&...args)
    {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second)
            return ReservedEvent();

        std::construct_at(reservation.second, std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    //////////////////////////////////////////////////////////////////////////
    /// --
    std::vector<ReservedEvents> ReserveRange(const size_t size); // TODO: static vector?

    //////////////////////////////////////////////////////////////////////////
    /// --
    void Commit(const Integer sequence_number);

    //////////////////////////////////////////////////////////////////////////
    /// --
    void Commit(const Integer sequence_number, const size_t count);
};