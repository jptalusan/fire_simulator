#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include "dispatch_policy.h"
#include "data/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
#include <memory>

class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(const std::string& osrmUrl);  // e.g., http://localhost:5000

    int getAction(State& state) override;

private:
    std::string osrmUrl_;
    Queries queries_;

    // Helper function to extract incident from the event
    std::shared_ptr<Incident> extractIncident(const Event& event) const;
    int findMinIndex(const std::vector<double>& durations);
};

#endif // NEAREST_DISPATCH_H
