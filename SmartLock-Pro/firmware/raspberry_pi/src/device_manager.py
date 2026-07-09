#!/usr/bin/env python3
"""
SmartLock Pro - Device Manager
Manages ESP32 devices and their state
"""

import json
import logging
import threading
import time
from datetime import datetime, timedelta
from typing import Dict, Any, List, Optional

logger = logging.getLogger(__name__)

class DeviceManager:
    """Device manager for SmartLock Pro"""
    
    def __init__(self, db_manager, mqtt_broker):
        self.db = db_manager
        self.mqtt = mqtt_broker
        self.devices = {}
        self.online_devices = set()
        self.device_status = {}
        self.heartbeat_timeout = 120  # seconds
        
        # Register MQTT callbacks
        self._register_mqtt_callbacks()
    
    def _register_mqtt_callbacks(self):
        """Register MQTT callbacks for device communication"""
        # Device status updates
        self.mqtt.subscribe("/smartlock/+/status", self._handle_status_update)
        self.mqtt.subscribe("/smartlock/+/events", self._handle_event)
        self.mqtt.subscribe("/smartlock/+/sensors", self._handle_sensor_data)
        self.mqtt.subscribe("/smartlock/system/heartbeat", self._handle_heartbeat)
        
        # Device discovery
        self.mqtt.subscribe("/smartlock/system/register", self._handle_device_registration)
    
    def _handle_status_update(self, payload, topic):
        """Handle device status updates"""
        try:
            # Extract device ID from topic
            parts = topic.split('/')
            if len(parts) >= 3:
                device_id = parts[2]
                
                # Update device status
                self.device_status[device_id] = {
                    'payload': payload,
                    'timestamp': datetime.now()
                }
                
                # Update device online status
                self.online_devices.add(device_id)
                
                # Update database
                self.db.update_device_status(
                    device_id=device_id,
                    status='online',
                    last_seen=datetime.now()
                )
                
                # Process automation rules
                self._process_device_rules(device_id, 'status', payload)
                
        except Exception as e:
            logger.error(f"Error handling status update: {e}")
    
    def _handle_event(self, payload, topic):
        """Handle device events"""
        try:
            parts = topic.split('/')
            if len(parts) >= 4:
                device_id = parts[2]
                event_type = parts[3]
                
                # Log event
                self.db.log_event(
                    device_id=device_id,
                    event_type=event_type,
                    message=payload.get('message', ''),
                    severity=payload.get('severity', 'info'),
                    event_data=payload
                )
                
                # Process automation rules
                self._process_device_rules(device_id, 'event', payload)
                
        except Exception as e:
            logger.error(f"Error handling event: {e}")
    
    def _handle_sensor_data(self, payload, topic):
        """Handle sensor data"""
        try:
            parts = topic.split('/')
            if len(parts) >= 4:
                device_id = parts[2]
                sensor_type = parts[3]
                
                # Store sensor data in database (optional)
                # Could be used for analytics
                
                # Process automation rules
                self._process_device_rules(device_id, 'sensor', payload)
                
        except Exception as e:
            logger.error(f"Error handling sensor data: {e}")
    
    def _handle_heartbeat(self, payload, topic):
        """Handle device heartbeat"""
        try:
            # Update all devices from heartbeat
            if isinstance(payload, dict):
                devices = payload.get('devices', [])
                for device_id in devices:
                    self.online_devices.add(device_id)
                    self.db.update_device_status(
                        device_id=device_id,
                        status='online',
                        last_seen=datetime.now()
                    )
                    
        except Exception as e:
            logger.error(f"Error handling heartbeat: {e}")
    
    def _handle_device_registration(self, payload, topic):
        """Handle new device registration"""
        try:
            if isinstance(payload, dict):
                device_id = payload.get('device_id')
                name = payload.get('name')
                firmware = payload.get('firmware_version')
                
                if device_id:
                    # Check if device exists
                    device = self.db.get_device(device_id)
                    if not device:
                        # Create new device
                        self.db.create_device(
                            device_id=device_id,
                            name=name or f"Device {device_id}",
                            device_type=payload.get('type', 'smartlock')
                        )
                        logger.info(f"New device registered: {device_id}")
                    else:
                        # Update device
                        self.db.update_device(
                            device_id=device_id,
                            firmware_version=firmware
                        )
                        logger.info(f"Device updated: {device_id}")
                    
                    # Add to online devices
                    self.online_devices.add(device_id)
                    
        except Exception as e:
            logger.error(f"Error handling device registration: {e}")
    
    def _process_device_rules(self, device_id: str, event_type: str, data: Dict):
        """Process automation rules for device events"""
        # This would trigger automation rules
        # Implementation in automation_engine.py
        pass
    
    def get_device(self, device_id: str) -> Optional[Dict]:
        """Get device details"""
        device = self.db.get_device(device_id)
        if device:
            return {
                'device': device.to_dict(),
                'status': self.device_status.get(device_id),
                'online': device_id in self.online_devices
            }
        return None
    
    def get_all_devices(self) -> List[Dict]:
        """Get all devices with status"""
        devices = self.db.get_all_devices()
        result = []
        
        for device in devices:
            result.append({
                'device': device.to_dict(),
                'status': self.device_status.get(device.id),
                'online': device.id in self.online_devices
            })
        
        return result
    
    def get_online_devices(self) -> List[str]:
        """Get list of online device IDs"""
        return list(self.online_devices)
    
    def get_device_status(self, device_id: str) -> Optional[Dict]:
        """Get current device status"""
        return self.device_status.get(device_id)
    
    def send_command(self, device_id: str, command: str, data: Dict = None) -> bool:
        """Send command to a device"""
        topic = f"/smartlock/{device_id}/commands/{command}"
        payload = data or {}
        payload['timestamp'] = datetime.now().isoformat()
        
        return self.mqtt.publish(topic, payload)
    
    def update_all_devices(self):
        """Update status for all devices"""
        try:
            # Check for devices that haven't sent heartbeat
            timeout = datetime.now() - timedelta(seconds=self.heartbeat_timeout)
            
            for device_id in list(self.online_devices):
                device = self.db.get_device(device_id)
                if device and device.last_seen:
                    if device.last_seen < timeout:
                        # Device offline
                        self.online_devices.discard(device_id)
                        self.db.update_device_status(
                            device_id=device_id,
                            status='offline'
                        )
                        logger.info(f"Device offline: {device_id}")
            
            # Request status from all devices
            for device_id in self.online_devices:
                self.send_command(device_id, 'status')
                
        except Exception as e:
            logger.error(f"Error updating devices: {e}")
    
    def discover_devices(self):
        """Discover new devices on the network"""
        try:
            # Send discovery broadcast
            self.mqtt.publish("/smartlock/system/discover", {
                'action': 'discover',
                'requester': 'system'
            }, retain=False)
            
            logger.info("Device discovery broadcast sent")
            return True
            
        except Exception as e:
            logger.error(f"Discovery failed: {e}")
            return False
