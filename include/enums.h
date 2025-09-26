#ifndef ENUMS_H
#define ENUMS_H

#include <string>
#include <algorithm>
#include <cctype>

enum class EventType : uint8_t {
    Incident,
    CheckIncident,
    ApparatusArrivalAtIncident,
    IncidentResolution,
    ApparatusReturnToStation
};

enum class StationActionType : uint8_t {
    Dispatch,
    DoNothing
};

enum class IncidentLevel : uint8_t {
    Invalid,
    Low,
    Moderate,
    High,
    Critical
};

enum class IncidentCategory : uint8_t {
    One,
    OneB,
    OneBM,
    OneC,
    OneD,
    OneE,
    OneEM,
    OneF,
    OneG,
    OneH,
    OneJ,
    Two,
    TwoM,
    TwoMF,
    TwoA,
    TwoB,
    TwoC,
    Three,
    ThreeF,
    ThreeM,
    ThreeA,
    ThreeB,
    ThreeC,
    ThreeCM,
    ThreeD,
    Four,
    FourM,
    FourA,
    FourB,
    FourC,
    Five,
    FiveA,
    Six,
    Seven,
    SevenB,
    SevenBM,
    Eight,
    EightA,
    EightB,
    EightC,
    EightD,
    EightE,
    EightF,
    EightG,
    Nine,
    Ten,
    Eleven,
    ElevenA,
    ElevenB,
    Thirteen,
    Fourteen,
    Fifteen,
    Sixteen,
    Eighteen,
    Invalid
};

enum class IncidentType : uint8_t {
    // Fire-related incidents
    BuildingFire,
    PassengerVehicleFire,
    GrassFire,
    BrushFire,
    OutsideRubbishFire,
    CookingFire,
    DumpsterFire,
    ChimneyFire,
    ForestFire,
    CamperFire,
    
    // Medical incidents
    Medical,
    
    // Alarm-related incidents
    AlarmSystemActivation,
    SmokeDetectorActivation,
    CODetectorActivation,
    SprinklerActivation,
    
    // Hazmat incidents
    GasLeak,
    ChemicalSpill,
    HazmatRelease,
    CarbonMonoxideIncident,
    
    // Service calls
    PublicService,
    AnimalRescue,
    LockOut,
    ServiceCall,
    
    // Other incidents
    VehicleAccident,
    PowerLineDown,
    WaterLeak,
    
    // Special categories
    CoverAssignment,
    NoIncidentFound,
    Cancelled,
    
    // Fallback
    Other,
    Invalid
};

enum class IncidentStatus : uint8_t {
    hasBeenReported,
    hasBeenRespondedTo,
    isBeingResolved,
    hasBeenResolved
};

enum class ApparatusStatus : uint8_t {
    Available,
    Dispatched,
    EnRouteToIncident,
    AtIncident,
    ReturningToStation
};

enum class ApparatusType : uint8_t {
    Pumper,
    Engine,
    Truck,
    Rescue,
    Hazard,
    Chief,
    Squad,
    Fast,
    Medic,
    Brush,
    Boat,
    UTV,
    Reach,
    Invalid // Added for safety, should not be used in practice
};

// to_string for IncidentType
inline std::string to_string(IncidentType type) {
    switch (type) {
        // Fire-related incidents
        case IncidentType::BuildingFire: return "Building fire";
        case IncidentType::PassengerVehicleFire: return "Passenger vehicle fire";
        case IncidentType::GrassFire: return "Grass fire";
        case IncidentType::BrushFire: return "Brush fire";
        case IncidentType::OutsideRubbishFire: return "Outside rubbish fire";
        case IncidentType::CookingFire: return "Cooking fire";
        case IncidentType::DumpsterFire: return "Dumpster fire";
        case IncidentType::ChimneyFire: return "Chimney or flue fire";
        case IncidentType::ForestFire: return "Forest, woods or wildland fire";
        case IncidentType::CamperFire: return "Camper or motor home fire";
        
        // Medical incidents
        case IncidentType::Medical: return "Medical";
        
        // Alarm-related incidents
        case IncidentType::AlarmSystemActivation: return "Alarm system activation, no fire - unintentional";
        case IncidentType::SmokeDetectorActivation: return "Smoke detector activation, no fire - unintentional";
        case IncidentType::CODetectorActivation: return "CO detector activation, no CO";
        case IncidentType::SprinklerActivation: return "Sprinkler activation, no fire - unintentional";
        
        // Hazmat incidents
        case IncidentType::GasLeak: return "Gas leak (natural gas or LPG)";
        case IncidentType::ChemicalSpill: return "Chemical spill or leak";
        case IncidentType::HazmatRelease: return "Hazardous materials release investigation";
        case IncidentType::CarbonMonoxideIncident: return "Carbon monoxide incident";
        
        // Service calls
        case IncidentType::PublicService: return "Public service";
        case IncidentType::AnimalRescue: return "Animal rescue";
        case IncidentType::LockOut: return "Lock-out";
        case IncidentType::ServiceCall: return "Service call";
        
        // Other incidents
        case IncidentType::VehicleAccident: return "Vehicle accident with no injuries";
        case IncidentType::PowerLineDown: return "Power line down";
        case IncidentType::WaterLeak: return "Water or steam leak";
        
        // Special categories
        case IncidentType::CoverAssignment: return "Cover assignment, standby, moveup";
        case IncidentType::NoIncidentFound: return "No incident found on arrival at dispatch address";
        case IncidentType::Cancelled: return "Dispatched and cancelled en route";
        
        // Fallback
        case IncidentType::Other: return "Good intent call -  other";
        case IncidentType::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

// Function to map incident type strings to IncidentType enum
inline IncidentType mapIncidentType(const std::string& incidentTypeStr) {
    // Convert to lowercase for case-insensitive comparison
    std::string lowerStr = incidentTypeStr;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    
    // Direct matches (case-insensitive)
    if (lowerStr == "building fire") return IncidentType::BuildingFire;
    if (lowerStr == "passenger vehicle fire") return IncidentType::PassengerVehicleFire;
    if (lowerStr == "grass fire") return IncidentType::GrassFire;
    if (lowerStr == "brush fire") return IncidentType::BrushFire;
    if (lowerStr == "outside rubbish fire") return IncidentType::OutsideRubbishFire;
    if (lowerStr == "cooking fire") return IncidentType::CookingFire;
    if (lowerStr == "dumpster fire") return IncidentType::DumpsterFire;
    if (lowerStr == "chimney or flue fire") return IncidentType::ChimneyFire;
    if (lowerStr == "forest, woods or wildland fire") return IncidentType::ForestFire;
    if (lowerStr == "camper or motor home fire") return IncidentType::CamperFire;
    if (lowerStr == "medical") return IncidentType::Medical;
    if (lowerStr == "alarm system activation, no fire - unintentional") return IncidentType::AlarmSystemActivation;
    if (lowerStr == "smoke detector activation, no fire - unintentional") return IncidentType::SmokeDetectorActivation;
    if (lowerStr == "co detector activation, no co") return IncidentType::CODetectorActivation;
    if (lowerStr == "sprinkler activation, no fire - unintentional") return IncidentType::SprinklerActivation;
    if (lowerStr == "gas leak (natural gas or lpg)") return IncidentType::GasLeak;
    if (lowerStr == "chemical spill or leak") return IncidentType::ChemicalSpill;
    if (lowerStr == "hazardous materials release investigation") return IncidentType::HazmatRelease;
    if (lowerStr == "carbon monoxide incident") return IncidentType::CarbonMonoxideIncident;
    if (lowerStr == "public service") return IncidentType::PublicService;
    if (lowerStr == "animal rescue") return IncidentType::AnimalRescue;
    if (lowerStr == "lock-out") return IncidentType::LockOut;
    if (lowerStr == "service call") return IncidentType::ServiceCall;
    if (lowerStr == "vehicle accident with no injuries") return IncidentType::VehicleAccident;
    if (lowerStr == "power line down") return IncidentType::PowerLineDown;
    if (lowerStr == "water or steam leak") return IncidentType::WaterLeak;
    if (lowerStr == "cover assignment, standby, moveup") return IncidentType::CoverAssignment;
    if (lowerStr == "no incident found on arrival at dispatch address") return IncidentType::NoIncidentFound;
    if (lowerStr == "cancelled en route") return IncidentType::Cancelled;
    
    // Pattern matching for similar types (case-insensitive)
    if (lowerStr.find("fire") != std::string::npos) {
        if (lowerStr.find("vehicle") != std::string::npos || lowerStr.find("car") != std::string::npos || 
            lowerStr.find("truck") != std::string::npos || lowerStr.find("auto") != std::string::npos ||
            lowerStr.find("motor") != std::string::npos) return IncidentType::PassengerVehicleFire;
        if (lowerStr.find("building") != std::string::npos || lowerStr.find("structure") != std::string::npos ||
            lowerStr.find("residential") != std::string::npos || lowerStr.find("commercial") != std::string::npos) 
            return IncidentType::BuildingFire;
        if (lowerStr.find("grass") != std::string::npos || lowerStr.find("vegetation") != std::string::npos ||
            lowerStr.find("field") != std::string::npos) 
            return IncidentType::GrassFire;
        if (lowerStr.find("brush") != std::string::npos || lowerStr.find("wildland") != std::string::npos ||
            lowerStr.find("woods") != std::string::npos || lowerStr.find("forest") != std::string::npos) 
            return IncidentType::BrushFire;
        if (lowerStr.find("rubbish") != std::string::npos || lowerStr.find("trash") != std::string::npos ||
            lowerStr.find("garbage") != std::string::npos || lowerStr.find("refuse") != std::string::npos) 
            return IncidentType::OutsideRubbishFire;
        if (lowerStr.find("cooking") != std::string::npos || lowerStr.find("kitchen") != std::string::npos) 
            return IncidentType::CookingFire;
        if (lowerStr.find("dumpster") != std::string::npos || lowerStr.find("container") != std::string::npos) 
            return IncidentType::DumpsterFire;
        if (lowerStr.find("chimney") != std::string::npos || lowerStr.find("flue") != std::string::npos) 
            return IncidentType::ChimneyFire;
        // If it contains "fire" but doesn't match specific patterns, it's still a fire-related incident
        return IncidentType::BuildingFire; // Default fire incidents to building fire instead of Other
    }
    
    if (lowerStr.find("alarm") != std::string::npos || lowerStr.find("detector") != std::string::npos) {
        if (lowerStr.find("smoke") != std::string::npos) return IncidentType::SmokeDetectorActivation;
        if (lowerStr.find("co") != std::string::npos || lowerStr.find("carbon monoxide") != std::string::npos) 
            return IncidentType::CODetectorActivation;
        if (lowerStr.find("sprinkler") != std::string::npos) return IncidentType::SprinklerActivation;
        return IncidentType::AlarmSystemActivation;
    }
    
    if (lowerStr.find("leak") != std::string::npos || lowerStr.find("spill") != std::string::npos) {
        if (lowerStr.find("gas") != std::string::npos || lowerStr.find("lpg") != std::string::npos || 
            lowerStr.find("natural gas") != std::string::npos) return IncidentType::GasLeak;
        if (lowerStr.find("water") != std::string::npos || lowerStr.find("steam") != std::string::npos ||
            lowerStr.find("pipe") != std::string::npos) 
            return IncidentType::WaterLeak;
        if (lowerStr.find("chemical") != std::string::npos || lowerStr.find("hazmat") != std::string::npos ||
            lowerStr.find("hazardous") != std::string::npos) 
            return IncidentType::ChemicalSpill;
        return IncidentType::GasLeak; // Default leaks to gas leak
    }
    
    if (lowerStr.find("medical") != std::string::npos || lowerStr.find("ems") != std::string::npos ||
        lowerStr.find("emergency medical") != std::string::npos) return IncidentType::Medical;
    
    if ((lowerStr.find("accident") != std::string::npos || lowerStr.find("collision") != std::string::npos) && 
        (lowerStr.find("vehicle") != std::string::npos || lowerStr.find("car") != std::string::npos ||
         lowerStr.find("auto") != std::string::npos)) 
        return IncidentType::VehicleAccident;
    
    if (lowerStr.find("rescue") != std::string::npos && lowerStr.find("animal") != std::string::npos) 
        return IncidentType::AnimalRescue;
    
    if (lowerStr.find("service") != std::string::npos && 
        (lowerStr.find("public") != std::string::npos || lowerStr.find("assist") != std::string::npos)) 
        return IncidentType::PublicService;
    
    if (lowerStr.find("cover") != std::string::npos || lowerStr.find("standby") != std::string::npos ||
        lowerStr.find("moveup") != std::string::npos || lowerStr.find("move up") != std::string::npos) 
        return IncidentType::CoverAssignment;
    
    if (lowerStr.find("cancel") != std::string::npos || lowerStr.find("cancelled") != std::string::npos) 
        return IncidentType::Cancelled;
    
    if ((lowerStr.find("lock") != std::string::npos && lowerStr.find("out") != std::string::npos) ||
        lowerStr.find("lockout") != std::string::npos) 
        return IncidentType::LockOut;
    
    if (lowerStr.find("power") != std::string::npos && lowerStr.find("line") != std::string::npos) 
        return IncidentType::PowerLineDown;
    
    // Default fallback
    return IncidentType::Other;
}

// to_string for IncidentCategory
inline std::string to_string(IncidentCategory category) {
    switch (category) {
        case IncidentCategory::One: return "One";
        case IncidentCategory::OneB: return "OneB";
        case IncidentCategory::OneBM: return "OneBM";
        case IncidentCategory::OneC: return "OneC";
        case IncidentCategory::OneD: return "OneD";
        case IncidentCategory::OneE: return "OneE";
        case IncidentCategory::OneEM: return "OneEM";
        case IncidentCategory::OneF: return "OneF";
        case IncidentCategory::OneG: return "OneG";
        case IncidentCategory::OneH: return "OneH";
        case IncidentCategory::OneJ: return "OneJ";
        case IncidentCategory::Two: return "Two";
        case IncidentCategory::TwoM: return "TwoM";
        case IncidentCategory::TwoMF: return "TwoMF";
        case IncidentCategory::TwoA: return "TwoA";
        case IncidentCategory::TwoB: return "TwoB";
        case IncidentCategory::TwoC: return "TwoC";
        case IncidentCategory::Three: return "Three";
        case IncidentCategory::ThreeF: return "ThreeF";
        case IncidentCategory::ThreeM: return "ThreeM";
        case IncidentCategory::ThreeA: return "ThreeA";
        case IncidentCategory::ThreeB: return "ThreeB";
        case IncidentCategory::ThreeC: return "ThreeC";
        case IncidentCategory::ThreeCM: return "ThreeCM";
        case IncidentCategory::ThreeD: return "ThreeD";
        case IncidentCategory::Four: return "Four";
        case IncidentCategory::FourM: return "FourM";
        case IncidentCategory::FourA: return "FourA";
        case IncidentCategory::FourB: return "FourB";
        case IncidentCategory::FourC: return "FourC";
        case IncidentCategory::Five: return "Five";
        case IncidentCategory::FiveA: return "FiveA";
        case IncidentCategory::Six: return "Six";
        case IncidentCategory::Seven: return "Seven";
        case IncidentCategory::SevenB: return "SevenB";
        case IncidentCategory::SevenBM: return "SevenBM";
        case IncidentCategory::Eight: return "Eight";
        case IncidentCategory::EightA: return "EightA";
        case IncidentCategory::EightB: return "EightB";
        case IncidentCategory::EightC: return "EightC";
        case IncidentCategory::EightD: return "EightD";
        case IncidentCategory::EightE: return "EightE";
        case IncidentCategory::EightF: return "EightF";
        case IncidentCategory::EightG: return "EightG";
        case IncidentCategory::Nine: return "Nine";
        case IncidentCategory::Ten: return "Ten";
        case IncidentCategory::Eleven: return "Eleven";
        case IncidentCategory::ElevenA: return "ElevenA";
        case IncidentCategory::ElevenB: return "ElevenB";
        case IncidentCategory::Thirteen: return "Thirteen";
        case IncidentCategory::Fourteen: return "Fourteen";
        case IncidentCategory::Fifteen: return "Fifteen";
        case IncidentCategory::Sixteen: return "Sixteen";
        case IncidentCategory::Eighteen: return "Eighteen";
        case IncidentCategory::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

// to_string for EventType
inline std::string to_string(EventType type) {
    switch (type) {
        case EventType::Incident: return "Incident";
        case EventType::IncidentResolution: return "IncidentResolution";
        case EventType::ApparatusArrivalAtIncident: return "ApparatusArrivalAtIncident";
        case EventType::ApparatusReturnToStation: return "ApparatusReturnToStation";
        default: return "Unknown";
    }
}

// to_string for StationActionType
inline std::string to_string(StationActionType type) {
    switch (type) {
        case StationActionType::Dispatch: return "Dispatch";
        case StationActionType::DoNothing: return "DoNothing";
        default: return "Unknown";
    }
}

// to_string for IncidentLevel
inline std::string to_string(IncidentLevel level) {
    switch (level) {
        case IncidentLevel::Invalid: return "Invalid";
        case IncidentLevel::Low: return "Low";
        case IncidentLevel::Moderate: return "Moderate";
        case IncidentLevel::High: return "High";
        case IncidentLevel::Critical: return "Critical";
        default: return "Unknown";
    }
}

inline const char* to_string(IncidentStatus status) {
    switch (status) {
        case IncidentStatus::hasBeenRespondedTo:    return "hasBeenRespondedTo";
        case IncidentStatus::hasBeenReported:       return "hasBeenReported";
        case IncidentStatus::isBeingResolved:       return "isBeingResolved";
        case IncidentStatus::hasBeenResolved:       return "hasBeenResolved";
        default:                                    return "Invalid";
    }
}


inline const char* to_string(ApparatusType type) {
    switch (type) {
        case ApparatusType::Pumper: return "Pumper";
        case ApparatusType::Engine: return "Engine";
        case ApparatusType::Truck: return "Truck";
        case ApparatusType::Rescue: return "Rescue";
        case ApparatusType::Hazard: return "Hazard";
        case ApparatusType::Chief: return "Chief";
        case ApparatusType::Squad: return "Squad";
        case ApparatusType::Fast: return "Fast";
        case ApparatusType::Medic: return "Medic";
        case ApparatusType::Brush: return "Brush";
        case ApparatusType::Boat: return "Boat";
        case ApparatusType::UTV: return "UTV";
        case ApparatusType::Reach: return "Reach";
        default: return "Invalid";
    }
}
#endif // ENUMS_H
