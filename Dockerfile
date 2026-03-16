# 1. Use a Python base image
FROM python:3.10-slim

# 2. Set the working directory inside the container
WORKDIR /app

# New Correction: Added pkg-config to the system dependencies
RUN apt-get update && apt-get install -y \
    default-libmysqlclient-dev \
    pkg-config \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# 3. Copy only requirements first
COPY server/requirements.txt .

# 4. Install dependencies
RUN pip install --no-cache-dir -r requirements.txt

# 5. Copy the rest of the application code
COPY server/ .

# 6. Expose the port
EXPOSE 5000

# 7. Start the application
CMD ["python", "greenhouseAPI.py"]
