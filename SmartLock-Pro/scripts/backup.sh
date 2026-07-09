#!/bin/bash
# SmartLock Pro - Backup Script

BACKUP_DIR="${BACKUP_DIR:-/backup/smartlock}"
DATE=$(date +%Y%m%d_%H%M%S)
RETENTION_DAYS=30

echo "=========================================="
echo "  SmartLock Pro - Backup"
echo "=========================================="

# Create backup directory
mkdir -p $BACKUP_DIR

# Backup database
echo "Backing up database..."
pg_dump -U smartlock -d smartlock > "$BACKUP_DIR/db_$DATE.sql"

# Backup configuration
echo "Backing up configuration..."
tar -czf "$BACKUP_DIR/config_$DATE.tar.gz" /opt/smartlock/config/

# Backup logs
echo "Backing up logs..."
tar -czf "$BACKUP_DIR/logs_$DATE.tar.gz" /opt/smartlock/logs/

# Backup MQTT data
echo "Backing up MQTT data..."
tar -czf "$BACKUP_DIR/mqtt_$DATE.tar.gz" /var/lib/mosquitto/

# Cleanup old backups
echo "Cleaning up old backups..."
find $BACKUP_DIR -name "*.sql" -mtime +$RETENTION_DAYS -delete
find $BACKUP_DIR -name "*.tar.gz" -mtime +$RETENTION_DAYS -delete

echo "Backup complete: $BACKUP_DIR/smartlock_$DATE"
