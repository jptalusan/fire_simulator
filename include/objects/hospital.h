#ifndef HOSPITAL_H
#define HOSPITAL_H

#include "objects/common.h"
#include <string>

class Hospital {
public:
    Hospital() :
        index_(-1),
        id_(""),
        name_(""),
        location_()
    {}

    Hospital(int index, const std::string& id, const Location& location,
             const std::string& name = "", int visitCount = 0) :
        index_(index),
        id_(id),
        name_(name),
        location_(location),
        visitCount_(visitCount)
    {}

    int getIndex() const noexcept { return index_; }
    const std::string& getId() const noexcept { return id_; }
    const std::string& getName() const noexcept { return name_; }
    const Location& getLocation() const noexcept { return location_; }
    int getVisitCount() const noexcept { return visitCount_; }

private:
    int index_;
    std::string id_;
    std::string name_;
    Location location_;
    int visitCount_;  // Historical visit count for weighted selection
};

#endif // HOSPITAL_H
