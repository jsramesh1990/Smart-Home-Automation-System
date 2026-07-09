# SmartLock Pro - Raspberry Pi Dockerfile
# Multi-stage build for ARM architecture

# Stage 1: Builder
FROM python:3.9-slim-bullseye AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    make \
    libffi-dev \
    libssl-dev \
    libjpeg-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy requirements first for better caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Stage 2: Final image
FROM python:3.9-slim-bullseye

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    mosquitto \
    mosquitto-clients \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r smartlock && useradd -r -g smartlock smartlock

# Set working directory
WORKDIR /app

# Copy from builder
COPY --from=builder /usr/local/lib/python3.9/site-packages /usr/local/lib/python3.9/site-packages
COPY --from=builder /usr/local/bin /usr/local/bin

# Copy application files
COPY src/ ./src/
COPY config/ ./config/
COPY scripts/ ./scripts/
COPY static/ ./static/
COPY templates/ ./templates/

# Set ownership
RUN chown -R smartlock:smartlock /app

# Expose ports
EXPOSE 1883 8883 5000 5001 8080 8000

# Health check
HEALTHCHECK --interval=30s --timeout=3s \
    CMD curl -f http://localhost:5000/health || exit 1

# Switch to non-root user
USER smartlock

# Start the application
CMD ["python", "-m", "src.main"]
