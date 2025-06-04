// event_data.h
#ifndef EVENT_DATA_H
#define EVENT_DATA_H

class EventData {
public:
    virtual ~EventData() = default;
    virtual void printInfo() const = 0;
};

#endif // EVENT_DATA_H