# 1. Use a Python base image
FROM python:3.10-slim

# 2. Set the working directory
WORKDIR /app

# Define Build Arguments (passed from docker-compose)
ARG API_PORT
ARG COLLECTOR_PORT

# Set Environment Variables inside the container
ENV API_PORT=${API_PORT:-5000}
ENV COLLECTOR_PORT=${COLLECTOR_PORT:-1884}

# System dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# 3. Copy requirements and install
COPY server/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 4. Copy application code
COPY server/ .
RUN chmod +x start.sh

# 5. Use the variables for EXPOSE
# Note: EXPOSE technically only accepts numeric constants in some versions, 
# but defining them as ENV here makes them available to your scripts.
EXPOSE ${API_PORT}
EXPOSE ${COLLECTOR_PORT}

# 6. Start the application
CMD ["./start.sh"]