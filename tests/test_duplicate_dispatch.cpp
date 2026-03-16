/**
 * Unit tests for duplicate-dispatch prevention.
 *
 * The double-dispatch bug occurred in firebeats_dispatch.cpp when the beat
 * matrix listed the same station at multiple priority positions for a zone.
 * Because dispatch actions are batched and vehicle status is not updated
 * until takeActions() processes them, the same vehicle could be picked
 * twice in a single getAction() call.  The fix added an
 * std::unordered_set<int> dispatchedVehicleIds tracker.
 *
 * These tests verify that no vehicle ID appears more than once in the
 * actions returned by a single getAction() call, for both FireBeatsDispatch
 * and NearestDispatch.
 */

#include <gtest/gtest.h>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>

#include "policy/firebeats_dispatch.h"
#include "policy/nearest_dispatch.h"
#include "simulator/state.h"
#include "simulator/action.h"
#include "objects/vehicle.h"
#include "objects/firestation.h"
#include "objects/incident.h"
#include "objects/common.h"
#include "enums.h"
#include "utils/logger.h"
#include "config/EnvLoader.h"
#include <spdlog/spdlog.h>

// ---------------------------------------------------------------------------
// Mock TravelTimeModel
// ---------------------------------------------------------------------------
// Each source is assigned a fixed travel time to any destination.
// getTravelTimeMatrix returns an Nx1 matrix (N = number of sources, 1 dest).
class MockTravelTimeModel : public TravelTimeModel {
public:
    // sourceTimes[i] = travel time from source i to any destination
    std::vector<double> sourceTimes;

    explicit MockTravelTimeModel(const std::vector<double>& times = {})
        : sourceTimes(times) {}

    std::pair<float, std::vector<Location>>
    getTravelTimeAndRoute(const Location& /*from*/, const Location& /*to*/) override {
        return {100.0f, {}};
    }

    std::vector<std::vector<double>>
    getTravelTimeMatrix(const std::vector<Location>& sources,
                        const std::vector<Location>& /*destinations*/) override {
        std::vector<std::vector<double>> matrix;
        matrix.reserve(sources.size());
        for (size_t i = 0; i < sources.size(); ++i) {
            double t = (i < sourceTimes.size()) ? sourceTimes[i] : 100.0;
            matrix.push_back({t});
        }
        return matrix;
    }
};

// ---------------------------------------------------------------------------
// Helper: assert no vehicle ID appears more than once in dispatch actions
// ---------------------------------------------------------------------------
static void assertNoDuplicateVehicles(const std::vector<Action>& actions,
                                      const std::string& context) {
    std::unordered_set<int> seen;
    for (const auto& action : actions) {
        if (action.type != StationActionType::Dispatch) continue;
        int vid = action.payload.vehicleIndex;
        ASSERT_EQ(seen.count(vid), 0u)
            << context << ": vehicle " << vid << " dispatched more than once";
        seen.insert(vid);
    }
}

// ---------------------------------------------------------------------------
// Helper: write a binary int matrix (width, height, data[width*height])
// ---------------------------------------------------------------------------
static void writeIntMatrix(const std::string& path, int width, int height,
                           const std::vector<int>& data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&width), sizeof(int));
    out.write(reinterpret_cast<const char*>(&height), sizeof(int));
    out.write(reinterpret_cast<const char*>(data.data()),
              sizeof(int) * static_cast<size_t>(width * height));
    out.close();
}

// ---------------------------------------------------------------------------
// Helper: write a zone CSV (ZoneID,Zone Name) with numZones entries (0..N-1)
// ---------------------------------------------------------------------------
static void writeZoneCSV(const std::string& path, int numZones) {
    std::ofstream out(path);
    out << "ZoneID,Zone Name\n";
    for (int i = 0; i < numZones; ++i) {
        out << i << "," << i << "\n";
    }
    out.close();
}

// ---------------------------------------------------------------------------
// Helper: create a Vehicle
// ---------------------------------------------------------------------------
static Vehicle makeVehicle(int id, int stationIdx, const std::string& stationId,
                           ApparatusType type, const Location& location,
                           ApparatusStatus status = ApparatusStatus::Available) {
    Vehicle v(stationIdx, stationId, id, location, type, status);
    return v;
}

// ---------------------------------------------------------------------------
// Helper: build a State from stations, vehicles, and an optional incident.
//
// IMPORTANT: vehicle IDs must form a contiguous range starting at 0 because
// the dispatch code does state.getConstVehicleList().at(vehicleId).  The
// vehicles vector must therefore be ordered so that the vehicle with ID k
// sits at index k.
//
// This helper also populates each station's apparatus map so that
// getAvailableApparatus returns the correct vehicle IDs.
// ---------------------------------------------------------------------------
static State buildState(std::vector<FireStation>& stations,
                        const std::vector<Vehicle>& vehicles,
                        const Incident* incident = nullptr) {
    State state;
    state.advanceTime(std::time(nullptr));

    // Group vehicles by (stationIndex, type)
    std::unordered_map<int, std::unordered_map<int, std::vector<Vehicle>>>
        stationTypeVehicles;
    for (const auto& v : vehicles) {
        stationTypeVehicles[v.getStationIndex()]
                           [static_cast<int>(v.getType())]
                           .push_back(v);
    }

    // Populate each station's apparatus maps
    for (auto& station : stations) {
        auto stIt = stationTypeVehicles.find(station.stationIndex);
        if (stIt == stationTypeVehicles.end()) continue;
        for (auto& [typeInt, vecs] : stIt->second) {
            auto aType = static_cast<ApparatusType>(typeInt);
            station.addApparatusToMap(aType, vecs);
            station.updateAvailableCount(aType, static_cast<int>(vecs.size()));
        }
    }

    // Add stations to state
    for (const auto& station : stations) {
        state.addStation(station);
    }

    // Add vehicles to state in order (index == vehicleId)
    for (const auto& v : vehicles) {
        state.addVehicle(v);
    }

    if (incident) {
        state.newIncident_ = *incident;
    }

    return state;
}

// ===================================================================
//  FireBeats duplicate-dispatch tests
// ===================================================================
class FireBeatsDuplicateTest : public ::testing::Test {
protected:
    static constexpr const char* kBinPath = "/tmp/test_beats_dup.bin";
    static constexpr const char* kCsvPath = "/tmp/test_zones_dup.csv";

    void SetUp() override {
        EnvLoader::init(R"({"CONSOLE_LOG_LEVEL":"warn","LOGS_PATH":""})");
        utils::Logger::init("dup_dispatch_test");
    }

    void TearDown() override {
        std::remove(kBinPath);
        std::remove(kCsvPath);
        spdlog::drop("dup_dispatch_test");
        spdlog::shutdown();
        EnvLoader::cleanup();
    }
};

// 1. Beat matrix repeats station 0 at positions 0 and 1 for zone 0.
//    Station 0 has 1 Engine, Station 1 has 1 Engine.
//    Incident needs 2 Engines.
//    Should dispatch 2 unique engines (one from each station), not the same one twice.
TEST_F(FireBeatsDuplicateTest, EngineNotDispatchedTwiceFromSameStation) {
    // Beat matrix: 3 stations (height=3), 1 zone (width=1).
    // Column 0 (zone 0): station order = [0, 0, 1]
    //   -> station 0 appears at priority 0 and 1.
    int width = 1, height = 3;
    writeIntMatrix(kBinPath, width, height, {0, 0, 1});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);
    Location loc1(36.17, -86.79);
    Location loc2(36.18, -86.80);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
        FireStation("S1", 1, loc1),
        FireStation("S2", 2, loc2),
    };

    // Vehicle IDs must be contiguous starting at 0.
    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),  // id=0
        makeVehicle(1, 1, "S1", ApparatusType::Engine, loc1),  // id=1
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Engine, 2}});

    MockTravelTimeModel ttm({60.0, 90.0, 120.0}); // one per station

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    // Count dispatch actions
    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 2) << "Should dispatch 2 engines total";
    assertNoDuplicateVehicles(actions, "EngineNotDispatchedTwiceFromSameStation");
}

// 2. 1 station repeated 3 times in beat matrix. Station has 1 Truck.
//    Incident needs 3 Trucks.  Should dispatch only 1 (the only one available).
TEST_F(FireBeatsDuplicateTest, TruckNotDispatchedMultipleTimes) {
    int width = 1, height = 3;
    writeIntMatrix(kBinPath, width, height, {0, 0, 0});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Truck, loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Truck, 3}});

    MockTravelTimeModel ttm({60.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 1) << "Only 1 truck available, should dispatch 1";
    assertNoDuplicateVehicles(actions, "TruckNotDispatchedMultipleTimes");
}

// 3. 1 station repeated 3 times. Station has 1 Engine + 1 Truck + 1 Rescue.
//    Incident needs all three.  Should dispatch 3 unique vehicles.
TEST_F(FireBeatsDuplicateTest, MixedApparatusTypesNoDuplicates) {
    int width = 1, height = 3;
    writeIntMatrix(kBinPath, width, height, {0, 0, 0});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
        makeVehicle(1, 0, "S0", ApparatusType::Truck,  loc0),
        makeVehicle(2, 0, "S0", ApparatusType::Rescue, loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({
        {ApparatusType::Engine, 1},
        {ApparatusType::Truck, 1},
        {ApparatusType::Rescue, 1},
    });

    MockTravelTimeModel ttm({60.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 3) << "Should dispatch 3 distinct vehicles";
    assertNoDuplicateVehicles(actions, "MixedApparatusTypesNoDuplicates");
}

// 4. 2 Medics at station 0 with same location/travel time.
//    Incident needs 1 Engine + 1 Medic.  Should dispatch exactly 1 Medic.
TEST_F(FireBeatsDuplicateTest, MedicNotDuplicatedWhenSameTravelTime) {
    int width = 1, height = 2;
    writeIntMatrix(kBinPath, width, height, {0, 1});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);
    Location loc1(36.17, -86.79);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
        FireStation("S1", 1, loc1),
    };

    // IDs must be contiguous from 0.
    // id 0 = Engine at S0, id 1 = Medic at S0, id 2 = Medic at S0
    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
        makeVehicle(1, 0, "S0", ApparatusType::Medic,  loc0),
        makeVehicle(2, 0, "S0", ApparatusType::Medic,  loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::Medical, IncidentLevel::Moderate,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({
        {ApparatusType::Engine, 1},
        {ApparatusType::Medic, 1},
    });

    // Station travel times (for fire apparatus): station 0 = 60, station 1 = 90
    // Medic travel times will be derived from vehicle locations.
    // Both medics are at loc0, so they get the same travel time.
    MockTravelTimeModel ttm({60.0, 60.0, 60.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int medicCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch &&
            a.payload.apparatusType == ApparatusType::Medic) {
            ++medicCount;
        }
    }
    EXPECT_EQ(medicCount, 1) << "Only 1 Medic should be dispatched";
    assertNoDuplicateVehicles(actions, "MedicNotDuplicatedWhenSameTravelTime");
}

// 5. 1 station (beat repeats), station has 2 Engines.
//    Incident needs 2 Engines.  Both should be dispatched (different IDs), no dups.
TEST_F(FireBeatsDuplicateTest, TwoEnginesSameStationBothDispatched) {
    int width = 1, height = 2;
    writeIntMatrix(kBinPath, width, height, {0, 0});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
        makeVehicle(1, 0, "S0", ApparatusType::Engine, loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Engine, 2}});

    MockTravelTimeModel ttm({60.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 2) << "Both engines should be dispatched";
    assertNoDuplicateVehicles(actions, "TwoEnginesSameStationBothDispatched");
}

// 6. 4 stations, beat alternates 0,1,0,1.  Multiple vehicle types.
//    Incident needs Engine(2) + Pumper(1) + Truck(1) + Rescue(1) + Chief(1).
//    All 6 vehicles dispatched, no duplicates.
TEST_F(FireBeatsDuplicateTest, AllFireApparatusTypesNoDuplicates) {
    int width = 1, height = 4;
    writeIntMatrix(kBinPath, width, height, {0, 1, 0, 1});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);
    Location loc1(36.17, -86.79);
    Location loc2(36.18, -86.80);
    Location loc3(36.19, -86.81);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
        FireStation("S1", 1, loc1),
        FireStation("S2", 2, loc2),
        FireStation("S3", 3, loc3),
    };

    // 6 vehicles spread across stations 0 and 1.
    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine,  loc0), // Engine #1
        makeVehicle(1, 1, "S1", ApparatusType::Engine,  loc1), // Engine #2
        makeVehicle(2, 0, "S0", ApparatusType::Pumper,  loc0),
        makeVehicle(3, 1, "S1", ApparatusType::Truck,   loc1),
        makeVehicle(4, 0, "S0", ApparatusType::Rescue,  loc0),
        makeVehicle(5, 1, "S1", ApparatusType::Chief,   loc1),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::Critical,
                 std::time(nullptr), IncidentCategory::Three);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({
        {ApparatusType::Engine, 2},
        {ApparatusType::Pumper, 1},
        {ApparatusType::Truck,  1},
        {ApparatusType::Rescue, 1},
        {ApparatusType::Chief,  1},
    });

    MockTravelTimeModel ttm({60.0, 90.0, 120.0, 150.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 6) << "All 6 vehicles should be dispatched";
    assertNoDuplicateVehicles(actions, "AllFireApparatusTypesNoDuplicates");
}

// 7. 1 station (beat repeats), 2 Engines but one is Dispatched (not Available).
//    Incident needs 2 Engines.  Only 1 should be dispatched (the Available one).
TEST_F(FireBeatsDuplicateTest, SkipsNonAvailableVehicles) {
    int width = 1, height = 2;
    writeIntMatrix(kBinPath, width, height, {0, 0});
    writeZoneCSV(kCsvPath, 1);

    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0, ApparatusStatus::Available),
        makeVehicle(1, 0, "S0", ApparatusType::Engine, loc0, ApparatusStatus::Dispatched),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Engine, 2}});

    MockTravelTimeModel ttm({60.0});

    State state = buildState(stations, vehicles, &inc);

    FireBeatsDispatch dispatch(ttm, kBinPath, kCsvPath, stations);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 1)
        << "Only 1 engine is Available, should dispatch 1";
    assertNoDuplicateVehicles(actions, "SkipsNonAvailableVehicles");
}

// ===================================================================
//  NearestDispatch duplicate-dispatch tests
// ===================================================================
class NearestDuplicateTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnvLoader::init(R"({"CONSOLE_LOG_LEVEL":"warn","LOGS_PATH":""})");
        utils::Logger::init("dup_nearest_test");
    }

    void TearDown() override {
        spdlog::drop("dup_nearest_test");
        spdlog::shutdown();
        EnvLoader::cleanup();
    }
};

// 1. 2 stations, 1 Engine each. Incident needs 2 Engines.
//    2 dispatches, no duplicates.
TEST_F(NearestDuplicateTest, EngineNoDuplicates) {
    Location loc0(36.16, -86.78);
    Location loc1(36.17, -86.79);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
        FireStation("S1", 1, loc1),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
        makeVehicle(1, 1, "S1", ApparatusType::Engine, loc1),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Engine, 2}});

    MockTravelTimeModel ttm({60.0, 90.0});

    State state = buildState(stations, vehicles, &inc);

    NearestDispatch dispatch(stations, ttm);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 2) << "Should dispatch 2 engines";
    assertNoDuplicateVehicles(actions, "EngineNoDuplicates");
}

// 2. 2 stations with various types.
//    Incident needs Engine + Truck + Medic.  3 dispatches, no duplicates.
TEST_F(NearestDuplicateTest, MixedTypesNoDuplicates) {
    Location loc0(36.16, -86.78);
    Location loc1(36.17, -86.79);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
        FireStation("S1", 1, loc1),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
        makeVehicle(1, 1, "S1", ApparatusType::Truck,  loc1),
        makeVehicle(2, 0, "S0", ApparatusType::Medic,  loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::BuildingFire, IncidentLevel::High,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({
        {ApparatusType::Engine, 1},
        {ApparatusType::Truck, 1},
        {ApparatusType::Medic, 1},
    });

    // Station times for fire apparatus (2 stations) + medic times (1 medic at loc0)
    MockTravelTimeModel ttm({60.0, 90.0, 60.0});

    State state = buildState(stations, vehicles, &inc);

    NearestDispatch dispatch(stations, ttm);
    auto actions = dispatch.getAction(state);

    int dispatchCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch) ++dispatchCount;
    }
    EXPECT_EQ(dispatchCount, 3) << "Should dispatch Engine + Truck + Medic";
    assertNoDuplicateVehicles(actions, "MixedTypesNoDuplicates");
}

// 3. 2 Medics at station 0 with same travel time.
//    Incident needs 1 Medic.  Exactly 1 dispatched.
TEST_F(NearestDuplicateTest, MedicSameTravelTimeNoDuplicates) {
    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Medic, loc0),
        makeVehicle(1, 0, "S0", ApparatusType::Medic, loc0),
    };

    Incident inc(0, 100, 36.16, -86.78,
                 IncidentType::Medical, IncidentLevel::Moderate,
                 std::time(nullptr), IncidentCategory::One);
    inc.zoneIndex = 0;
    inc.setRequiredApparatusMap({{ApparatusType::Medic, 1}});

    MockTravelTimeModel ttm({60.0, 60.0});

    State state = buildState(stations, vehicles, &inc);

    NearestDispatch dispatch(stations, ttm);
    auto actions = dispatch.getAction(state);

    int medicCount = 0;
    for (const auto& a : actions) {
        if (a.type == StationActionType::Dispatch &&
            a.payload.apparatusType == ApparatusType::Medic) {
            ++medicCount;
        }
    }
    EXPECT_EQ(medicCount, 1) << "Exactly 1 Medic should be dispatched";
    assertNoDuplicateVehicles(actions, "MedicSameTravelTimeNoDuplicates");
}

// 4. No incident set.  Actions should be empty.
TEST_F(NearestDuplicateTest, NoIncidentReturnsEmpty) {
    Location loc0(36.16, -86.78);

    std::vector<FireStation> stations = {
        FireStation("S0", 0, loc0),
    };

    std::vector<Vehicle> vehicles = {
        makeVehicle(0, 0, "S0", ApparatusType::Engine, loc0),
    };

    MockTravelTimeModel ttm({60.0});

    // Build state WITHOUT an incident (pass nullptr)
    State state = buildState(stations, vehicles, nullptr);

    NearestDispatch dispatch(stations, ttm);
    auto actions = dispatch.getAction(state);

    EXPECT_TRUE(actions.empty())
        << "No incident means no dispatch actions";
}
