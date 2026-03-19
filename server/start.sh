#!/bin/bash

# Start the collector in the background
python greenhouseCollector.py &

# Start the API in the foreground
python greenhouseAPI.py