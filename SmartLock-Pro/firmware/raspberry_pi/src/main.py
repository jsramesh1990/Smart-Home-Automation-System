#!/usr/bin/env python3
"""
SmartLock Pro - Raspberry Pi Main Application
Complete backend service with MQTT, API, and automation
"""

import os
import sys
import signal
import logging
import threading
import time
from datetime import datetime
from typing import Dict, Any

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from flask import Flask, jsonify, request, send_file
from flask_cors import CORS
from flask_socketio import SocketIO, emit
from flask_jwt_extended import JWTManager, create_access_token, jwt_required, get_jwt_identity
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

from src.mqtt_broker import MQTTBroker
from src.database import DatabaseManager
from src.automation_engine import AutomationEngine
from src.api_server import APIServer
from src.notification_service import NotificationService
from src.device_manager import DeviceManager
from src.logger import setup_logging

# Setup logging
logger = setup_logging()

# Initialize Flask app
app = Flask(__name__)
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', 'dev-secret-key-change-in-production')
app.config['JWT_SECRET_KEY'] = os.getenv('JWT_SECRET_KEY', 'jwt-secret-key-change-in-production')
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv('DATABASE_URL', 'sqlite:///smartlock.db')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

# Initialize extensions
CORS(app, resources={r"/api/*": {"origins": "*"}})
socketio = SocketIO(app, cors_allowed_origins="*")
jwt = JWTManager(app)
limiter = Limiter(app, key_func=get_remote_address)

# Initialize services
db_manager = DatabaseManager(app)
mqtt_broker = MQTTBroker()
automation_engine = AutomationEngine(db_manager, mqtt_broker)
notification_service = NotificationService()
device_manager = DeviceManager(db_manager, mqtt_broker)

# Initialize API server
api_server = APIServer(app, db_manager, mqtt_broker, automation_engine, notification_service, device_manager)

# ============================================================
# APPLICATION ROUTES
# ============================================================

@app.route('/')
def index():
    """Main page"""
    return jsonify({
        'service': 'SmartLock Pro Backend',
        'version': '1.0.0',
        'status': 'running',
        'timestamp': datetime.now().isoformat()
    })

@app.route('/health')
def health_check():
    """Health check endpoint"""
    status = {
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'services': {
            'database': db_manager.check_connection(),
            'mqtt': mqtt_broker.is_connected(),
            'redis': True,  # Placeholder
            'influxdb': True  # Placeholder
        },
        'system': {
            'cpu': get_cpu_usage(),
            'memory': get_memory_usage(),
            'uptime': get_uptime()
        }
    }
    return jsonify(status)

@app.route('/api/status')
def get_system_status():
    """Get system status"""
    return jsonify({
        'status': 'online',
        'devices': device_manager.get_device_count(),
        'users': db_manager.get_user_count(),
        'events_today': db_manager.get_events_today(),
        'uptime': get_uptime()
    })

# ============================================================
# WEBSOCKET EVENTS
# ============================================================

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    logger.info(f"Client connected: {request.sid}")
    socketio.emit('connected', {'status': 'success'})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    logger.info(f"Client disconnected: {request.sid}")

@socketio.on('subscribe')
def handle_subscribe(data):
    """Handle subscription to events"""
    topic = data.get('topic')
    if topic:
        socketio.enter_room(request.sid, topic)
        logger.info(f"Client {request.sid} subscribed to {topic}")

@socketio.on('unsubscribe')
def handle_unsubscribe(data):
    """Handle unsubscription from events"""
    topic = data.get('topic')
    if topic:
        socketio.leave_room(request.sid, topic)
        logger.info(f"Client {request.sid} unsubscribed from {topic}")

# ============================================================
# SYSTEM FUNCTIONS
# ============================================================

def get_cpu_usage():
    """Get CPU usage percentage"""
    try:
        import psutil
        return psutil.cpu_percent(interval=1)
    except:
        return 0

def get_memory_usage():
    """Get memory usage"""
    try:
        import psutil
        memory = psutil.virtual_memory()
        return {
            'total': memory.total,
            'available': memory.available,
            'used': memory.used,
            'percent': memory.percent
        }
    except:
        return {}

def get_uptime():
    """Get system uptime"""
    try:
        import psutil
        return psutil.boot_time()
    except:
        return time.time()

# ============================================================
# BACKGROUND TASKS
# ============================================================

def background_tasks():
    """Run background tasks"""
    while True:
        try:
            # Update device status
            device_manager.update_all_devices()
            
            # Process automation rules
            automation_engine.process_rules()
            
            # Clean up old logs
            db_manager.cleanup_old_logs(days=30)
            
            # Send heartbeat
            mqtt_broker.publish_heartbeat()
            
            time.sleep(60)  # Run every minute
        except Exception as e:
            logger.error(f"Background task error: {e}")
            time.sleep(10)

def mqtt_worker():
    """MQTT worker thread"""
    mqtt_broker.connect()
    mqtt_broker.loop_forever()

# ============================================================
# SIGNAL HANDLING
# ============================================================

def signal_handler(sig, frame):
    """Handle shutdown signals"""
    logger.info("Shutting down...")
    
    # Clean up
    mqtt_broker.disconnect()
    db_manager.close()
    
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# ============================================================
# MAIN ENTRY POINT
# ============================================================

if __name__ == '__main__':
    logger.info("Starting SmartLock Pro Backend...")
    
    # Initialize database
    db_manager.create_tables()
    
    # Start MQTT worker thread
    mqtt_thread = threading.Thread(target=mqtt_worker, daemon=True)
    mqtt_thread.start()
    
    # Start background tasks
    bg_thread = threading.Thread(target=background_tasks, daemon=True)
    bg_thread.start()
    
    # Start WebSocket server
    socketio.run(
        app,
        host='0.0.0.0',
        port=5000,
        debug=os.getenv('DEBUG', 'false').lower() == 'true',
        use_reloader=False
    )
