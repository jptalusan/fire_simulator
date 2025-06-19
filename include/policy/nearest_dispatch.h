#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include <memory>
#include "dispatch_policy.h"
#include "data/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
    
class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(const std::string& distanceMatrixPath="", const std::string& durationMatrixPath="");

    std::vector<Action> getAction(const State& state) override;

    ~NearestDispatch();
private:
    Queries queries_;
    std::string distanceMatrixPath_;
    std::string durationMatrixPath_;
    double* distanceMatrix_;
    double* durationMatrix_;
    int width_;
    int height_;

    // Helper function to extract incident from the event
    int findMinIndex(const std::vector<double>& durations);
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations);
    std::vector<double> getColumn(double* matrix, int width, int height, int col_index) const;
};

#endif // NEAREST_DISPATCH_H
