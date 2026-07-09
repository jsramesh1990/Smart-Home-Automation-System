#!/bin/bash
# SmartLock Pro - Setup Script

set -e

echo "=========================================="
echo "  SmartLock Pro - Raspberry Pi Setup"
echo "=========================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${YELLOW}Please run as root (sudo)${NC}"
    exit 1
fi

# Detect system
echo -e "${GREEN}Detecting system...${NC}"
if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "OS: $NAME $VERSION"
else
    echo "Unknown OS"
    exit 1
fi

# Install dependencies
echo -e "${GREEN}Installing dependencies...${NC}"
apt-get update
apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    mosquitto \
    mosquitto-clients \
    postgresql \
    postgresql-contrib \
    redis-server \
    nginx \
    git \
    curl \
    wget \
    build-essential \
    libssl-dev \
    libffi-dev \
    libjpeg-dev \
    zlib1g-dev

# Setup Python virtual environment
echo -e "${GREEN}Setting up Python environment...${NC}"
cd /opt
mkdir -p smartlock
cd smartlock
python3 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# Setup database
echo -e "${GREEN}Setting up database...${NC}"
sudo -u postgres psql -c "CREATE USER smartlock WITH PASSWORD 'SmartLock2026!';" 2>/dev/null || true
sudo -u postgres psql -c "CREATE DATABASE smartlock OWNER smartlock;" 2>/dev/null || true
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE smartlock TO smartlock;" 2>/dev/null || true

# Initialize database
python3 -c "from src.database import Base, engine; Base.metadata.create_all(engine)"

# Setup MQTT
echo -e "${GREEN}Setting up MQTT broker...${NC}"
mkdir -p /etc/mosquitto/conf.d
mkdir -p /var/log/mosquitto
chown mosquitto:mosquitto /var/log/mosquitto

# Copy MQTT config
cp config/mosquitto/mosquitto.conf /etc/mosquitto/conf.d/smartlock.conf

# Create MQTT users
mosquitto_passwd -c /etc/mosquitto/passwd admin 2>/dev/null || true
mosquitto_passwd /etc/mosquitto/passwd device 2>/dev/null || true
mosquitto_passwd /etc/mosquitto/passwd mobile 2>/dev/null || true
mosquitto_passwd /etc/mosquitto/passwd dashboard 2>/dev/null || true

# Setup service
echo -e "${GREEN}Creating systemd service...${NC}"
cat > /etc/systemd/system/smartlock.service << EOF
[Unit]
Description=SmartLock Pro Backend Service
After=network.target postgresql.service mosquitto.service redis-server.service

[Service]
Type=simple
User=smartlock
WorkingDirectory=/opt/smartlock
Environment="PATH=/opt/smartlock/venv/bin"
Environment="PYTHONUNBUFFERED=1"
ExecStart=/opt/smartlock/venv/bin/python -m src.main
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Create user
useradd -r -s /bin/false smartlock 2>/dev/null || true
chown -R smartlock:smartlock /opt/smartlock

# Setup nginx
echo -e "${GREEN}Setting up nginx...${NC}"
cat > /etc/nginx/sites-available/smartlock << EOFserver {
    listen 80;
    server_name _;
    
    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
    }
    
    location /socket.io/ {
        proxy_pass http://127.0.0.1:5001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
    }
    
    location /static/ {
        alias /opt/smartlock/static/;
    }
}
EOF

ln -sf /etc/nginx/sites-available/smartlock /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default

# Start services
echo -e "${GREEN}Starting services...${NC}"
systemctl daemon-reload
systemctl enable mosquitto
systemctl start mosquitto
systemctl enable postgresql
systemctl start postgresql
systemctl enable redis-server
systemctl start redis-server
systemctl enable smartlock
systemctl start smartlock
systemctl enable nginx
systemctl restart nginx

echo -e "${GREEN}==========================================${NC}"
echo -e "${GREEN}  SmartLock Pro Setup Complete!${NC}"
echo -e "${GREEN}==========================================${NC}"
echo ""
echo -e "Services started:"
echo -e "  - MQTT Broker: port 1883"
echo -e "  - PostgreSQL: port 5432"
echo -e "  - Redis: port 6379"
echo -e "  - Web Server: port 80"
echo -e "  - Backend API: port 5000"
echo ""
echo -e "${YELLOW}Default credentials:${NC}"
echo -e "  Admin: admin / SmartLock2026!"
echo ""
echo -e "Check status: systemctl status smartlock"
echo -e "View logs: journalctl -u smartlock -f"
echo ""
echo -e "${GREEN}Installation complete!${NC}"
