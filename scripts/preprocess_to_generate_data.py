# This is for preprocessing and creating beats.bin
# This will be used for firebeats.

import geopandas as gpd
import os
import pandas as pd
from shapely.geometry import Point
import re
import struct
import numpy as np


def split_string(s):
    match = re.match(r'^(\d+)(.*)$', s)
    if match:
        number = int(match.group(1))
        suffix = match.group(2) if match.group(2) else None
        return number, suffix
    else:
        raise ValueError(f"Invalid input: {s}")

# Suppose 'union_polygon' may be a MultiPolygon or Polygon
def split_to_polygons(geom):
    if geom.geom_type == 'MultiPolygon':
        return list(geom.geoms)
    elif geom.geom_type == 'Polygon':
        return [geom]
    else:
        return []

# We need to address this based on Kelly's workflow.
ignored_names = [
    "FD/RADIO",
    "FD/FST11",
    "FD/FST06",
    "FD/FST09",
    "FD/FST37",
    "FD/FAST01",
    "FD/SQ01",
    "FD/SQ37",
    "FD/SQ09",
    "FD/SQ11",
    "FD/TC05",
    "FD/DS31",
    "FD/MED41",
    "FD/HQ",
    "FD/BAR*",
    "FD/DSOP",
    "FD/BT13", "FD/BT22", "FD/BT35", "FD/BT36"
]

DATA_DIR = "./data"

if __name__ == "__main__":
    stations_df = pd.read_csv(os.path.join(DATA_DIR, "stations.csv"))
    
    df = gpd.read_file("data/FireBeats_shapefile_05152025")
    zones = df['NAME'].unique()
    zones = sorted(zones)

    # Read the Excel file and specify the sheet name (or use sheet number)
    excel_path = "data/FIRE RUN CARDS OCT 2024/"
    sheet_name = "RunCardOrder"  # Change to your actual sheet name

    beats = []
    max_runs = 0
    ############################################################
    ############### Create zones.csv (dictionary of zone to id) ###############
    ############################################################
    for zone_name in zones:

        i_name, s_name = split_string(zone_name)
        # print(i_name, s_name)
        if s_name is not None:
            new_name = f"{str(i_name).zfill(2)}{s_name}"
        else:
            new_name = str(i_name).zfill(2)
        # Construct the full path to the Excel file for the specific zone
        excel_path = f"data/FIRE RUN CARDS OCT 2024/BEAT-{new_name}.xlsx"
        
        # Check if the file exists before proceeding
        if not os.path.exists(excel_path):
            print(f"File {excel_path} does not exist. Skipping.")
            continue
        
        # Read the sheet into a DataFrame
        df = pd.read_excel(excel_path, sheet_name=sheet_name)
        # Confirm these with Fire Department
        # Again update this based on Kelly's workflow
        df = df[~df['OrderValue'].isin(ignored_names)]
        df = df[~df['OrderValue'].str.contains("FST")]
        df = df[~df['OrderValue'].str.contains("FB")]
        df = df[~df['OrderValue'].str.contains("SQ")]
        df = df[~df['OrderValue'].str.contains("MED")]
        df = df[~df['OrderValue'].str.contains("BT")]
        df = df[~df['OrderValue'].str.contains("DSOP")]
        df = df[~df['OrderValue'].str.contains("RADIO")]
        df = df[~df['OrderValue'].str.contains("HQ")]
        df = df[~df['OrderValue'].str.contains("EN")]
        df = df[~df['OrderValue'].str.contains("RE")]
        df = df[~df['OrderValue'].str.contains("ATV")]
        df = df[~df['OrderValue'].str.contains("TC")]
        df = df[~df['OrderValue'].str.contains("BAR")]
        df['Facility Name'] = df['OrderValue'].str.split('/').str[1]
        try:
            df['Facility Name'] = df['Facility Name'].str.replace('S', 'Station ')
            df['StationID'] = df['Facility Name'].str.split(' ').str[1]
            df['StationID'] = df['StationID'].astype('int')
            df['Facility Name'] = 'Station ' + df['StationID'].astype(str).str.zfill(2)
            df.drop(columns=['StationID'], inplace=True)
        except Exception as e:
            print(new_name, e)
            # display(df.head())

        max_runs = max(max_runs, df['OrderValue'].shape[0])
        run_order = df['Facility Name'].tolist()
        if len(run_order) < max_runs:
            run_order += ["None"] * (max_runs - len(run_order))
        run_order = [new_name] + run_order
        beats.append(run_order)
    df = pd.DataFrame(beats, columns=['Zone'] + [f'Run {i+1}' for i in range(max_runs)])
    # replace all '' in df with 'None'
    df = df.replace('', 'None')
    df = df.fillna('None')
    for i in range(len(beats)):
        # print(beats[i])
        if len(beats[i]) < max_runs:
            beats[i] += ["None"] * (max_runs - len(beats[i]))
        # print(len(beats[i]))
    df.to_csv("data/beats.csv", index=False)

    # map the values in df to StationID in stations df. so Station 01 becomes 0, Station 02 becomes 1, etc.
    # 1. Create a mapping from facility name to StationID
    station_map = dict(zip(stations_df['Facility Name'], stations_df['StationID']))
    station_map['Station 02'] = 1 # Station 2 merged with 3

    # 2. Replace all station names in df (except for the 'Zone' column)
    df.iloc[:, 1:] = df.iloc[:, 1:].replace(station_map)
    df.replace('None', -1, inplace=True)
    df = df.reset_index(drop=False)
    zones_df = df[["index", "Zone"]].copy()
    zones_df.rename(columns={"index": "ZoneID", "Zone":"Zone Name"}, inplace=True)
    zones_df.to_csv("data/zones.csv", index=False)

    zones_to_stations_df = df.drop(columns=["Zone", "index"], errors='ignore')
    zones_to_stations_df = zones_to_stations_df.T
    print(zones_to_stations_df.head())
    print(zones_to_stations_df.shape)
    ############################################################
    ############### Create beats.bin ###############
    ############################################################
    # Save to binary file
    def save_array_to_binary(filename, array):
        height, width = array.shape
        with open(filename, 'wb') as f:
            # Write width and height as 32-bit integers
            f.write(struct.pack('ii', width, height))
            # Flatten array in row-major order and write the data
            f.write(array.astype(np.int32).tobytes(order='C'))

    # Usage

    def save_string_matrix(filename, matrix):
        with open(filename, "wb") as f:
            height = len(matrix)
            width = len(matrix[0]) if height > 0 else 0
            f.write(struct.pack("ii", width, height))

            for row in matrix:
                for s in row:
                    s_bytes = s.encode("utf-8")
                    f.write(struct.pack("i", len(s_bytes)))
                    f.write(s_bytes)
    # for strings
    # matrix = df.to_numpy().tolist()
    # save_string_matrix("logs/beats.bin", matrix)

    # for integers
    print(zones_to_stations_df.to_numpy().shape)
    arr = zones_to_stations_df.to_numpy().astype(np.int32)
    print(arr)

    save_array_to_binary("data/beats.bin", arr)

    ############################################################
    ############### Create beats_shpfile.geojson ###############
    ############################################################
    zone_map = dict(zip(zones_df['Zone Name'], zones_df['ZoneID']))
    df = gpd.read_file("data/FireBeats_shapefile_05152025")
    # Convert to 4326
    df = df.to_crs(epsg=4326)
    df_arr = []
    for k, v in df.groupby("NAME"):
        union_polygon = v.union_all()
        # split the name into int and string components

        i_name, s_name = split_string(k)
        if s_name is not None:
            name = f"{str(i_name).zfill(2)}{s_name}"
        else:
            name = str(i_name).zfill(2)
        polygons = split_to_polygons(union_polygon)
        for poly in polygons:
            data = {
                "NAME": name,
                "ZONE_ID": zone_map.get(name, -1),
                "ZONE": v["ZONE"].iloc[0],
                "TYPE": v["TYPE"].iloc[0],
                "geometry": poly
            }
            df_arr.append(data)
    gdf = gpd.GeoDataFrame(df_arr, crs="EPSG:4326")
    gdf.to_file("data/beats_shpfile.geojson", driver="GeoJSON")

    ############################################################
    ############### Create bounds.geojson ###############
    ############################################################
    rect = gdf.union_all().minimum_rotated_rectangle
    rect_gdf = gpd.GeoDataFrame(geometry=[rect], crs=gdf.crs)
    rect_gdf.to_file("data/bounds.geojson", driver="GeoJSON")