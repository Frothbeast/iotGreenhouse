# 1. Use a Python base image
FROM python:3.10-slim

# 2. Set the working directory inside the container
WORKDIR /app

# System dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# 3. Copy only requirements first
COPY server/requirements.txt .

# 4. Install dependencies
RUN pip install --no-cache-dir -r requirements.txt [cite: 5]

# 5. Copy the rest of the application code
COPY server/ .

# Ensure the entrypoint script is executable
RUN chmod +x start.sh

# 6. Expose the API port (5000) and Collector port (1883)
EXPOSE 5000
EXPOSE 1883 [cite: 3]

# 7. Start both the Collector and the API
CMD ["./start.sh"]