# Fire Simulator

## Overview
This project is a simple C++ application that take in incidents and stations in csv format and simulate dispatching of fire trucks to incidents.

## Folder Structure
```
fire_simulator
├── data
│   └── incidents.csv
│   └── stations.csv
├── src
│   └── folder/file.cpp
├── include
│   └── folder/file.h
├── CMakeLists.txt
├── .env
└── README.md
```

## Building Prerequisites on RHEL8

1. Install an updated `gcc-toolset-13` this will provide updated `gcc` and `g++`.
    ```bash
    sudo yum update
    dnf install gcc-toolset-13
    yum install libcurl-devel
    # add this to your bashrc
    vi ~/.bash_profile
    
    # Enable gcc-toolset-13 automatically (After the initial path= command)
    if [ -f /opt/rh/gcc-toolset-13/enable ]; then
        source /opt/rh/gcc-toolset-13/enable
    fi
    ```
2. Install `Python3.10` to get `cmake`
    ```bash
    yum install python3.11
    # You need to source activate this python first then install cmake.
    # We need cmake > v3.11 for everything
    /bin/python3.11 -m venv .venv
    source .venv/bin/activate
    pip install cmake

    # update profile
    sudo vi ~/.bash_profile
    # Add this line at the end (after the toolset steps above)
    PATH="$HOME/.venv/bin/cmake:$PATH”
    # Activate
    source ~/.bash_profile
    ```
3. Download, build, and install `boost v1.89` since the current `boost-devel` included in the `dnf` is outdated (v1.66) which will cause issues with our `onnxruntime` build later. [(Reference)](https://www.boost.org/doc/user-guide/getting-started.html)
    ```bash
    sudo yum install bzip2-devel zlib-devel libicu-devel
    
    # You can just extract it to your home directory
    cd ~
    # The link in the tutorial is broken, check releases page: https://www.boost.org/releases/1.89.0/
    wget https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.bz2
    tar xf boost_1_89_0.tar.bz2
    cd boost_1_89_0

    # Boostrap
    ./bootstrap.sh
    ./b2
    ./b2 install --prefix=/usr/local
    ```
4. Clone, build, and install `onnxruntime (v1.22.2)` to the `fire_simulator/externals` folder.
    ```bash
    cd ~
    git clone --recursive https://github.com/Microsoft/onnxruntime.git
    cd onnxruntime

    # Make sure to use this release, newer ones have a failure that no one resolved.
    git checkout v1.22.2

    # If you messed up and ran this without running the above command, just delete `build` folder.
    ./build.sh --config RelWithDebInfo --build_shared_lib --parallel --compile_no_warning_as_error --skip_submodule_sync --cmake_extra_defines

    cd build/Linux/RelWithDebInfo

    # This will install it to a path that has lib64 instead of lib, so adjust accordingly
    # Destination_Dir for our case is the fire_simulator/externals folder.
    make install DESTDIR=[DESTINATION_DIR]
    ```
5. Other packages such as `nlohmann-json`, `fmt`, and `spdlog` should be handled by the `CMakeLists.txt`

## Install and Setup up Docker
1. Install the `docker-ce` and `docker-ce-cli` and run it on systemctl.
    ```bash
    sudo dnf -y install dnf-plugins-core
    sudo dnf config-manager --add-repo https://download.docker.com/linux/rhel/docker-ce.repo
    sudo dnf install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
    # start docker engine, this should start docker when the system restarts.
    sudo systemctl enable --now docker
    ```
2. Pull the docker image needed
    ```bash
    cd ~/fire_simulator/docker
    curl -L "https://download.geofabrik.de/north-america/us/tennessee-latest.osm.pbf" -o ./data/osm.pbf
    touch ./data/speeds.csv
    sudo docker build --platform=linux/amd64 --no-cache --tag tn_osrm:ems -f Dockerfile .
    sudo docker run -d 
        --name ems_osrm -m=4g \
        --restart unless-stopped \
        -p 8085:8085 \
        tn_osrm:ems
    ```

## Building `fire_simulator`
1. Clone the repo and build
    ```bash
    cd ~
    git clone git@github.com:jptalusan/fire_simulator.git
    cd fire_simulator
    git checkout rhel_instructions
    cmake -S . -B build && cmake --build build
    ```
2. Before running the application, make sure all data required are present and correct (based on features)

## Running the Application
Modify the `pub.env` and change it to `.env`, update the paths and OSRM url.

1. Make sure that the `incidents.csv` look like this:
```csv
incident_id,lat,lon,incident_type,incident_level,datetime,category
0,36.005691,-86.73419,Road Closure,Low,2025-01-01 00:00:00,Nine
```
it should be 0th indexed without any missing indices in the middle.

1. and the `stations.csv` look like this: (and stations_with_apapratus.csv)
```
StationID,Stations,lat,lon,Address,Engine_ID,Truck,Rescue,Hazard,Squad,FAST,Medic,Brush,Boat,UTV,REACH,Chief
0,Station 1,36.2293898,-86.75674762,130 Broadmoor Avenue,1,,1,,1,,,,,,,
```
it should be 0th  indexed without any missing indices in the middle.

1. A third file, `bounds.geojson` with a single polygon, defines the bounds of the system. If a point in incidents or stations is not within the boundary, it is ignored.
Right now, if you don't have a bounds.geojson it will not check any point.

1. Supplementary files can be generated by running `scripts/preprocess_to_generate_data.py` a list of required files for this script are:
    * `FIRE RUN CARDS OCT 2024`
    * `FireBeats_shapefile_05152025`


2. while inside build directory
    ```bash
    cd ~/fire_simulator/build
    ./src/fire_simulator --env PATH_TO_ENV
    ```

## Cleaning Up
To remove the compiled files and clean the project directory, use the command:

```
make clean
```

## Development
Don't forget to create test cases. Place them inside `test/` and run `./test_simulator` after make.

```bash
# Run only nearest dispatch tests
./tests/unit_tests --gtest_filter="NearestDispatchTest.*"

# Run only location tests
./tests/unit_tests --gtest_filter="LocationTest.*"

# Running all tests, while in build dir
ctest
```

### Precomputing Travel times

Policies that rely on travel time matrices use `OSRM`. To save time, we query the service prior to execution. This then creates 2 binary files:
1. `logs/distance_matrix.bin`
2. `logs/duration_matrix.bin`

### Dispatch Policies
Current behavior: incidents are served on a **first-come, first-serve** basis. it will only be considered resolved when all required apparatus is met.
Different dispatch policies affect which apparatus are sent to an incident.
* Nearest: Send the nearest fire trucks based on travel time.
* FireBeats: Send the fire trucks based on fire beats sequence.
    1. FireBeats_shapefile_05152025: Shape file of service zones.
    2. FIRE RUN CARDS OCT 2024: A collection of fire beat excel sheets for each service zone.
    Fire beats require preprocessing of these two files. The preprocessing code is included in `preprocess.ipynb`. This will generate a file `logs/beats.bin` that detail the different beats per service zone. And a file called `beats_shpfile.geojson` which are used during precmputing to identify the service zone an incident fall under.

### Summary of required files/services
1. `OSRM`: For generating routing solutions and travel time matrices. I have included a `Docker` container.
1. `incidents.csv`: Can be synthetically generated by `preprocess.ipynb` given a bounds.
    > Note index is internally tracked IDs, while id is what is written in the csv input. Same with the station index and ID.
2. `stations.csv`: Collected from [data.nashville.gov](https://data.nashville.gov/datasets/Nashville::fire-stations/explore?location=36.180107%2C-86.792213%2C10.04&showTable=true) but needs to be preprocessed into the correct WSG:84 projection.
3. `bounds.geojson`
4. `beats_shpfile.geojson`: Generated by `preprocess.ipynb`
5. `beats.bin`: Generated by `preprocess.ipynb`

### Summary of output files
1. `output.log`
2. `station_report.csv`: Mapping of stations to incidents. The same incidents can be mapped to different stations (if they all sent to the incident).
3. `incident_report.csv`: All metrics per incident, mostly timing related.

## TODO:
1. ~~Switch from vector of events to Priority Queue~~
2. ~~Clean up incident and station, remove function calls inside, put in a separate standalone function file.~~
3. ~~Clean up activeIncidents_, it should just be a priority queue (or even just a queue?)~~
5. ~~Create a separate map of incidents. that i just look up O(1) when i need information about the incident. dont add them in the event.~~
4. Change incidents so no 2 incidents have the same time (have at least a second of difference).
6. Maybe categorize medic as a :moving" fire Stations
