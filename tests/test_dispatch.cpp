/*
Unit tests for dispatching.
1. Nearest dispatch should dispatch the closest fire station to the incident.
2. Firebeats should follow the zoning rules.
3. Dispatch should not dispatch if there are no available fire stations.
4. Dispatch should handle multiple incidents correctly.
5. Dispatch should prioritize incidents based on severity.****
    - this is not yet certain. Right now its priority based on first come first served basis.
6. Dispatch should log all actions taken.
7. Dispatch should handle edge cases like no incidents or all stations busy.
8. Dispatch should handle concurrent incidents correctly.
9. Dispatch should ensure that the nearest station is not already responding to another incident.
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <memory>

#include "policy/nearest_dispatch.h"
#include "simulator/state.h"
#include "utils/logger.h"
#include "config/EnvLoader.h"
#include <spdlog/spdlog.h>
// #include "data/incident.h"
// #include "data/station.h"
// #include "data/apparatus.h"
// #include "simulator/action.h"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class NearestDispatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnvLoader::init("../.env");
        // Initialize logger
        utils::Logger::init("boilerplate_app");
        utils::Logger::setLevel("info");
        // Create temporary test matrix files
        createTestMatrixFiles();
        
        // Initialize NearestDispatch with test files
        dispatch_ = std::make_unique<NearestDispatch>(distanceMatrixPath_, durationMatrixPath_);
        
        // Setup test state
        setupTestState();
    }

    void TearDown() override {
        // Clean up test files
        std::remove(distanceMatrixPath_.c_str());
        std::remove(durationMatrixPath_.c_str());
        
        dispatch_.reset();
        
        // Clean up logger to avoid conflicts between tests
        spdlog::drop("boilerplate_app");
        spdlog::shutdown();

        EnvLoader::cleanup(); // Clean up EnvLoader singleton
    }

    void createTestMatrixFiles() {
        // Create 3x3 test matrices (3 stations, 3 incidents)
        width_ = 3;
        height_ = 3;
        
        // Distance matrix (arbitrary values for testing)
        double distanceData[] = {
            100.0, 200.0, 300.0,  // Station 0 to incidents 0,1,2
            150.0, 100.0, 250.0,  // Station 1 to incidents 0,1,2  
            200.0, 150.0, 100.0   // Station 2 to incidents 0,1,2
        };
        
        // Duration matrix (station 0 closest to incident 0, station 1 closest to incident 1, etc.)
        double durationData[] = {
            60.0,  120.0, 180.0,  // Station 0: 1min, 2min, 3min
            90.0,  60.0,  150.0,  // Station 1: 1.5min, 1min, 2.5min
            120.0, 90.0,  60.0    // Station 2: 2min, 1.5min, 1min
        };
        
        writeMatrixToFile(distanceMatrixPath_, distanceData, width_, height_);
        writeMatrixToFile(durationMatrixPath_, durationData, width_, height_);
    }

    void writeMatrixToFile(const std::string& filename, double* data, int width, int height) {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(&width), sizeof(int));
        out.write(reinterpret_cast<const char*>(&height), sizeof(int));
        out.write(reinterpret_cast<const char*>(data), sizeof(double) * width * height);
        out.close();
    }

    void setupTestState() {
        // Create test stations
        stations_.clear();
        stations_.emplace_back(0, 101, 2, 1, -86.7809, 36.1608);  // Station 0: 2 engines, 1 ambulance
        stations_.emplace_back(1, 102, 3, 2, -86.7810, 36.1609);  // Station 1: 3 engines, 2 ambulances  
        stations_.emplace_back(2, 103, 1, 1, -86.7811, 36.1610);  // Station 2: 1 engine, 1 ambulance

        // Create test apparatus
        apparatuses_.clear();
        // Station 0 apparatus
        apparatuses_.emplace_back(0, 0, ApparatusType::Engine);
        apparatuses_.emplace_back(1, 0, ApparatusType::Engine);
        apparatuses_.emplace_back(2, 0, ApparatusType::Medic);
        
        // Station 1 apparatus  
        apparatuses_.emplace_back(3, 1, ApparatusType::Engine);
        apparatuses_.emplace_back(4, 1, ApparatusType::Engine);
        apparatuses_.emplace_back(5, 1, ApparatusType::Engine);
        apparatuses_.emplace_back(6, 1, ApparatusType::Medic);
        apparatuses_.emplace_back(7, 1, ApparatusType::Medic);
        
        // Station 2 apparatus
        apparatuses_.emplace_back(8, 2, ApparatusType::Engine);
        apparatuses_.emplace_back(9, 2, ApparatusType::Medic);

        // Initialize station apparatus counts
        for (auto& apparatus : apparatuses_) {
            int stationIndex = apparatus.getStationIndex();
            Station& station = stations_.at(stationIndex);
            station.updateAvailableCount(apparatus.getType(), 1);
            station.updateTotalCount(apparatus.getType(), 1);
        }

        // Create test incidents
        incidents_.clear();
        time_t currentTime = std::time(nullptr);
        
        // Incident 0: Should be closest to Station 0
        incidents_.emplace_back(0, 201, 36.1608, -86.7809, 
                               IncidentType::Fire, IncidentLevel::Low, currentTime);
        incidents_[0].resolvedTime = currentTime + 1800; // 30 minutes to resolve
        
        // Incident 1: Should be closest to Station 1  
        incidents_.emplace_back(1, 202, 36.1609, -86.7810,
                               IncidentType::Fire, IncidentLevel::Moderate, currentTime);
        incidents_[1].resolvedTime = currentTime + 2400; // 40 minutes to resolve
        
        // Incident 2: Should be closest to Station 2
        incidents_.emplace_back(2, 203, 36.1610, -86.7811,
                               IncidentType::Fire, IncidentLevel::High, currentTime);
        incidents_[2].resolvedTime = currentTime + 3600; // 60 minutes to resolve

        // Set required apparatus for incidents
        std::unordered_map<ApparatusType, int> lowRequirement = {{ApparatusType::Engine, 1}};
        std::unordered_map<ApparatusType, int> moderateRequirement = {{ApparatusType::Engine, 2}};
        std::unordered_map<ApparatusType, int> highRequirement = {{ApparatusType::Engine, 3}, {ApparatusType::Medic, 1}};
        
        incidents_[0].setRequiredApparatusMap(lowRequirement);
        incidents_[1].setRequiredApparatusMap(moderateRequirement);
        incidents_[2].setRequiredApparatusMap(highRequirement);
    }

    State createMockState(int activeIncidentIndex = 0) {
        State state;
        state.advanceTime(std::time(nullptr));
        
        // Add stations
        for (const auto& station : stations_) {
            state.addStation(station);
        }
        
        // Add apparatus
        for (const auto& apparatus : apparatuses_) {
            state.addApparatus(apparatus);
        }
        
        // Add active incident
        if (activeIncidentIndex >= 0 && static_cast<size_t>(activeIncidentIndex) < incidents_.size()) {
            state.getActiveIncidents().insert({activeIncidentIndex, incidents_[activeIncidentIndex]});
            state.inProgressIncidentIndices.push_back(activeIncidentIndex);
        }
        
        return state;
    }

protected:
    std::unique_ptr<NearestDispatch> dispatch_;
    std::vector<Station> stations_;
    std::vector<Apparatus> apparatuses_;
    std::vector<Incident> incidents_;
    
    std::string distanceMatrixPath_ = "test_distance_matrix.bin";
    std::string durationMatrixPath_ = "test_duration_matrix.bin";
    int width_, height_;
};

// Test basic functionality
TEST_F(NearestDispatchTest, DispatchesToNearestStation) {
    // Setup: Incident 0 should dispatch from Station 0 (closest)
    State state = createMockState(0);
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_FALSE(actions.empty());
    EXPECT_EQ(actions[0].type, StationActionType::Dispatch);
    EXPECT_EQ(actions[0].payload.stationIndex, 0);  // Station 0 is closest to incident 0
    EXPECT_EQ(actions[0].payload.incidentIndex, 0);
    EXPECT_EQ(actions[0].payload.apparatusType, ApparatusType::Engine);
    EXPECT_EQ(actions[0].payload.apparatusCount, 1);  // Low level incident needs 1 engine
    EXPECT_DOUBLE_EQ(actions[0].payload.travelTime, 60.0);  // 1 minute travel time
}

TEST_F(NearestDispatchTest, DispatchesToSecondNearestWhenFirstUnavailable) {
    // Setup: Make Station 0 unavailable by dispatching all its engines
    State state = createMockState(0);
    
    // Dispatch all engines from station 0
    auto& station0 = state.getStation(0);
    station0.dispatchApparatus(ApparatusType::Engine, 2);  // Dispatch both engines
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert  
    ASSERT_FALSE(actions.empty());
    EXPECT_EQ(actions[0].payload.stationIndex, 1);  // Should dispatch from Station 1 (second closest)
    EXPECT_DOUBLE_EQ(actions[0].payload.travelTime, 90.0);  // 1.5 minute travel time
}

TEST_F(NearestDispatchTest, HandlesMultipleApparatusTypes) {
    // Setup: High level incident requiring multiple apparatus types
    State state = createMockState(2);  // Incident 2 requires 3 engines + 1 ambulance
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_GE(actions.size(), 2);  // Should have multiple actions for different apparatus types
    
    // Check for engine dispatch
    auto engineAction = std::find_if(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.apparatusType == ApparatusType::Engine; });
    ASSERT_NE(engineAction, actions.end());
    EXPECT_EQ(engineAction->payload.stationIndex, 2);  // Station 2 closest to incident 2
    
    // Check for medic dispatch
    auto medicAction = std::find_if(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.apparatusType == ApparatusType::Medic; });
    ASSERT_NE(medicAction, actions.end());
}

TEST_F(NearestDispatchTest, ReturnsDoNothingWhenNoIncidents) {
    // Setup: State with no active incidents
    State state = createMockState(-1);  // No active incidents
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, StationActionType::DoNothing);
}

TEST_F(NearestDispatchTest, ReturnsDoNothingWhenNoStationsAvailable) {
    // Setup: Dispatch all apparatus from all stations
    State state = createMockState(0);
    
    for (auto& station : state.getAllStations_()) {
        int available_engine = station.getAvailableCount(ApparatusType::Engine);
        int available_medic = station.getAvailableCount(ApparatusType::Medic);
        station.dispatchApparatus(ApparatusType::Engine, available_engine);  // Dispatch more than available
        station.dispatchApparatus(ApparatusType::Medic, available_medic);
        state.dispatchApparatus(ApparatusType::Engine, available_engine, station.getStationIndex());
        state.dispatchApparatus(ApparatusType::Medic, available_medic, station.getStationIndex());
    }
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, StationActionType::DoNothing);
}

TEST_F(NearestDispatchTest, RespectsTravelTimeConstraints) {
    // Setup: Create incident with very short resolution time
    State state = createMockState(0);
    
    // Make incident resolve very soon (before any station can reach)
    state.getActiveIncidents().at(0).resolvedTime = state.getSystemTime() + 30; // 30 seconds
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert - should return DoNothing since no station can reach in time
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, StationActionType::DoNothing);
}

TEST_F(NearestDispatchTest, DispatchesOptimalAmountPerStation) {
    // Setup: Moderate incident requiring 2 engines, station 0 has 2 engines
    State state = createMockState(1);  // Incident 1 requires 2 engines
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_FALSE(actions.empty());
    auto engineAction = std::find_if(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.apparatusType == ApparatusType::Engine; });
    
    ASSERT_NE(engineAction, actions.end());
    EXPECT_EQ(engineAction->payload.stationIndex, 1);  // Station 1 closest to incident 1  
    EXPECT_EQ(engineAction->payload.apparatusCount, 2);  // Should dispatch 2 engines
}

// Performance test
TEST_F(NearestDispatchTest, PerformanceTest) {
    State state = createMockState(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run multiple iterations
    for (int i = 0; i < 1000; ++i) {
        std::vector<Action> actions = dispatch_->getAction(state);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 iterations in reasonable time (< 1 second)
    EXPECT_LT(duration.count(), 1000);
    std::cout << "1000 iterations completed in " << duration.count() << "ms" << std::endl;
}

// Integration test with actual matrix loading
class NearestDispatchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Skip if matrix files don't exist
        if (!std::ifstream("../logs/distance_matrix.bin") || !std::ifstream("../logs/duration_matrix.bin")) {
            GTEST_SKIP() << "Matrix files not found, skipping integration test";
        }
        
        dispatch_ = std::make_unique<NearestDispatch>("../logs/distance_matrix.bin", "../logs/duration_matrix.bin");
    }

protected:
    std::unique_ptr<NearestDispatch> dispatch_;
};

TEST_F(NearestDispatchIntegrationTest, LoadsRealMatrixFiles) {
    // This test verifies that the constructor can load real matrix files
    EXPECT_NE(dispatch_, nullptr);
    // Additional tests with real data can be added here
}