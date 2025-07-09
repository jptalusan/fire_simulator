#ifndef DISPATCH_POLICY_H
#define DISPATCH_POLICY_H

#include "simulator/state.h"
#include "simulator/event.h"
#include "simulator/action.h"

class DispatchPolicy {
public:
    virtual ~DispatchPolicy() = default;

    // Handle dispatching logic given the state and an event
    virtual std::vector<Action> getAction(const State& state) = 0;

protected:
    int getNextIncidentIndex(const State& state) const;
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations);
    int findMinIndex(const std::vector<double>& durations);
    std::vector<double> getColumn(double* matrix, int width, int height, int col_index) const;
    std::vector<int> getColumn(int* matrix, int width, int height, int col_index) const;
};

#endif // DISPATCH_POLICY_H
