#ifndef FIREBEATS_DISPATCH_H
#define FIREBEATS_DISPATCH_H

#include <memory>
#include "dispatch_policy.h"
#include "data/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
    
class FireBeatsDispatch : public DispatchPolicy {
public:
    FireBeatsDispatch(const std::string& distanceMatrixPath="", 
                      const std::string& durationMatrixPath="",
                      const std::string& fireBeatsMatrixPath="");

    std::vector<Action> getAction(const State& state) override;

    ~FireBeatsDispatch();
private:
    Queries queries_;
    // Distance and duration matrices
    std::string distanceMatrixPath_;
    std::string durationMatrixPath_;
    double* distanceMatrix_;
    double* durationMatrix_;
    int width_;
    int height_;
    // FireBeats matrix
    std::string fireBeatsMatrixPath_;
    std::unordered_map<std::string, std::vector<std::string>> fireBeatsMatrix_;
    int fireBeatsWidth_;
    int fireBeatsHeight_;

    // Helper function to extract incident from the event
    int findMinIndex(const std::vector<double>& durations);
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations);
    std::vector<double> getColumn(double* matrix, int width, int height, int col_index) const;
    std::unordered_map<std::string, std::vector<std::string>> getFireBeats(const std::string& filename, int& height, int& width) const;
};

#endif // FIREBEATS_DISPATCH_H
