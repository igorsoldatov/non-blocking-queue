#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <algorithm>
#include <ranges>
#include "EventProcessor.h"
#include "Event.h"

void Writer(EventProcessor &event_processor, const int id, const int num_events_to_reserve)
{
    for (int i = 0; i < 10'000'000; i = i + num_events_to_reserve)
    {
        // Reservation of one event
        if (num_events_to_reserve == 1)
        {
            const auto data = (i + 1) + id * 100'000'000;
            auto reserved_event = event_processor.Reserve<Event>(data);
            if (!reserved_event.IsValid())
            {
                // ERROR: Reserve() failed ...
                std::this_thread::yield();
                continue;
            }
            else
            {
                event_processor.Commit(reserved_event.GetSequenceNumber());
            }
            continue;
        }

        // queue multiple events
        auto reserved_events_collection = event_processor.ReserveRange(num_events_to_reserve); // It can reserve less items than requested! You should always check how many events have been reserved!
        if (reserved_events_collection.empty())
        {
            // ERROR: ReserveRange() failed
        }
        else
        {
            std::ranges::for_each(reserved_events_collection,
                                  [&](IEventProcessor::ReservedEvents &reserved_events)
                                  {
                                      if (!reserved_events.IsValid())
                                      {
                                          // ERROR: Reserve() failed
                                      }
                                      else
                                      {
                                        const auto size = reserved_events.Count();
                                          for (size_t j = 0; j < size; ++j)
                                          {
                                              const auto data = reserved_events.GetSequenceNumber() - size + j + 1;
                                              reserved_events.Emplace<Event>(j, static_cast<int>(data));
                                          }
                                          event_processor.Commit(reserved_events.GetSequenceNumber(), size);
                                      }
                                  });
        }
    }
}

void ProcessEvents(EventProcessor &event_processor)
{
    while (true)
    {
        const auto event = event_processor.Pop<Event>();
        if (!event.has_value()){
            std::this_thread::yield();
        }
    }
}

int main()
{
    const size_t capacity = 1024 * 1024;
    const size_t event_size = sizeof(Event);
    EventProcessor processor(capacity, event_size);

    std::vector<std::thread> writers;
    for (int i = 0; i < 16; ++i)
    {
        writers.emplace_back(Writer, std::ref(processor), i + 1, i + 1);
    }

    std::thread reader(ProcessEvents, std::ref(processor));

    for (auto &writer : writers)
    {
        writer.join();
    }

    reader.join();

    return 0;
}