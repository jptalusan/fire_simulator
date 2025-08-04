#ifndef APPARATUS_H
#define APPARATUS_H

#include "enums.h"

struct Apparatus {
    // Constructors
    Apparatus() : id_(0), stationIndex_(0), status_(ApparatusStatus::Available), type_(ApparatusType::Engine) {}
    
    Apparatus(int id, int stationIndex, ApparatusType type)
        : id_(id), stationIndex_(stationIndex), status_(ApparatusStatus::Available), type_(type) {}

    // Getters (made noexcept for better performance)
    int getId() const noexcept { return id_; }
    int getStationIndex() const noexcept { return stationIndex_; }
    ApparatusStatus getStatus() const noexcept { return status_; }
    ApparatusType getType() const noexcept { return type_; }
    
    // Setters (made noexcept)
    void setStatus(ApparatusStatus status) noexcept { status_ = status; }
    void setStationIndex(int stationIndex) noexcept { stationIndex_ = stationIndex; }
    void setType(ApparatusType type) noexcept { type_ = type; }
    void setId(int id) noexcept { id_ = id; }

private:
    int id_;                    // 4 bytes
    int stationIndex_;          // 4 bytes
    ApparatusStatus status_;    // 1 byte (with uint8_t from your enums)
    ApparatusType type_;        // 1 byte (with uint8_t from your enums)
    // 2 bytes padding to align to 4-byte boundary
    // Total: 12 bytes
};

#endif // APPARATUS_H
