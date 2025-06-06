#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include "dispatch_policy.h"
#include "data/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
#include <memory>

class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(const std::string& osrmUrl);  // e.g., http://localhost:5000

    void dispatch(State& state, const Event& event) override;

private:
    std::string osrmUrl_;

    // Helper function to extract incident from the event
    std::shared_ptr<Incident> extractIncident(const Event& event) const;
};

#endif // NEAREST_DISPATCH_H
