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

## Building the Project
Install the required packages:
> `gdb` debugging does not work on current macOS devices.
```bash
# For macOS
brew install cmake
brew install curl
brew install googletest
xcode-select --install

# For unix
sudo apt update
sudo apt install cmake g++ libcurl4-openssl-dev libgtest-dev gdb
```

To build the project, navigate to the project directory and run the following command:

```bash
mkdir build
cd build
cmake ..
make
```

This will compile the source files and create an executable in the project's root folder.

## Running the Application
Modify the `pub.env` and change it to `.env`, update the paths and OSRM url.

Make sure that the `incidents.csv` look like this:
```csv
incident_id,lat,lon,incident_type,incident_level,datetime
1,36.005691,-86.73419,Road Closure,Low,2025-01-01 00:00:00
```
and the `stations.csv` look like this:
```
OBJECTID,Facility Name,Address,City,State,Zip Code,GLOBALID,lon,lat
1,Station 39,1247 South Dickerson Rd,Goodlettsvi,TN,37072,eac3496b-ab7d-4f6a-ad14-bf5ada67676a,-86.73860485,36.29107537
```

After building the project, you can run the application with the following command:
```
./run_simulator
# If you are still inside build folder
../run_simulator
# just make sure the paths in the .env reflect where you are
```

## Cleaning Up
To remove the compiled files and clean the project directory, use the command:

```
make clean
```

## Development
Don't forget to create test cases. Place them inside `test/` and run `./test_simulator` after make.
> `gdb` does not work on `arm`-based macOS devices right now. Use Linux to build and debug code.
