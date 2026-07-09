#!/usr/bin/env python3
"""
SmartLock Pro - MQTT Broker Interface
Handles all MQTT communication with ESP32 devices
"""

import json
import logging
import threading
import time
from typing import Callable, Dict, Any, Optional
from datetime import datetime

import paho.mqtt.client as mqtt

logger = logging.getLogger(__name__)

class MQTTBroker:
    """MQTT Broker Interface for SmartLock Pro"""
    
    def __init__(self, host: str = 'localhost', port: int = 1883, 
                 username: str = None, password: str = None):
        self.host = host
        self.port = port
        self.username = username
        self.password = password
        
        self.client = None
        self.connected = False
        self.callbacks = {}
        self.retained_messages = {}
        
        self.setup_client()
    
    def setup_client(self):
        """Setup MQTT client"""
        self.client = mqtt.Client(
            client_id=f"smartlock_broker_{int(time.time())}",
            clean_session=True
        )
        
        # Set credentials
        if self.username and self.password:
            self.client.username_pw_set(self.username, self.password)
        
        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        self.client.on_publish = self._on_publish
        self.client.on_subscribe = self._on_subscribe
    
    def connect(self):
        """Connect to MQTT broker"""
        try:
            logger.info(f"Connecting to MQTT broker at {self.host}:{self.port}")
            self.client.connect(self.host, self.port, 60)
            
            # Start loop in separate thread
            self.client.loop_start()
            
            # Wait for connection
            timeout = 30
            while not self.connected and timeout > 0:
                time.sleep(0.5)
                timeout -= 0.5
            
            if self.connected:
                logger.info("MQTT broker connected successfully")
                self._subscribe_default_topics()
                return True
            else:
                logger.error("MQTT connection timeout")
                return False
                
        except Exception as e:
            logger.error(f"MQTT connection failed: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            logger.info("MQTT disconnected")
    
    def _on_connect(self, client, userdata, flags, rc):
        """Handle connection event"""
        if rc == 0:
            self.connected = True
            logger.info(f"MQTT connected with result code {rc}")
        else:
            self.connected = False
            logger.error(f"MQTT connection failed with result code {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """Handle disconnection event"""
        self.connected = False
        logger.warning(f"MQTT disconnected with result code {rc}")
        
        # Attempt to reconnect
        if rc != 0:
            logger.info("Attempting to reconnect...")
            time.sleep(5)
            self.connect()
    
    def _on_message(self, client, userdata, msg):
        """Handle incoming messages"""
        try:
            topic = msg.topic
            payload = msg.payload.decode('utf-8')
            
            logger.debug(f"MQTT message received: {topic} -> {payload}")
            
            # Parse JSON if possible
            try:
                data = json.loads(payload)
            except:
                data = payload
            
            # Execute callbacks for this topic
            if topic in self.callbacks:
                for callback in self.callbacks[topic]:
                    try:
                        callback(data, topic)
                    except Exception as e:
                        logger.error(f"Callback error for {topic}: {e}")
            
            # Also handle wildcard callbacks
            for pattern, callbacks in self.callbacks.items():
                if pattern.endswith('#'):
                    prefix = pattern[:-1]
                    if topic.startswith(prefix):
                        for callback in callbacks:
                            try:
                                callback(data, topic)
                            except Exception as e:
                                logger.error(f"Wildcard callback error: {e}")
                
                elif pattern.endswith('+'):
                    # Single-level wildcard
                    parts = topic.split('/')
                    pattern_parts = pattern.split('/')
                    if len(parts) == len(pattern_parts):
                        match = True
                        for i, part in enumerate(pattern_parts):
                            if part != '+' and part != parts[i]:
                                match = False
                                break
                        if match:
                            for callback in callbacks:
                                try:
                                    callback(data, topic)
                                except Exception as e:
                                    logger.error(f"Wildcard callback error: {e}")
            
            # Store retained messages
            if msg.retain:
                self.retained_messages[topic] = {
                    'payload': data,
                    'timestamp': datetime.now().isoformat()
                }
                
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
    
    def _on_publish(self, client, userdata, mid):
        """Handle publish confirmation"""
        logger.debug(f"MQTT message published with mid: {mid}")
    
    def _on_subscribe(self, client, userdata, mid, granted_qos):
        """Handle subscription confirmation"""
        logger.debug(f"MQTT subscription confirmed: {mid} with QoS {granted_qos}")
    
    def _subscribe_default_topics(self):
        """Subscribe to default topics"""
        topics = [
            "/smartlock/+/status/#",
            "/smartlock/+/events/#",
            "/smartlock/+/sensors/#",
            "/smartlock/system/#",
            "/smartlock/alerts/#"
        ]
        for topic in topics:
            self.subscribe(topic)
    
    def publish(self, topic: str, payload: Any, qos: int = 1, retain: bool = False):
        """Publish a message to a topic"""
        if not self.connected:
            logger.warning(f"Cannot publish to {topic}: Not connected")
            return False
        
        try:
            # Convert payload to JSON if dict
            if isinstance(payload, dict):
                payload = json.dumps(payload)
            elif not isinstance(payload, str):
                payload = str(payload)
            
            result = self.client.publish(topic, payload, qos=qos, retain=retain)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Published to {topic}: {payload}")
                return True
            else:
                logger.error(f"Failed to publish to {topic}: {result.rc}")
                return False
                
        except Exception as e:
            logger.error(f"Publish error: {e}")
            return False
    
    def subscribe(self, topic: str, callback: Callable = None, qos: int = 1):
        """Subscribe to a topic"""
        if not self.connected:
            logger.warning(f"Cannot subscribe to {topic}: Not connected")
            return False
        
        try:
            result = self.client.subscribe(topic, qos)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Subscribed to {topic}")
                
                # Register callback
                if callback:
                    if topic not in self.callbacks:
                        self.callbacks[topic] = []
                    self.callbacks[topic].append(callback)
                
                return True
            else:
                logger.error(f"Failed to subscribe to {topic}: {result.rc}")
                return False
                
        except Exception as e:
            logger.error(f"Subscription error: {e}")
            return False
    
    def unsubscribe(self, topic: str, callback: Callable = None):
        """Unsubscribe from a topic"""
        if not self.connected:
            return False
        
        try:
            result = self.client.unsubscribe(topic)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Unsubscribed from {topic}")
                
                # Remove callback
                if topic in self.callbacks:
                    if callback and callback in self.callbacks[topic]:
                        self.callbacks[topic].remove(callback)
                    if not callback or not self.callbacks[topic]:
                        del self.callbacks[topic]
                
                return True
            else:
                return False
                
        except Exception as e:
            logger.error(f"Unsubscribe error: {e}")
            return False
    
    def get_retained(self, topic: str) -> Optional[Dict]:
        """Get retained message for a topic"""
        return self.retained_messages.get(topic)
    
    def publish_device_status(self, device_id: str, status: Dict):
        """Publish device status"""
        topic = f"/smartlock/{device_id}/status"
        status_data = {
            **status,
            'timestamp': datetime.now().isoformat()
        }
        return self.publish(topic, status_data, retain=True)
    
    def publish_device_event(self, device_id: str, event_type: str, data: Dict):
        """Publish device event"""
        topic = f"/smartlock/{device_id}/events/{event_type}"
        event_data = {
            **data,
            'timestamp': datetime.now().isoformat()
        }
        return self.publish(topic, event_data)
    
    def publish_sensor_data(self, device_id: str, sensor_type: str, value: Any):
        """Publish sensor data"""
        topic = f"/smartlock/{device_id}/sensors/{sensor_type}"
        data = {
            'value': value,
            'timestamp': datetime.now().isoformat()
        }
        return self.publish(topic, data)
    
    def publish_heartbeat(self):
        """Publish system heartbeat"""
        topic = "/smartlock/system/heartbeat"
        data = {
            'status': 'online',
            'timestamp': datetime.now().isoformat(),
            'uptime': int(time.time())
        }
        return self.publish(topic, data, retain=True)
    
    def is_connected(self) -> bool:
        """Check if connected to broker"""
        return self.connected
