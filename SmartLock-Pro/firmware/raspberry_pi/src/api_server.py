#!/usr/bin/env python3
"""
SmartLock Pro - REST API Server
Handles all HTTP API endpoints for the SmartLock system
"""

import json
import logging
from datetime import datetime, timedelta
from typing import Dict, Any, Optional, List
from flask import request, jsonify, Blueprint
from flask_jwt_extended import (
    create_access_token, create_refresh_token,
    jwt_required, get_jwt_identity, get_jwt
)
from werkzeug.security import generate_password_hash, check_password_hash

logger = logging.getLogger(__name__)

# Create API Blueprint
api_bp = Blueprint('api', __name__, url_prefix='/api/v1')

class APIServer:
    """REST API Server for SmartLock Pro"""
    
    def __init__(self, app, db_manager, mqtt_broker, 
                 automation_engine, notification_service, device_manager):
        self.app = app
        self.db = db_manager
        self.mqtt = mqtt_broker
        self.automation = automation_engine
        self.notification = notification_service
        self.device_manager = device_manager
        
        # Register routes
        self._register_routes()
    
    def _register_routes(self):
        """Register all API routes"""
        
        # ============================================================
        # AUTHENTICATION ENDPOINTS
        # ============================================================
        
        @api_bp.route('/auth/login', methods=['POST'])
        def login():
            """User login endpoint"""
            data = request.get_json()
            
            if not data or not data.get('username') or not data.get('password'):
                return jsonify({'error': 'Missing username or password'}), 400
            
            username = data['username']
            password = data['password']
            
            # Get user from database
            user = self.db.get_user_by_username(username)
            if not user:
                user = self.db.get_user_by_email(username)
            
            if not user or not check_password_hash(user.password_hash, password):
                return jsonify({'error': 'Invalid credentials'}), 401
            
            if not user.enabled:
                return jsonify({'error': 'User account disabled'}), 403
            
            # Generate tokens
            access_token = create_access_token(
                identity=str(user.id),
                additional_claims={
                    'username': user.username,
                    'role': user.role,
                    'access_level': user.access_level
                }
            )
            refresh_token = create_refresh_token(identity=str(user.id))
            
            # Update last login
            self.db.update_user(user.id, last_login=datetime.now())
            
            return jsonify({
                'access_token': access_token,
                'refresh_token': refresh_token,
                'user': user.to_dict()
            }), 200
        
        @api_bp.route('/auth/refresh', methods=['POST'])
        @jwt_required(refresh=True)
        def refresh():
            """Refresh access token"""
            user_id = int(get_jwt_identity())
            user = self.db.get_user(user_id)
            
            if not user:
                return jsonify({'error': 'User not found'}), 404
            
            access_token = create_access_token(
                identity=str(user.id),
                additional_claims={
                    'username': user.username,
                    'role': user.role,
                    'access_level': user.access_level
                }
            )
            
            return jsonify({'access_token': access_token}), 200
        
        @api_bp.route('/auth/logout', methods=['POST'])
        @jwt_required()
        def logout():
            """User logout"""
            # Add token to blacklist if needed
            return jsonify({'message': 'Logged out successfully'}), 200
        
        @api_bp.route('/auth/me', methods=['GET'])
        @jwt_required()
        def get_current_user():
            """Get current user info"""
            user_id = int(get_jwt_identity())
            user = self.db.get_user(user_id)
            
            if not user:
                return jsonify({'error': 'User not found'}), 404
            
            return jsonify(user.to_dict()), 200
        
        # ============================================================
        # USER MANAGEMENT ENDPOINTS
        # ============================================================
        
        @api_bp.route('/users', methods=['GET'])
        @jwt_required()
        def list_users():
            """List all users"""
            # Check admin permission
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            # Get all users
            users = self.db.get_all_users()
            return jsonify([u.to_dict() for u in users]), 200
        
        @api_bp.route('/users', methods=['POST'])
        @jwt_required()
        def create_user():
            """Create a new user"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            
            # Validate required fields
            required = ['username', 'email', 'password']
            for field in required:
                if field not in data:
                    return jsonify({'error': f'Missing {field}'}), 400
            
            # Check if user exists
            if self.db.get_user_by_username(data['username']):
                return jsonify({'error': 'Username already exists'}), 409
            
            if self.db.get_user_by_email(data['email']):
                return jsonify({'error': 'Email already exists'}), 409
            
            # Create user
            password_hash = generate_password_hash(data['password'])
            user = self.db.create_user(
                username=data['username'],
                email=data['email'],
                password_hash=password_hash,
                full_name=data.get('full_name'),
                role=data.get('role', 'user')
            )
            
            if not user:
                return jsonify({'error': 'Failed to create user'}), 500
            
            return jsonify(user.to_dict()), 201
        
        @api_bp.route('/users/<int:user_id>', methods=['GET'])
        @jwt_required()
        def get_user(user_id):
            """Get user by ID"""
            current_user_id = int(get_jwt_identity())
            claims = get_jwt()
            
            # Users can only view their own profile unless admin
            if current_user_id != user_id and claims.get('role') != 'admin':
                return jsonify({'error': 'Unauthorized'}), 403
            
            user = self.db.get_user(user_id)
            if not user:
                return jsonify({'error': 'User not found'}), 404
            
            return jsonify(user.to_dict()), 200
        
        @api_bp.route('/users/<int:user_id>', methods=['PUT'])
        @jwt_required()
        def update_user(user_id):
            """Update user details"""
            current_user_id = int(get_jwt_identity())
            claims = get_jwt()
            
            # Users can only update their own profile unless admin
            if current_user_id != user_id and claims.get('role') != 'admin':
                return jsonify({'error': 'Unauthorized'}), 403
            
            data = request.get_json()
            allowed_fields = ['full_name', 'email', 'role', 'enabled', 
                            'fingerprint_id', 'rfid_uid', 'pin_code']
            
            update_data = {}
            for field in allowed_fields:
                if field in data:
                    update_data[field] = data[field]
            
            # Update password if provided
            if 'password' in data and data['password']:
                update_data['password_hash'] = generate_password_hash(data['password'])
            
            if not update_data:
                return jsonify({'error': 'No fields to update'}), 400
            
            if self.db.update_user(user_id, **update_data):
                return jsonify({'message': 'User updated successfully'}), 200
            
            return jsonify({'error': 'Failed to update user'}), 500
        
        @api_bp.route('/users/<int:user_id>', methods=['DELETE'])
        @jwt_required()
        def delete_user(user_id):
            """Delete a user"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            current_user_id = int(get_jwt_identity())
            if current_user_id == user_id:
                return jsonify({'error': 'Cannot delete yourself'}), 400
            
            if self.db.delete_user(user_id):
                return jsonify({'message': 'User deleted successfully'}), 200
            
            return jsonify({'error': 'User not found'}), 404
        
        # ============================================================
        # DEVICE MANAGEMENT ENDPOINTS
        # ============================================================
        
        @api_bp.route('/devices', methods=['GET'])
        @jwt_required()
        def list_devices():
            """List all devices"""
            devices = self.db.get_all_devices()
            return jsonify([d.to_dict() for d in devices]), 200
        
        @api_bp.route('/devices', methods=['POST'])
        @jwt_required()
        def create_device():
            """Register a new device"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            
            if not data.get('device_id') or not data.get('name'):
                return jsonify({'error': 'Missing device_id or name'}), 400
            
            # Check if device exists
            if self.db.get_device(data['device_id']):
                return jsonify({'error': 'Device already exists'}), 409
            
            device = self.db.create_device(
                device_id=data['device_id'],
                name=data['name'],
                location=data.get('location'),
                device_type=data.get('type', 'smartlock')
            )
            
            if not device:
                return jsonify({'error': 'Failed to create device'}), 500
            
            # Send MQTT configuration
            self.mqtt.publish(f"/smartlock/{device.id}/config", {
                'action': 'configure',
                'config': data.get('config', {})
            })
            
            return jsonify(device.to_dict()), 201
        
        @api_bp.route('/devices/<device_id>', methods=['GET'])
        @jwt_required()
        def get_device(device_id):
            """Get device by ID"""
            device = self.db.get_device(device_id)
            if not device:
                return jsonify({'error': 'Device not found'}), 404
            
            return jsonify(device.to_dict()), 200
        
        @api_bp.route('/devices/<device_id>', methods=['PUT'])
        @jwt_required()
        def update_device(device_id):
            """Update device configuration"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            
            update_data = {}
            allowed_fields = ['name', 'location', 'status', 'config']
            for field in allowed_fields:
                if field in data:
                    update_data[field] = data[field]
            
            if not update_data:
                return jsonify({'error': 'No fields to update'}), 400
            
            if self.db.update_device(device_id, **update_data):
                # Send config update via MQTT
                if 'config' in update_data:
                    self.mqtt.publish(f"/smartlock/{device_id}/config", {
                        'action': 'update',
                        'config': update_data['config']
                    })
                return jsonify({'message': 'Device updated successfully'}), 200
            
            return jsonify({'error': 'Device not found'}), 404
        
        @api_bp.route('/devices/<device_id>', methods=['DELETE'])
        @jwt_required()
        def delete_device(device_id):
            """Delete a device"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            if self.db.delete_device(device_id):
                return jsonify({'message': 'Device deleted successfully'}), 200
            
            return jsonify({'error': 'Device not found'}), 404
        
        @api_bp.route('/devices/<device_id>/status', methods=['GET'])
        @jwt_required()
        def get_device_status(device_id):
            """Get device status"""
            device = self.db.get_device(device_id)
            if not device:
                return jsonify({'error': 'Device not found'}), 404
            
            # Get status via MQTT
            self.mqtt.publish(f"/smartlock/{device_id}/commands/status", {
                'action': 'status',
                'requester': 'api'
            })
            
            # Get retained status
            status = self.mqtt.get_retained(f"/smartlock/{device_id}/status")
            
            return jsonify({
                'device': device.to_dict(),
                'status': status.get('payload') if status else None
            }), 200
        
        @api_bp.route('/devices/<device_id>/lock', methods=['POST'])
        @jwt_required()
        def lock_device(device_id):
            """Lock the door"""
            device = self.db.get_device(device_id)
            if not device:
                return jsonify({'error': 'Device not found'}), 404
            
            user_id = int(get_jwt_identity())
            user = self.db.get_user(user_id)
            
            # Send lock command via MQTT
            result = self.mqtt.publish(f"/smartlock/{device_id}/commands/lock", {
                'action': 'lock',
                'requester': user.username,
                'timestamp': datetime.now().isoformat()
            })
            
            if not result:
                return jsonify({'error': 'Failed to send command'}), 500
            
            # Log access
            self.db.log_access(
                user_id=user_id,
                device_id=device_id,
                method='remote',
                granted=True,
                reason='Lock command'
            )
            
            return jsonify({'message': 'Lock command sent'}), 200
        
        @api_bp.route('/devices/<device_id>/unlock', methods=['POST'])
        @jwt_required()
        def unlock_device(device_id):
            """Unlock the door"""
            device = self.db.get_device(device_id)
            if not device:
                return jsonify({'error': 'Device not found'}), 404
            
            user_id = int(get_jwt_identity())
            user = self.db.get_user(user_id)
            
            # Send unlock command via MQTT
            result = self.mqtt.publish(f"/smartlock/{device_id}/commands/unlock", {
                'action': 'unlock',
                'requester': user.username,
                'timestamp': datetime.now().isoformat()
            })
            
            if not result:
                return jsonify({'error': 'Failed to send command'}), 500
            
            # Log access
            self.db.log_access(
                user_id=user_id,
                device_id=device_id,
                method='remote',
                granted=True,
                reason='Unlock command'
            )
            
            return jsonify({'message': 'Unlock command sent'}), 200
        
        @api_bp.route('/devices/<device_id>/ota', methods=['POST'])
        @jwt_required()
        def trigger_ota(device_id):
            """Trigger OTA update"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            version = data.get('version')
            
            if not version:
                return jsonify({'error': 'Version required'}), 400
            
            # Send OTA command
            result = self.mqtt.publish(f"/smartlock/{device_id}/commands/ota", {
                'action': 'update',
                'version': version,
                'url': f"http://ota.smartlock.com/firmware/{version}.bin"
            })
            
            if not result:
                return jsonify({'error': 'Failed to send OTA command'}), 500
            
            return jsonify({'message': 'OTA update triggered'}), 200
        
        # ============================================================
        # ACCESS LOG ENDPOINTS
        # ============================================================
        
        @api_bp.route('/logs/access', methods=['GET'])
        @jwt_required()
        def get_access_logs():
            """Get access logs"""
            user_id = request.args.get('user_id', type=int)
            device_id = request.args.get('device_id')
            granted = request.args.get('granted')
            if granted is not None:
                granted = granted.lower() == 'true'
            
            start_date = request.args.get('start_date')
            end_date = request.args.get('end_date')
            limit = request.args.get('limit', 100, type=int)
            
            # Parse dates
            if start_date:
                start_date = datetime.fromisoformat(start_date)
            if end_date:
                end_date = datetime.fromisoformat(end_date)
            
            logs = self.db.get_access_logs(
                user_id=user_id,
                device_id=device_id,
                granted=granted,
                start_date=start_date,
                end_date=end_date,
                limit=min(limit, 1000)
            )
            
            return jsonify([l.to_dict() for l in logs]), 200
        
        @api_bp.route('/logs/access/export', methods=['GET'])
        @jwt_required()
        def export_access_logs():
            """Export access logs as CSV"""
            # Implementation for CSV export
            # Similar to get_access_logs but returns CSV
            return jsonify({'message': 'Export endpoint'}), 200
        
        @api_bp.route('/logs/events', methods=['GET'])
        @jwt_required()
        def get_event_logs():
            """Get event logs"""
            device_id = request.args.get('device_id')
            event_type = request.args.get('event_type')
            severity = request.args.get('severity')
            limit = request.args.get('limit', 100, type=int)
            
            logs = self.db.get_event_logs(
                device_id=device_id,
                event_type=event_type,
                severity=severity,
                limit=min(limit, 1000)
            )
            
            return jsonify([l.to_dict() for l in logs]), 200
        
        # ============================================================
        # AUTOMATION ENDPOINTS
        # ============================================================
        
        @api_bp.route('/automation/rules', methods=['GET'])
        @jwt_required()
        def list_rules():
            """List automation rules"""
            rules = self.db.get_all_rules()
            return jsonify([r.to_dict() for r in rules]), 200
        
        @api_bp.route('/automation/rules', methods=['POST'])
        @jwt_required()
        def create_rule():
            """Create an automation rule"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            
            if not data.get('name') or not data.get('conditions') or not data.get('actions'):
                return jsonify({'error': 'Missing required fields'}), 400
            
            rule = self.db.create_rule(
                name=data['name'],
                description=data.get('description'),
                conditions=data['conditions'],
                actions=data['actions'],
                schedule=data.get('schedule')
            )
            
            if not rule:
                return jsonify({'error': 'Failed to create rule'}), 500
            
            # Reload automation engine
            self.automation.reload_rules()
            
            return jsonify(rule.to_dict()), 201
        
        @api_bp.route('/automation/rules/<int:rule_id>', methods=['PUT'])
        @jwt_required()
        def update_rule(rule_id):
            """Update an automation rule"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            data = request.get_json()
            
            update_data = {}
            allowed_fields = ['name', 'description', 'enabled', 'conditions', 'actions', 'schedule']
            for field in allowed_fields:
                if field in data:
                    update_data[field] = data[field]
            
            if not update_data:
                return jsonify({'error': 'No fields to update'}), 400
            
            if self.db.update_rule(rule_id, **update_data):
                self.automation.reload_rules()
                return jsonify({'message': 'Rule updated successfully'}), 200
            
            return jsonify({'error': 'Rule not found'}), 404
        
        @api_bp.route('/automation/rules/<int:rule_id>', methods=['DELETE'])
        @jwt_required()
        def delete_rule(rule_id):
            """Delete an automation rule"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            if self.db.delete_rule(rule_id):
                self.automation.reload_rules()
                return jsonify({'message': 'Rule deleted successfully'}), 200
            
            return jsonify({'error': 'Rule not found'}), 404
        
        # ============================================================
        # NOTIFICATION ENDPOINTS
        # ============================================================
        
        @api_bp.route('/notifications/test', methods=['POST'])
        @jwt_required()
        def test_notification():
            """Send a test notification"""
            data = request.get_json()
            user_id = int(get_jwt_identity())
            
            result = self.notification.send(
                user_id=user_id,
                title=data.get('title', 'Test Notification'),
                message=data.get('message', 'This is a test notification'),
                data=data.get('data', {})
            )
            
            if result:
                return jsonify({'message': 'Notification sent'}), 200
            
            return jsonify({'error': 'Failed to send notification'}), 500
        
        # ============================================================
        # SYSTEM ENDPOINTS
        # ============================================================
        
        @api_bp.route('/system/health', methods=['GET'])
        def system_health():
            """System health check"""
            return jsonify({
                'status': 'healthy',
                'timestamp': datetime.now().isoformat(),
                'version': '1.0.0'
            }), 200
        
        @api_bp.route('/system/metrics', methods=['GET'])
        @jwt_required()
        def system_metrics():
            """System metrics"""
            claims = get_jwt()
            if claims.get('role') != 'admin':
                return jsonify({'error': 'Admin access required'}), 403
            
            # Get metrics
            device_count = self.db.get_device_count()
            user_count = self.db.get_user_count()
            events_today = self.db.get_events_today()
            
            # Get online devices from MQTT
            online_devices = self.device_manager.get_online_devices()
            
            return jsonify({
                'devices': {
                    'total': device_count,
                    'online': len(online_devices),
                    'offline': device_count - len(online_devices)
                },
                'users': user_count,
                'events_today': events_today,
                'timestamp': datetime.now().isoformat()
            }), 200
        
        # Register blueprint
        self.app.register_blueprint(api_bp)
