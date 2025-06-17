#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include <memory>
#include "dispatch_policy.h"
#include "data/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
    
class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(const std::string& osrmUrl);  // e.g., http://localhost:5000

    Action getAction(const State& state) override;

private:
    std::string osrmUrl_;
    Queries queries_;

    // Helper function to extract incident from the event
    int findMinIndex(const std::vector<double>& durations);
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations);
};

#endif // NEAREST_DISPATCH_H
