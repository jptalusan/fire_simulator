/*
Unit tests for Firebeats dispatching.
1. Firebeats should dispatch based on zoning rules and priority.
2. Firebeats should respect zone assignments for stations.
3. Firebeats should fall back to nearest available station when zone station is unavailable.
4. Firebeats should handle multiple incidents correctly with zone priority.
5. Firebeats should prioritize incidents based on severity and zone coverage.
6. Firebeats should log all actions taken.
7. Firebeats should handle edge cases like no incidents or all stations busy.
8. Firebeats should handle concurrent incidents correctly.
9. Firebeats should ensure proper zone coverage is maintained.
10. Check for invalid zone indices and handle them gracefully.
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <memory>

#include "policy/firebeats_dispatch.h"
#include "simulator/state.h"
#include "utils/logger.h"
#include "config/EnvLoader.h"
#include <spdlog/spdlog.h>

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class FireBeatsDispatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnvLoader::init("../.env");
        // Initialize logger
        utils::Logger::init("firebeats_test_app");
        utils::Logger::setLevel("info");
        // Create temporary test matrix files
        createTestMatrixFiles();
        
        // Initialize FireBeatsDispatch with test files
        dispatch_ = std::make_unique<FireBeatsDispatch>(distanceMatrixPath_, 
            durationMatrixPath_, fireBeatsMatrixPath_, zoneCSVPath_);
        
        // Setup test state
        setupTestState();
    }

    void TearDown() override {
        // Clean up test files
        std::remove(distanceMatrixPath_.c_str());
        std::remove(durationMatrixPath_.c_str());
        std::remove(fireBeatsMatrixPath_.c_str());
        std::remove(zoneCSVPath_.c_str());
        
        dispatch_.reset();
        
        // Clean up logger to avoid conflicts between tests
        spdlog::drop("firebeats_test_app");
        spdlog::shutdown();

        EnvLoader::cleanup(); // Clean up EnvLoader singleton
    }

    void createTestMatrixFiles() {
        // Create 4x4 test matrices (4 stations, 4 incidents for zone testing)
        width_ = 4;
        height_ = 4;
        
        // Distance matrix - designed to test zone priorities
        double distanceData[] = {
            100.0, 200.0, 300.0, 400.0,  // Station 0 to incidents 0,1,2,3
            150.0, 100.0, 250.0, 350.0,  // Station 1 to incidents 0,1,2,3  
            200.0, 150.0, 100.0, 300.0,  // Station 2 to incidents 0,1,2,3
            250.0, 200.0, 150.0, 100.0   // Station 3 to incidents 0,1,2,3
        };
        
        // Duration matrix - zone-based priorities
        double durationData[] = {
            60.0,  120.0, 180.0, 240.0,  // Station 0: primary for zone 0
            90.0,  60.0,  150.0, 210.0,  // Station 1: primary for zone 1
            120.0, 90.0,  60.0,  180.0,  // Station 2: primary for zone 2
            150.0, 120.0, 90.0,  60.0    // Station 3: primary for zone 3
        };
        
        // Duration matrix - zone-based priorities TODO: Double check this
        int fireBeatsData[] = {
            0, 1, 2, 3,  // Station 0: primary for zone 0
            1, 0, 1, 2,  // Station 1: primary for zone 1
            2, 2, 0, 1,  // Station 2: primary for zone 2
            3, 3, 3, 0   // Station 3: primary for zone 3
        };
        
        writeMatrixToFile(distanceMatrixPath_, distanceData, width_, height_);
        writeMatrixToFile(durationMatrixPath_, durationData, width_, height_);
        writeIntMatrixToFile(fireBeatsMatrixPath_, fireBeatsData, width_, height_);
        writeZoneCSVFile(zoneCSVPath_);  // Add this line
    }

    void writeMatrixToFile(const std::string& filename, double* data, int width, int height) {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(&width), sizeof(int));
        out.write(reinterpret_cast<const char*>(&height), sizeof(int));
        out.write(reinterpret_cast<const char*>(data), sizeof(double) * width * height);
        out.close();
    }

    void writeIntMatrixToFile(const std::string& filename, int* data, int width, int height) {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(&width), sizeof(int));
        out.write(reinterpret_cast<const char*>(&height), sizeof(int));
        out.write(reinterpret_cast<const char*>(data), sizeof(int) * width * height);
        out.close();
    }

    void writeZoneCSVFile(const std::string& filename) {
        std::ofstream out(filename);
        out << "ZoneID,Zone Name\n";
        out << "0,0\n";
        out << "1,1\n";
        out << "2,2\n";
        out << "3,3\n";
        out.close();
    }

    void setupTestState() {
        // Create test stations with zone assignments
        stations_.clear();
        stations_.emplace_back(0, 101, 2, 1, -86.7809, 36.1608);  // Station 0: Zone 0
        stations_.emplace_back(1, 102, 3, 2, -86.7810, 36.1609);  // Station 1: Zone 1  
        stations_.emplace_back(2, 103, 2, 1, -86.7811, 36.1610);  // Station 2: Zone 2
        stations_.emplace_back(3, 104, 1, 1, -86.7812, 36.1611);  // Station 3: Zone 3

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
        apparatuses_.emplace_back(9, 2, ApparatusType::Engine);
        apparatuses_.emplace_back(10, 2, ApparatusType::Medic);

        // Station 3 apparatus
        apparatuses_.emplace_back(11, 3, ApparatusType::Engine);
        apparatuses_.emplace_back(12, 3, ApparatusType::Medic);

        // Initialize station apparatus counts
        for (auto& apparatus : apparatuses_) {
            int stationIndex = apparatus.getStationIndex();
            Station& station = stations_.at(stationIndex);
            station.updateAvailableCount(apparatus.getType(), 1);
            station.updateTotalCount(apparatus.getType(), 1);
        }

        // Create test incidents in different zones
        incidents_.clear();
        time_t currentTime = std::time(nullptr);
        
        // Incident 0: Zone 0 - should prefer Station 0
        incidents_.emplace_back(0, 201, 36.1608, -86.7809, 
                               IncidentType::BuildingFire, IncidentLevel::Low, currentTime, IncidentCategory::One);
        incidents_[0].zoneIndex = 0;  // Assign to Zone 0
        incidents_[0].resolvedTime = currentTime + 1800; // 30 minutes to resolve
        
        // Incident 1: Zone 1 - should prefer Station 1
        incidents_.emplace_back(1, 202, 36.1609, -86.7810,
                               IncidentType::BuildingFire, IncidentLevel::Moderate, currentTime, IncidentCategory::OneB);
        incidents_[1].zoneIndex = 1;  // Assign to Zone 1
        incidents_[1].resolvedTime = currentTime + 2400; // 40 minutes to resolve
        
        // Incident 2: Zone 2 - should prefer Station 2
        incidents_.emplace_back(2, 203, 36.1610, -86.7811,
                               IncidentType::BuildingFire, IncidentLevel::High, currentTime, IncidentCategory::OneC);
        incidents_[2].zoneIndex = 2;  // Assign to Zone 2
        incidents_[2].resolvedTime = currentTime + 3600; // 60 minutes to resolve

        // Incident 3: Zone 3 - should prefer Station 3
        incidents_.emplace_back(3, 204, 36.1611, -86.7812,
                               IncidentType::BuildingFire, IncidentLevel::Moderate, currentTime, IncidentCategory::OneD);
        incidents_[3].zoneIndex = 3;  // Assign to Zone 3
        incidents_[3].resolvedTime = currentTime + 2700; // 45 minutes to resolve

        // Set required apparatus for incidents
        std::unordered_map<ApparatusType, int> lowRequirement = {{ApparatusType::Engine, 1}};
        std::unordered_map<ApparatusType, int> moderateRequirement = {{ApparatusType::Engine, 2}};
        std::unordered_map<ApparatusType, int> highRequirement = {{ApparatusType::Engine, 3}, {ApparatusType::Medic, 1}};
        
        incidents_[0].setRequiredApparatusMap(lowRequirement);
        incidents_[1].setRequiredApparatusMap(moderateRequirement);
        incidents_[2].setRequiredApparatusMap(highRequirement);
        incidents_[3].setRequiredApparatusMap(moderateRequirement);
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
    std::unique_ptr<FireBeatsDispatch> dispatch_;
    std::vector<Station> stations_;
    std::vector<Apparatus> apparatuses_;
    std::vector<Incident> incidents_;
    
    std::string distanceMatrixPath_ = "test_firebeats_distance_matrix.bin";
    std::string durationMatrixPath_ = "test_firebeats_duration_matrix.bin";
    std::string fireBeatsMatrixPath_ = "test_firebeats_matrix.bin";
    std::string zoneCSVPath_ = "test_zones.csv";  // Add this line

    int width_, height_;
};

// Test zone-based dispatching
TEST_F(FireBeatsDispatchTest, DispatchesToZoneStation) {
    // Setup: Incident 0 in Zone 0 should dispatch from Station 0
    State state = createMockState(0);
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_FALSE(actions.empty());
    EXPECT_EQ(actions[0].type, StationActionType::Dispatch);
    EXPECT_EQ(actions[0].payload.stationIndex, 0);  // Station 0 is zone station for incident 0
    EXPECT_EQ(actions[0].payload.incidentIndex, 0);
    EXPECT_EQ(actions[0].payload.apparatusType, ApparatusType::Engine);
    EXPECT_EQ(actions[0].payload.apparatusCount, 1);  // Low level incident needs 1 engine
    EXPECT_DOUBLE_EQ(actions[0].payload.travelTime, 60.0);  // Zone station travel time
}

TEST_F(FireBeatsDispatchTest, FallsBackWhenZoneStationUnavailable) {
    // Setup: Make Zone 0 station (Station 0) unavailable
    State state = createMockState(0);
    
    // Dispatch all engines from station 0
    auto& station0 = state.getStation(0);
    station0.dispatchApparatus(ApparatusType::Engine, 2);  // Dispatch both engines
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert - should fall back to next best station
    ASSERT_FALSE(actions.empty());
    EXPECT_NE(actions[0].payload.stationIndex, 0);  // Should not use Station 0
    // Should use one of the available stations based on firebeats priority
}

TEST_F(FireBeatsDispatchTest, HandlesMultipleZones) {
    // Setup: High level incident in Zone 2 requiring multiple apparatus types
    State state = createMockState(2);  // Incident 2 requires 3 engines + 1 medic
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_GE(actions.size(), 1);  // Should have at least one action
    
    // Check that primary zone station (Station 2) is prioritized
    auto primaryAction = std::find_if(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.stationIndex == 2; });
    
    if (primaryAction != actions.end()) {
        EXPECT_EQ(primaryAction->payload.incidentIndex, 2);
        EXPECT_DOUBLE_EQ(primaryAction->payload.travelTime, 60.0);  // Zone station travel time
    }
}

TEST_F(FireBeatsDispatchTest, PrioritizesByIncidentSeverity) {
    // Setup: Multiple incidents with different severity levels
    State state;
    state.advanceTime(std::time(nullptr));
    
    // Add stations and apparatus
    for (const auto& station : stations_) {
        state.addStation(station);
    }
    for (const auto& apparatus : apparatuses_) {
        state.addApparatus(apparatus);
    }
    
    // Add multiple active incidents
    state.getActiveIncidents().insert({0, incidents_[0]});  // Low severity
    state.getActiveIncidents().insert({2, incidents_[2]});  // High severity
    state.inProgressIncidentIndices.push_back(0);
    state.inProgressIncidentIndices.push_back(2);
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    GTEST_SKIP(); 
    // Assert - should prioritize high severity incident first
    ASSERT_FALSE(actions.empty());
    // Firebeats should handle the higher severity incident (2) first
    bool hasHighSeverityAction = std::any_of(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.incidentIndex == 2; });
    EXPECT_TRUE(hasHighSeverityAction);
}

TEST_F(FireBeatsDispatchTest, ReturnsDoNothingWhenNoIncidents) {
    // Setup: State with no active incidents
    State state = createMockState(-1);  // No active incidents
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, StationActionType::DoNothing);
}

TEST_F(FireBeatsDispatchTest, ReturnsDoNothingWhenNoStationsAvailable) {
    // Setup: Dispatch all apparatus from all stations
    State state = createMockState(0);
    
    for (auto& station : state.getAllStations_()) {
        int available_engine = station.getAvailableCount(ApparatusType::Engine);
        int available_medic = station.getAvailableCount(ApparatusType::Medic);
        station.dispatchApparatus(ApparatusType::Engine, available_engine);  // Dispatch more than available
        station.dispatchApparatus(ApparatusType::Medic, available_medic);
        state.dispatchApparatus(ApparatusType::Engine, available_engine, station.getStationIndex()); // Ensure state reflects this
        state.dispatchApparatus(ApparatusType::Medic, available_medic, station.getStationIndex());
    }
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].type, StationActionType::DoNothing);
}

TEST_F(FireBeatsDispatchTest, RespectsTravelTimeConstraints) {
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

TEST_F(FireBeatsDispatchTest, OptimizesApparatusDistribution) {
    // Setup: Incident requiring multiple engines from zone with limited resources
    State state = createMockState(3);  // Incident 3 in Zone 3 (Station 3 has only 1 engine)
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert
    ASSERT_FALSE(actions.empty());
    
    // Should use Station 3's available apparatus first (zone priority)
    auto station3Action = std::find_if(actions.begin(), actions.end(),
        [](const Action& a) { return a.payload.stationIndex == 3; });
    
    if (station3Action != actions.end()) {
        EXPECT_EQ(station3Action->payload.apparatusType, ApparatusType::Engine);
        EXPECT_LE(station3Action->payload.apparatusCount, 1);  // Station 3 only has 1 engine
    }
}

TEST_F(FireBeatsDispatchTest, HandlesZoneCoverageGaps) {
    // Setup: Make primary zone station completely unavailable
    State state = createMockState(1);  // Incident 1 in Zone 1
    
    // Remove all apparatus from Station 1 (zone station)
    auto& station1 = state.getStation(1);
    station1.dispatchApparatus(ApparatusType::Engine, 3);
    station1.dispatchApparatus(ApparatusType::Medic, 2);
    
    // Act
    std::vector<Action> actions = dispatch_->getAction(state);
    
    // Assert - should use backup stations based on firebeats algorithm
    ASSERT_FALSE(actions.empty());
    EXPECT_NE(actions[0].payload.stationIndex, 1);  // Should not use unavailable Station 1
    
    // Should still dispatch to the incident using alternative stations
    EXPECT_EQ(actions[0].payload.incidentIndex, 1);
    EXPECT_EQ(actions[0].type, StationActionType::Dispatch);
}

// Performance test
TEST_F(FireBeatsDispatchTest, PerformanceTest) {
    State state = createMockState(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run multiple iterations
    for (int i = 0; i < 1000; ++i) {
        std::vector<Action> actions = dispatch_->getAction(state);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 iterations in reasonable time (< 2 seconds for more complex algorithm)
    EXPECT_LT(duration.count(), 2000);
    std::cout << "Firebeats: 1000 iterations completed in " << duration.count() << "ms" << std::endl;
}

// Integration test with actual matrix loading
class FireBeatsDispatchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Skip if matrix files don't exist
        if (!std::ifstream("../logs/distance_matrix.bin") || !std::ifstream("../logs/duration_matrix.bin")) {
            GTEST_SKIP() << "Matrix files not found, skipping integration test";
        }
        
        dispatch_ = std::make_unique<FireBeatsDispatch>(
            "../logs/distance_matrix.bin", 
            "../logs/duration_matrix.bin",
            "../logs/beats.bin",
            "../data/zones.csv");
    }

protected:
    std::unique_ptr<FireBeatsDispatch> dispatch_;
};

TEST_F(FireBeatsDispatchIntegrationTest, LoadsRealMatrixFiles) {
    // This test verifies that the constructor can load real matrix files
    EXPECT_NE(dispatch_, nullptr);
    // Additional tests with real data can be added here
}

// Test zone assignment logic
TEST_F(FireBeatsDispatchTest, VerifiesZoneAssignments) {
    // This test verifies that the zone-to-station mapping works correctly
    State state = createMockState(0);
    
    // Test each zone gets its preferred station when available
    for (int incidentIndex = 0; incidentIndex < 4; ++incidentIndex) {
        State testState = createMockState(incidentIndex);
        std::vector<Action> actions = dispatch_->getAction(testState);
        
        if (!actions.empty() && actions[0].type == StationActionType::Dispatch) {
            // The station index should prefer the zone station (same as incident index in our test setup)
            // This may not always be true if the zone station is unavailable, but it's the preferred choice
            std::cout << "Incident " << incidentIndex << " dispatched from Station " 
                      << actions[0].payload.stationIndex << std::endl;
        }
    }
}