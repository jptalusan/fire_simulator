#ifndef DISPATCH_POLICY_H
#define DISPATCH_POLICY_H

#include "simulator/state.h"
#include "simulator/event.h"
#include "simulator/action.h"

class DispatchPolicy {
public:
    virtual ~DispatchPolicy() = default;

    // Handle dispatching logic given the state and an event
    virtual Action getAction(const State& state) = 0;
};

#endif // DISPATCH_POLICY_H
