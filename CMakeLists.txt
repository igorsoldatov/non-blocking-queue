cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)

project ("EventProcessor")

add_executable(event_processor "main.cpp" "Event.h" "EventProcessor.h" "IEvent.h" "IEventProcessor.h" )