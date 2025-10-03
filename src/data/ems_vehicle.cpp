// .h file
// Think of EMS as if its a station
/*
EMS_Vehicle::EMS_Vehicle() {
    private:
        StationID stationID;
        Location location;
        ApparatusStatus status;
}

// PSEUDOCODE


create stations (loader)
create EMS vehicles (separate from stations in the loader)


when an EMS incident occurs (ie. you need 1 EMS)
    2 step process
    1. 
    - dispatch policy: get stations to incident distances/durations. (41x1) - read from a matrix
        - this is only for suppression
    - select which station will dispatch suppression
    2. 
    - get all EMS that are ReturningToStation or Available
        - check their location (lat lon),
        - create a cost table from all EMS here to incident (E x 1)
    - select which EMS will go to incident

calculate service time
    - for whichever apparatus arrives first (suppresion or EMS) then get service time

when done:
    - suppresion goes back to station
    - EMS goes back to station
        - we get route from current location to home station
        - get duration to return
    - when next event comes in (new event time):
        - loop through all EMS (and maybe suppression)
        - check if the current time event time, is greater than their duration to return
            - their at back and available
        - if they are not:
            - check how far along they are to returning
            - get percent traveled back.
            - use that to check where you are -> update location and status (so we can see it in get action)


*/

