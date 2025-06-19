FROM linusmotu/osrm-more-alt-linux:transitwebapp
WORKDIR /

ENV PORT=8085
# ARG SPEED_FILE
# ARG PBF_FILE

COPY data/osm.pbf /media/location_file.osm.pbf
COPY data/speeds.csv /media/speed_file.csv
COPY data/emergency.lua /opt/emergency.lua

RUN /usr/local/bin/osrm-extract -p /opt/emergency.lua /media/location_file.osm.pbf
RUN /usr/local/bin/osrm-partition  /media/location_file.osrm
RUN /usr/local/bin/osrm-customize  /media/location_file.osrm

RUN /usr/local/bin/osrm-contract /media/location_file.osrm --segment-speed-file /media/speed_file.csv

EXPOSE ${PORT}
# Allow larger table query sizes
RUN echo "/usr/local/bin/osrm-routed --port ${PORT} --algorithm mld --max-alternatives 10 --max-matching-size 10000 --max-viaroute-size 100 --max-trip-size 10000 --max-nearest-size 10000 --max-table-size 10000 /media/location_file.osrm" > start.sh \
    && chmod u+x start.sh

CMD ["sh", "-c", "./start.sh" ]