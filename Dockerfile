FROM python:3.10-slim

RUN apt-get update && apt-get install -y nodejs npm build-essential && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY client/package*.json ./client/
RUN cd client && npm install

COPY client/ ./client/
ARG REACT_APP_API_URL
ENV REACT_APP_API_URL=${REACT_APP_API_URL}
RUN cd client && npm run build

COPY server/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY server/ .

RUN mkdir -p /app/client/build && cp -r /app/client/build/* /app/client/build/ || true

ARG API_PORT=5000
ARG COLLECTOR_PORT=1884
ENV API_PORT=${API_PORT}
ENV COLLECTOR_PORT=${COLLECTOR_PORT}

EXPOSE ${API_PORT}
EXPOSE ${COLLECTOR_PORT}

RUN chmod +x start.sh
CMD ["./start.sh"]