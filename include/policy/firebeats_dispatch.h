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
                      const std::string& fireBeatsMatrixPath="",
                      const std::string& zoneIDToNameMapPath="");

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
    int* fireBeatsMatrix_;
    int fireBeatsWidth_;
    int fireBeatsHeight_;
    std::unordered_map<int, std::string> beatsIndexToNameMap_;

    // Helper function to extract incident from the event
    int findMinIndex(const std::vector<double>& durations);
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations);
    std::vector<double> getColumn(double* matrix, int width, int height, int col_index) const;
    std::vector<int> getColumn(int* matrix, int width, int height, int col_index) const;
    int* getFireBeats(const std::string& filename, int& height, int& width) const;
    std::unordered_map<int, std::string> readZoneIndexToNameMapCSV(const std::string& filename) const;
};

#endif // FIREBEATS_DISPATCH_H
