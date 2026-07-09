#!/usr/bin/env python3
"""
SmartLock Pro - Automation Engine
Handles automation rules and triggers
"""

import json
import logging
import threading
import time
from datetime import datetime, timedelta
from typing import Dict, Any, List, Optional
import schedule

logger = logging.getLogger(__name__)

class AutomationEngine:
    """Automation rule engine for SmartLock Pro"""
    
    def __init__(self, db_manager, mqtt_broker):
        self.db = db_manager
        self.mqtt = mqtt_broker
        self.rules = []
        self.running = False
        self.thread = None
        self.event_handlers = {}
        
        # Load rules
        self.reload_rules()
    
    def reload_rules(self):
        """Reload automation rules from database"""
        try:
            self.rules = self.db.get_active_rules()
            logger.info(f"Loaded {len(self.rules)} automation rules")
            
            # Register event handlers
            self._register_event_handlers()
        except Exception as e:
            logger.error(f"Failed to reload rules: {e}")
    
    def _register_event_handlers(self):
        """Register MQTT event handlers for rules"""
        # Clear existing handlers
        for topic in list(self.event_handlers.keys()):
            self.mqtt.unsubscribe(topic)
        self.event_handlers.clear()
        
        # Register handlers for each rule
        for rule in self.rules:
            conditions = rule.conditions
            
            # Check for MQTT event triggers
            if conditions.get('type') == 'mqtt_event':
                topic = conditions.get('topic')
                if topic:
                    if topic not in self.event_handlers:
                        self.event_handlers[topic] = []
                        self.mqtt.subscribe(topic, self._handle_mqtt_event)
                    self.event_handlers[topic].append(rule.id)
            
            # Check for scheduled triggers
            if conditions.get('type') == 'schedule':
                schedule_time = conditions.get('time')
                if schedule_time:
                    self._schedule_rule(rule)
    
    def _handle_mqtt_event(self, payload, topic):
        """Handle MQTT events for automation rules"""
        try:
            # Find rules triggered by this topic
            if topic in self.event_handlers:
                for rule_id in self.event_handlers[topic]:
                    rule = next((r for r in self.rules if r.id == rule_id), None)
                    if rule and self._check_conditions(rule, payload):
                        self._execute_actions(rule, payload)
        except Exception as e:
            logger.error(f"Error handling MQTT event: {e}")
    
    def _check_conditions(self, rule, payload) -> bool:
        """Check if rule conditions are met"""
        conditions = rule.conditions
        
        # Check condition type
        condition_type = conditions.get('type')
        
        if condition_type == 'mqtt_event':
            # Check payload conditions
            payload_conditions = conditions.get('payload', {})
            for key, expected_value in payload_conditions.items():
                actual_value = payload.get(key)
                if actual_value != expected_value:
                    return False
            return True
        
        elif condition_type == 'schedule':
            # Check schedule time
            now = datetime.now()
            schedule_time = conditions.get('time')
            
            if schedule_time:
                # Parse schedule time (HH:MM)
                try:
                    hours, minutes = map(int, schedule_time.split(':'))
                    current_hours = now.hour
                    current_minutes = now.minute
                    
                    # Check within 1 minute window
                    if abs(current_hours - hours) == 0 and abs(current_minutes - minutes) <= 1:
                        return True
                except:
                    pass
            
            # Check days of week
            days = conditions.get('days', [])
            if days:
                current_day = now.strftime('%A').lower()
                if current_day not in days:
                    return False
            
            return True
        
        elif condition_type == 'device_state':
            # Check device state conditions
            device_id = conditions.get('device_id')
            state = conditions.get('state')
            
            if device_id and state:
                # Get current state from MQTT
                status = self.mqtt.get_retained(f"/smartlock/{device_id}/status")
                if status:
                    current_state = status.get('payload', {}).get('lock_state')
                    return current_state == state
            
            return False
        
        return True
    
    def _execute_actions(self, rule, trigger_data=None):
        """Execute automation rule actions"""
        try:
            actions = rule.actions
            logger.info(f"Executing rule: {rule.name}")
            
            for action in actions:
                action_type = action.get('type')
                
                if action_type == 'mqtt_publish':
                    # Publish MQTT message
                    topic = action.get('topic')
                    payload = action.get('payload', {})
                    
                    # Add trigger data to payload
                    if trigger_data:
                        payload['trigger'] = trigger_data
                    
                    self.mqtt.publish(topic, payload)
                    logger.info(f"Published MQTT: {topic}")
                
                elif action_type == 'device_command':
                    # Send device command
                    device_id = action.get('device_id')
                    command = action.get('command')
                    
                    if device_id and command:
                        self.mqtt.publish(f"/smartlock/{device_id}/commands/{command}", {
                            'action': command,
                            'source': 'automation',
                            'rule': rule.name
                        })
                        logger.info(f"Sent command to {device_id}: {command}")
                
                elif action_type == 'notification':
                    # Send notification
                    title = action.get('title', 'Automation Alert')
                    message = action.get('message', '')
                    users = action.get('users', [])
                    
                    for user_id in users:
                        self.mqtt.publish(f"/smartlock/notification/{user_id}", {
                            'title': title,
                            'message': message,
                            'rule': rule.name
                        })
                    logger.info(f"Sent notification: {title}")
                
                elif action_type == 'webhook':
                    # Call webhook
                    import requests
                    url = action.get('url')
                    method = action.get('method', 'POST')
                    
                    if url:
                        try:
                            if method.upper() == 'GET':
                                response = requests.get(url, timeout=5)
                            else:
                                response = requests.post(url, json=action.get('data', {}), timeout=5)
                            
                            if response.status_code in [200, 201, 202]:
                                logger.info(f"Webhook called: {url}")
                            else:
                                logger.warning(f"Webhook returned {response.status_code}: {url}")
                        except Exception as e:
                            logger.error(f"Webhook error: {e}")
                
                elif action_type == 'delay':
                    # Add delay between actions
                    delay = action.get('seconds', 1)
                    time.sleep(delay)
            
            # Log action execution
            self.db.log_event(
                device_id='system',
                event_type='automation',
                message=f"Executed rule: {rule.name}",
                severity='info',
                rule_id=rule.id
            )
            
        except Exception as e:
            logger.error(f"Error executing actions for rule {rule.id}: {e}")
    
    def _schedule_rule(self, rule):
        """Schedule a rule based on schedule conditions"""
        try:
            schedule_data = rule.schedule
            
            # Parse schedule
            if schedule_data:
                schedule_type = schedule_data.get('type')
                
                if schedule_type == 'daily':
                    time_str = schedule_data.get('time', '00:00')
                    schedule.every().day.at(time_str).do(
                        self._execute_actions, rule
                    )
                
                elif schedule_type == 'weekly':
                    day = schedule_data.get('day', 'monday').lower()
                    time_str = schedule_data.get('time', '00:00')
                    
                    # Get day function
                    day_func = getattr(schedule.every(), day, None)
                    if day_func:
                        day_func().at(time_str).do(self._execute_actions, rule)
                
                elif schedule_type == 'interval':
                    interval = schedule_data.get('interval', 60)
                    schedule.every(interval).seconds.do(
                        self._execute_actions, rule
                    )
                
                logger.info(f"Scheduled rule: {rule.name}")
        except Exception as e:
            logger.error(f"Failed to schedule rule {rule.id}: {e}")
    
    def start(self):
        """Start the automation engine"""
        if self.running:
            return
        
        self.running = True
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()
        logger.info("Automation engine started")
    
    def stop(self):
        """Stop the automation engine"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=5)
        logger.info("Automation engine stopped")
    
    def _run(self):
        """Main loop for automation engine"""
        while self.running:
            try:
                # Run scheduled tasks
                schedule.run_pending()
                
                # Process time-based rules
                self._process_time_based_rules()
                
                time.sleep(1)
            except Exception as e:
                logger.error(f"Automation engine error: {e}")
                time.sleep(5)
    
    def _process_time_based_rules(self):
        """Process time-based rules"""
        now = datetime.now()
        
        for rule in self.rules:
            conditions = rule.conditions
            if conditions.get('type') == 'schedule':
                # Check if it's time to execute
                if self._check_conditions(rule, None):
                    self._execute_actions(rule)
    
    def process_rules(self):
        """Process all rules (called periodically)"""
        self._process_time_based_rules()
