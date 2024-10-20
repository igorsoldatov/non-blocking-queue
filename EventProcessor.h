#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <stdint.h>
#include <atomic>
#include <thread>
#include <optional>
#include "Event.h"
#include "IEventProcessor.h"

class EventProcessor : public IEventProcessor
{
public:
    EventProcessor(const size_t capacity, const size_t event_size)
        : capacity_(capacity), event_size_(event_size), buffer_(capacity * event_size), reserve_pos_(0), write_pos_(0), read_pos_(0)
    {
    }

    template <typename T>
    std::pair<size_t, void *const> ReserveEvent();

    template <class T, class... Args>
    ReservedEvent Reserve(Args &&...args)
    {
        const auto reservation = ReserveEvent<T>();
        if (!reservation.second)
            return ReservedEvent();

        std::construct_at(reinterpret_cast<T *>(reservation.second), std::forward<Args>(args)...);
        return ReservedEvent(reservation.first, reservation.second);
    }

    std::vector<ReservedEvents> ReserveRange(const size_t size);

    void Commit(const Integer sequence_number);

    void Commit(const Integer sequence_number, const size_t count);

    template<class T>
    std::optional<T*> Pop();

private:
    size_t capacity_;
    size_t event_size_;
    std::vector<uint8_t> buffer_;
    std::atomic<Integer> reserve_pos_;
    std::atomic<Integer> write_pos_;
    std::atomic<Integer> read_pos_;

    inline size_t GetCellNumber(const Integer sequence_number) {
        return (sequence_number - 1) % capacity_;
    }
    inline void* GetCellAddress(const Integer sequence_number) {
        const auto cell = GetCellNumber(sequence_number);
        return &(buffer_[cell * event_size_]);
    }
};


template <typename T>
std::pair<size_t, void *const> EventProcessor::ReserveEvent()
{
    Integer current_reserved, next_reserved, current_read;
    bool stop = false;

    do
    {
        current_reserved = reserve_pos_.load();
        current_read = read_pos_.load();
        next_reserved = current_reserved + 1;

        if (next_reserved - current_read >= capacity_)
        {
            // Buffer is full
            stop = false;
        } else {
            stop = reserve_pos_.compare_exchange_weak(current_reserved, next_reserved);
        }
    } while (!stop);

    void* event_cell_address = GetCellAddress(next_reserved);
    return {static_cast<size_t>(next_reserved), event_cell_address};
}

template<class T>
std::optional<T*> EventProcessor::Pop()
{
    auto current_read = read_pos_.load();
    auto current_write = write_pos_.load();
    auto next_read = current_read + 1;

    if (next_read > current_write)
        return {};
    
    const auto event = reinterpret_cast<T*>(GetCellAddress(next_read));
    event->Process();

    read_pos_.store(next_read);

    return {event};
}


EventProcessor::ReservedEvent::ReservedEvent()
    : sequence_number_(0), event_(nullptr) {}

EventProcessor::ReservedEvent::ReservedEvent(const Integer sequence_number, void *const event)
    : sequence_number_(sequence_number), event_(event) {}

EventProcessor::ReservedEvents::ReservedEvents(const Integer sequence_number,
                                                void *const event,
                                                const size_t count,
                                                const size_t event_size)
    : sequence_number_(sequence_number), events_(event), count_(count), event_size_(event_size) {}


void EventProcessor::Commit(const Integer sequence_number, const size_t count)
{
    Integer current_write, current_reserve;
    bool stop = false;
    do
    {
        current_write = write_pos_.load();
        current_reserve = reserve_pos_.load();

        if (sequence_number == current_write + count)
        {
            stop = write_pos_.compare_exchange_weak(current_write, sequence_number);
        } else {
            stop = false;
        }
    } while (!stop);
}

void EventProcessor::Commit(const Integer sequence_number)
{
    Integer current_write, current_reserve;
    bool stop = false;
    do
    {
        current_write = write_pos_.load();
        current_reserve = reserve_pos_.load();

        if (sequence_number == current_write + 1)
        {
            stop = write_pos_.compare_exchange_weak(current_write, sequence_number);
        } else {
            stop = false;
        }
    } while (!stop);
}

std::vector<EventProcessor::ReservedEvents> EventProcessor::ReserveRange(const size_t size)
{
    std::vector<ReservedEvents> reserved_events;
    Integer current_reserved, next_reserved, current_read;
    bool stop = false;

    do
    {
        current_reserved = reserve_pos_.load();
        current_read = read_pos_.load();
        next_reserved = current_reserved + size;

        if (next_reserved - current_read >= capacity_)
        {
            // Buffer is full
            stop = false;
        } else {
            stop = reserve_pos_.compare_exchange_weak(current_reserved, next_reserved);
        }
    } while (!stop);

    if ((current_reserved + 1) / capacity_ == next_reserved / capacity_){
        auto cell_address = GetCellAddress(current_reserved + 1);
        reserved_events.emplace_back(next_reserved, cell_address, size, event_size_);
    } else {     
        const size_t size2 = next_reserved % capacity_;
        const size_t size1 = size - size2;
        auto cell_address1 = GetCellAddress(current_reserved + 1);
        reserved_events.emplace_back(current_reserved + size1, cell_address1, size1, event_size_);
        //reserved_events.emplace_back(next_reserved, reinterpret_cast<void*>(&(buffer_[0])), size2, event_size_);
    }

    return std::move(reserved_events);
}
