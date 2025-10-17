#ifndef TRAVEL_TIME_MODEL_H
#define TRAVEL_TIME_MODEL_H

class TravelTimeModel {
public:
    virtual std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) = 0;
    virtual std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) = 0;
    virtual ~TravelTimeModel() = default;
};

class OSRMTravelTimeModel : public TravelTimeModel {
public:
    OSRMTravelTimeModel(const std::string& base_url = "") : base_url_(base_url) {}
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;
private:
    std::string base_url_;
};

class InterpolatedTravelTimeModel : public TravelTimeModel {
public:
    InterpolatedTravelTimeModel(const std::vector<Location>& grid_locations,
                                const std::vector<std::vector<double>>& travel_time_matrix)
        : grid_locations_(grid_locations), travel_time_matrix_(travel_time_matrix) {}
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;
private:
    std::vector<Location> grid_locations_;
    std::vector<std::vector<double>> travel_time_matrix_;
};

class ESRITravelTimeModel : public TravelTimeModel {
public:
    ESRITravelTimeModel(const std::string& api_key) : api_key_(api_key) {}
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;
private:
    std::string api_key_;
};

#endif // TRAVEL_TIME_MODEL_H