# 1. Use a Python base image
FROM python:3.10-slim

# 2. Set the working directory inside the container
WORKDIR /app

# New Correction: Simplified system dependencies (mysql-connector-python doesn't need C-headers)
RUN apt-get update && apt-get install -y \
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
# New Correction: Ensure host is 0.0.0.0 in greenhouseAPI.py as discussed
CMD ["python", "greenhouseAPI.py"]
