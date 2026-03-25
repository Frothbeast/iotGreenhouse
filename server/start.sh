#!/bin/bash

docker network inspect iot_network >/dev/null 2>&1 || \
    docker network create --driver bridge iot_network


python greenhouseCollector.py &
python greenhouseAPI.py