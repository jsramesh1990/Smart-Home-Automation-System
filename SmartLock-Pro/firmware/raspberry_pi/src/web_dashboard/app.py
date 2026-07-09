#!/usr/bin/env python3
"""
SmartLock Pro - Web Dashboard
Flask-based web interface for monitoring and controlling SmartLock devices
"""

import os
import json
import logging
from datetime import datetime, timedelta
from flask import Flask, render_template, request, jsonify, session, redirect, url_for, flash
from flask_socketio import SocketIO, emit, join_room, leave_room
from flask_login import LoginManager, UserMixin, login_user, logout_user, login_required, current_user
from werkzeug.security import generate_password_hash, check_password_hash
import requests
import time

logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)
app.config['SECRET_KEY'] = os.getenv('DASHBOARD_SECRET_KEY', 'dev-secret-key-change-me')
app.config['SOCKETIO_MESSAGE_QUEUE'] = os.getenv('REDIS_URL', 'redis://localhost:6379/0')

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet')

# Initialize Login Manager
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'
login_manager.login_message_category = 'info'

# ============================================================
# USER MANAGEMENT (Simulated - In production, use database)
# ============================================================

class User(UserMixin):
    def __init__(self, id, username, email, role='user'):
        self.id = id
        self.username = username
        self.email = email
        self.role = role
    
    def is_admin(self):
        return self.role == 'admin'

# In-memory user store (replace with database in production)
users = {
    'admin': {
        'password': generate_password_hash('SmartLock2026!'),
        'email': 'admin@smartlock.com',
        'role': 'admin'
    },
    'user': {
        'password': generate_password_hash('user123'),
        'email': 'user@smartlock.com',
        'role': 'user'
    }
}

@login_manager.user_loader
def load_user(user_id):
    # In production, load from database
    for username, data in users.items():
        if str(data.get('id', username)) == user_id:
            return User(username, username, data['email'], data['role'])
    return None

# ============================================================
# API CONFIGURATION
# ============================================================

API_BASE_URL = os.getenv('API_BASE_URL', 'http://localhost:5000/api/v1')
MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))

# ============================================================
# ROUTES
# ============================================================

@app.route('/')
@login_required
def index():
    """Dashboard home page"""
    return render_template('index.html', 
                         current_user=current_user,
                         active_page='dashboard')

@app.route('/login', methods=['GET', 'POST'])
def login():
    """User login page"""
    if current_user.is_authenticated:
        return redirect(url_for('index'))
    
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')
        remember = request.form.get('remember', False)
        
        if username in users and check_password_hash(users[username]['password'], password):
            user = User(username, username, users[username]['email'], users[username]['role'])
            login_user(user, remember=remember)
            
            # Log login event
            logger.info(f"User logged in: {username}")
            
            next_page = request.args.get('next')
            return redirect(next_page) if next_page else redirect(url_for('index'))
        else:
            flash('Invalid username or password', 'danger')
    
    return render_template('login.html')

@app.route('/logout')
@login_required
def logout():
    """User logout"""
    logout_user()
    flash('You have been logged out', 'info')
    return redirect(url_for('login'))

@app.route('/devices')
@login_required
def devices():
    """Devices management page"""
    return render_template('devices.html', 
                         current_user=current_user,
                         active_page='devices')

@app.route('/users')
@login_required
def users_page():
    """Users management page"""
    if not current_user.is_admin():
        flash('Admin access required', 'danger')
        return redirect(url_for('index'))
    
    return render_template('users.html', 
                         current_user=current_user,
                         active_page='users')

@app.route('/logs')
@login_required
def logs():
    """Access logs page"""
    return render_template('logs.html', 
                         current_user=current_user,
                         active_page='logs')

@app.route('/automation')
@login_required
def automation():
    """Automation rules page"""
    if not current_user.is_admin():
        flash('Admin access required', 'danger')
        return redirect(url_for('index'))
    
    return render_template('automation.html', 
                         current_user=current_user,
                         active_page='automation')

@app.route('/settings')
@login_required
def settings():
    """System settings page"""
    if not current_user.is_admin():
        flash('Admin access required', 'danger')
        return redirect(url_for('index'))
    
    return render_template('settings.html', 
                         current_user=current_user,
                         active_page='settings')

@app.route('/profile')
@login_required
def profile():
    """User profile page"""
    return render_template('profile.html', 
                         current_user=current_user)

# ============================================================
# API PROXY ENDPOINTS
# ============================================================

@app.route('/api/dashboard/stats')
@login_required
def get_dashboard_stats():
    """Get dashboard statistics"""
    try:
        # In production, fetch from backend API
        response = requests.get(
            f"{API_BASE_URL}/system/metrics",
            headers={'Authorization': f'Bearer {session.get("access_token")}'}
        )
        
        if response.status_code == 200:
            return jsonify(response.json())
        else:
            # Fallback to simulated data
            return jsonify({
                'devices': {
                    'total': 5,
                    'online': 4,
                    'offline': 1
                },
                'users': 12,
                'events_today': 87,
                'access_stats': {
                    'granted': 75,
                    'denied': 12
                }
            })
    except:
        return jsonify({
            'devices': {'total': 5, 'online': 4, 'offline': 1},
            'users': 12,
            'events_today': 87,
            'access_stats': {'granted': 75, 'denied': 12}
        })

@app.route('/api/devices')
@login_required
def get_devices():
    """Get all devices"""
    try:
        response = requests.get(
            f"{API_BASE_URL}/devices",
            headers={'Authorization': f'Bearer {session.get("access_token")}'}
        )
        if response.status_code == 200:
            return jsonify(response.json())
    except:
        pass
    
    # Simulated device data
    devices = [
        {
            'id': 'DEV_001',
            'name': 'Front Door',
            'location': 'Main Entrance',
            'status': 'online',
            'firmware_version': 'v1.2.3',
            'last_seen': datetime.now().isoformat(),
            'lock_state': 'locked',
            'door_open': False,
            'battery_level': 85
        },
        {
            'id': 'DEV_002',
            'name': 'Back Door',
            'location': 'Back Entrance',
            'status': 'online',
            'firmware_version': 'v1.2.3',
            'last_seen': datetime.now().isoformat(),
            'lock_state': 'unlocked',
            'door_open': True,
            'battery_level': 72
        },
        {
            'id': 'DEV_003',
            'name': 'Garage Door',
            'location': 'Garage',
            'status': 'offline',
            'firmware_version': 'v1.2.2',
            'last_seen': (datetime.now() - timedelta(hours=2)).isoformat(),
            'lock_state': 'unknown',
            'door_open': False,
            'battery_level': 0
        }
    ]
    return jsonify(devices)

@app.route('/api/devices/<device_id>/control', methods=['POST'])
@login_required
def control_device(device_id):
    """Control a device (lock/unlock)"""
    data = request.get_json()
    action = data.get('action')
    
    # In production, send MQTT command
    # mqtt_client.publish(f"/smartlock/{device_id}/commands/{action}")
    
    # Simulate command
    logger.info(f"Device {device_id} {action} command received")
    
    return jsonify({
        'success': True,
        'message': f"Device {action} command sent"
    })

@app.route('/api/access-logs')
@login_required
def get_access_logs():
    """Get access logs"""
    limit = request.args.get('limit', 100, type=int)
    page = request.args.get('page', 1, type=int)
    
    try:
        response = requests.get(
            f"{API_BASE_URL}/logs/access?limit={limit}&page={page}",
            headers={'Authorization': f'Bearer {session.get("access_token")}'}
        )
        if response.status_code == 200:
            return jsonify(response.json())
    except:
        pass
    
    # Simulated logs
    logs = []
    methods = ['fingerprint', 'rfid', 'keypad', 'mobile', 'remote']
    users = ['John Doe', 'Jane Smith', 'Admin', 'Guest', 'Mike Johnson']
    
    for i in range(min(limit, 20)):
        granted = i % 5 != 2  # ~80% granted
        logs.append({
            'id': i + 1,
            'user': users[i % len(users)],
            'device': 'DEV_001',
            'method': methods[i % len(methods)],
            'granted': granted,
            'timestamp': (datetime.now() - timedelta(minutes=i*5)).isoformat()
        })
    
    return jsonify({
        'data': logs,
        'total': 100,
        'page': page,
        'pages': 5
    })

# ============================================================
# WEBSOCKET EVENTS
# ============================================================

@socketio.on('connect')
def handle_connect():
    """Handle client connection"""
    logger.info(f"Client connected: {request.sid}")
    emit('connected', {'status': 'ok'})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection"""
    logger.info(f"Client disconnected: {request.sid}")

@socketio.on('subscribe')
def handle_subscribe(data):
    """Handle subscription to device updates"""
    device_id = data.get('device_id')
    if device_id:
        room = f"device_{device_id}"
        join_room(room)
        logger.info(f"Client {request.sid} subscribed to {room}")

@socketio.on('unsubscribe')
def handle_unsubscribe(data):
    """Handle unsubscription"""
    device_id = data.get('device_id')
    if device_id:
        room = f"device_{device_id}"
        leave_room(room)
        logger.info(f"Client {request.sid} unsubscribed from {room}")

@socketio.on('get_device_status')
def handle_get_device_status(data):
    """Get device status via WebSocket"""
    device_id = data.get('device_id')
    if device_id:
        # In production, get from MQTT or API
        emit('device_status', {
            'device_id': device_id,
            'status': 'online',
            'lock_state': 'locked',
            'timestamp': datetime.now().isoformat()
        }, room=request.sid)

# ============================================================
# CONTEXT PROCESSOR
# ============================================================

@app.context_processor
def inject_now():
    """Inject current time into templates"""
    return {'now': datetime.now()}

# ============================================================
# ERROR HANDLERS
# ============================================================

@app.errorhandler(404)
def page_not_found(e):
    """404 error handler"""
    return render_template('404.html'), 404

@app.errorhandler(500)
def internal_server_error(e):
    """500 error handler"""
    return render_template('500.html'), 500

# ============================================================
# MAIN ENTRY POINT
# ============================================================

if __name__ == '__main__':
    logger.info("Starting SmartLock Pro Dashboard...")
    socketio.run(app, host='0.0.0.0', port=8080, debug=True, use_reloader=False)
