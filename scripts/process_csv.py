#!/usr/bin/env python3

import pandas as pd
import geopandas as gpd
import os
from dotenv import load_dotenv
import requests
import time
import json
import numpy as np

def get_osrm_route(start_lon, start_lat, end_lon, end_lat):
    """
    Get route geometry from OSRM between two points.
    
    Args:
        start_lon, start_lat: Starting coordinates (station)
        end_lon, end_lat: Ending coordinates (incident)
        osrm_url: OSRM server URL
    
    Returns:
        dict: Contains geometry, duration, distance, or None if failed
    """
    osrm_url = os.getenv("BASE_OSRM_URL", "../logs/")  # Default fallback if not in .env
    try:
        # OSRM expects coordinates in lon,lat format
        url = f"{osrm_url}/route/v1/driving/{start_lon},{start_lat};{end_lon},{end_lat}"
        params = {
            'geometries': 'geojson',  # Get geometry as GeoJSON
            'overview': 'full'        # Get full geometry
        }
        
        response = requests.get(url, params=params, timeout=10)
        
        if response.status_code == 200:
            data = response.json()
            if data.get('routes'):
                route = data['routes'][0]
                return {
                    'geometry': route['geometry'],
                    'duration': route['duration'],  # seconds
                    'distance': route['distance']   # meters
                }
        
        print(f"OSRM error: {response.status_code} - {response.text}")
        return None
        
    except Exception as e:
        print(f"Error getting route: {e}")
        return None

def add_routes_to_dataframe(df, delay=0.1):
    """
    Add OSRM route data to each row in the dataframe.
    
    Args:
        df: DataFrame with station_lat, station_lon, incident_lat, incident_lon columns
        delay: Delay between requests to avoid overwhelming OSRM server
    
    Returns:
        DataFrame with added route_geometry, route_duration, route_distance columns
    """
    routes = []
    
    print(f"Getting routes for {len(df)} records...")
    
    for idx, row in df.iterrows():
        if idx % 10 == 0:  # Progress indicator
            print(f"Processing route {idx + 1}/{len(df)}")
        
        route_data = get_osrm_route(
            row['station_lon'], row['station_lat'],
            row['incident_lon'], row['incident_lat']
        )
        
        if route_data:
            routes.append({
                'route_geometry': route_data['geometry'],
                'route_duration': route_data['duration'],
                'route_distance': route_data['distance']
            })
        else:
            routes.append({
                'route_geometry': None,
                'route_duration': None,
                'route_distance': None
            })
        
        # Add delay to avoid overwhelming the server
        if delay > 0:
            time.sleep(delay)
    
    # Add route data to dataframe
    routes_df = pd.DataFrame(routes)
    return pd.concat([df, routes_df], axis=1)

def process():
    env_path = os.path.join(os.path.dirname(__file__), '../.env')
    load_dotenv(dotenv_path=env_path)

    print(os.getcwd())
    # Now you can access environment variables
    INCIDENTS_CSV_PATH = os.getenv("INCIDENTS_CSV_PATH", "../logs/")  # Default fallback if not in .env
    STATIONS_CSV_PATH = os.getenv("STATIONS_CSV_PATH", "../data/")  # Default fallback if not in .env
    STATION_REPORT_CSV_PATH = os.getenv("STATION_REPORT_CSV_PATH", "../logs/station_report.csv")
    INCIDENT_REPORT_CSV_PATH = os.getenv("INCIDENT_REPORT_CSV_PATH", "../logs/incident_report.csv")
    stations_df = pd.read_csv(STATIONS_CSV_PATH)
    stations_df = stations_df[['StationID', 'Facility Name', 'lat', 'lon']]

    incidents_df = pd.read_csv(INCIDENTS_CSV_PATH)
    incidents_df = incidents_df[['incident_id', 'lat', 'lon', 'datetime', 'incident_level']]
    # TODO: Change incident_level to some numerical value

    station_report_df = pd.read_csv(STATION_REPORT_CSV_PATH)
    incident_report_df = pd.read_csv(INCIDENT_REPORT_CSV_PATH)
    incident_report_df = incident_report_df[['IncidentID', 'Resolved']]

    merged = station_report_df.merge(stations_df, on='StationID')
    merged = merged.rename(columns={"lat":"station_lat", "lon":"station_lon"})

    merged = merged.merge(incidents_df, left_on='IncidentID', right_on='incident_id')
    merged = merged.rename(columns={"lat":"incident_lat", "lon":"incident_lon"})

    merged = merged.merge(incident_report_df, on='IncidentID')

    merged['Resolved'] = pd.to_datetime(merged['Resolved'])

    duplicated_df = []
    for _, row in merged.iterrows():
        travel_time_s = row['TravelTime']
        resolved_time = row['Resolved']
        duplicated_df.append(row)
        return_row = row.copy()
        return_row['station_lat'] = row['incident_lat']
        return_row['station_lon'] = row['incident_lon']
        return_row['incident_lat'] = row['station_lat']
        return_row['incident_lon'] = row['station_lon']
        return_row['DispatchTime'] = row['Resolved']
        duplicated_df.append(return_row)
        pass
    duplicated_df = pd.concat(duplicated_df, axis=1).T
    duplicated_df = duplicated_df.reset_index(drop=True)

    # # Add route data using OSRM
    duplicated_df = add_routes_to_dataframe(duplicated_df, delay=0)
    duplicated_df['datetime'] = pd.to_datetime(duplicated_df['datetime'])
    duplicated_df['DispatchTime'] = pd.to_datetime(duplicated_df['DispatchTime'])
    
    print(duplicated_df[['DispatchTime', 'station_lat', 'station_lon', 'incident_lat', 'incident_lon', 'Resolved', 'StationID', 'IncidentID', 'datetime']])
    print(duplicated_df.shape)

    
    express_geo_json = dict(type="FeatureCollection", features=[])

    for _stationID, station_df in duplicated_df.groupby("StationID"):
        feature = dict(
            type="Feature", geometry=None, properties=dict(station_id=str(_stationID))
        )
        data = []
        timestamp = station_df.iloc[0]['DispatchTime']
        timestamp = timestamp.timestamp()
        for j, w in station_df.iterrows():
            geom = w["route_geometry"]
            coords = geom['coordinates']
            duration = w["route_duration"]
            coords = np.array(coords)
            coords = coords[::10]
            time_per_seg = duration / len(coords)
            
            for coord in coords:
                timestamp += time_per_seg
                data.append([coord[0], coord[1], 0, int(timestamp)])
                # Create a new feature per row
            feature["geometry"] = dict(type="LineString", coordinates=data)
        express_geo_json["features"].append(feature)

    # save geojson as json
    try:
        output_path = "../logs/routes.json"
        with open(output_path, 'w') as f:
            json.dump(express_geo_json, f)
        print(f"Saved GeoJSON as JSON to {output_path}")
    except Exception as e:
        print(f"Error saving JSON: {e}")
    

if __name__ == "__main__":
    print("Processing CSV files...")
    process()