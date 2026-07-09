#!/usr/bin/env python3
"""
SmartLock Pro - Notification Service
Handles push notifications, emails, SMS, and other alerts
"""

import json
import logging
import smtplib
import requests
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from typing import Dict, Any, List, Optional
from datetime import datetime

logger = logging.getLogger(__name__)

class NotificationService:
    """Notification service for SmartLock Pro"""
    
    def __init__(self):
        self.config = self._load_config()
        self.initialize_services()
    
    def _load_config(self) -> Dict:
        """Load notification configuration"""
        # In production, load from config file or environment variables
        return {
            'email': {
                'enabled': True,
                'smtp_host': 'smtp.gmail.com',
                'smtp_port': 587,
                'username': 'smartlock@example.com',
                'password': 'your_email_password',
                'from_email': 'smartlock@example.com'
            },
            'push': {
                'enabled': True,
                'fcm_api_key': 'your_fcm_api_key',
                'apns_key': 'your_apns_key'
            },
            'sms': {
                'enabled': False,
                'twilio_account_sid': 'your_twilio_sid',
                'twilio_auth_token': 'your_twilio_token',
                'twilio_phone_number': '+1234567890'
            },
            'webhook': {
                'enabled': True,
                'endpoints': []
            }
        }
    
    def initialize_services(self):
        """Initialize notification services"""
        # Email
        if self.config.get('email', {}).get('enabled'):
            try:
                self.email_server = smtplib.SMTP(
                    self.config['email']['smtp_host'],
                    self.config['email']['smtp_port']
                )
                self.email_server.starttls()
                self.email_server.login(
                    self.config['email']['username'],
                    self.config['email']['password']
                )
                logger.info("Email service initialized")
            except Exception as e:
                logger.error(f"Failed to initialize email: {e}")
        
        # Push notifications (Firebase)
        # Implementation would use FCM/APNS libraries
    
    def send(self, user_id: int, title: str, message: str, 
             data: Dict = None, channels: List[str] = None) -> bool:
        """Send notification to user"""
        try:
            # Get user notification preferences
            user = self.db.get_user(user_id) if hasattr(self, 'db') else None
            if user:
                channels = channels or user.notification_preferences.get('channels', ['push', 'email'])
            
            # Send through each channel
            success = True
            
            if 'push' in channels and self.config.get('push', {}).get('enabled'):
                if not self._send_push_notification(user_id, title, message, data):
                    success = False
            
            if 'email' in channels and self.config.get('email', {}).get('enabled'):
                if not self._send_email(user_id, title, message, data):
                    success = False
            
            if 'sms' in channels and self.config.get('sms', {}).get('enabled'):
                if not self._send_sms(user_id, title, message, data):
                    success = False
            
            if 'webhook' in channels and self.config.get('webhook', {}).get('enabled'):
                if not self._send_webhook(title, message, data):
                    success = False
            
            # Log notification
            logger.info(f"Notification sent to user {user_id}: {title}")
            return success            
        except Exception as e:
            logger.error(f"Failed to send notification: {e}")
            return False
    
    def _send_push_notification(self, user_id: int, title: str, 
                               message: str, data: Dict = None) -> bool:
        """Send push notification via FCM/APNS"""
        try:
            # Get user's device tokens
            # In production, you'd store device tokens in the database
            
            # Send via FCM
            # payload = {
            #     'to': device_token,
            #     'notification': {
            #         'title': title,
            #         'body': message
            #     },
            #     'data': data or {}
            # }
            # response = requests.post(
            #     'https://fcm.googleapis.com/fcm/send',
            #     headers={
            #         'Authorization': f'key={self.config["push"]["fcm_api_key"]}',
            #         'Content-Type': 'application/json'
            #     },
            #     json=payload
            # )
            # return response.status_code == 200
            
            logger.info(f"Push notification would be sent: {title}")
            return True
            
        except Exception as e:
            logger.error(f"Push notification failed: {e}")
            return False
    
    def _send_email(self, user_id: int, title: str, 
                   message: str, data: Dict = None) -> bool:
        """Send email notification"""
        try:
            # Get user email
            user = self.db.get_user(user_id) if hasattr(self, 'db') else None
            if not user:
                logger.warning(f"User {user_id} not found for email")
                return False
            
            # Create email
            msg = MIMEMultipart()
            msg['From'] = self.config['email']['from_email']
            msg['To'] = user.email
            msg['Subject'] = f'[SmartLock] {title}'
            
            # Email body
            body = f"""
            SmartLock Pro Notification
            
            {message}
            
            Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
            
            ---
            This is an automated notification from SmartLock Pro.
            """
            
            msg.attach(MIMEText(body, 'plain'))
            
            # Send email
            self.email_server.send_message(msg)
            logger.info(f"Email sent to {user.email}: {title}")
            return True
            
        except Exception as e:
            logger.error(f"Email failed: {e}")
            return False
    
    def _send_sms(self, user_id: int, title: str, 
                 message: str, data: Dict = None) -> bool:
        """Send SMS notification"""
        try:
            # Get user phone number
            # In production, you'd store phone numbers in the database
            
            # Send via Twilio
            # client = Client(
            #     self.config['sms']['twilio_account_sid'],
            #     self.config['sms']['twilio_auth_token']
            # )
            # message = client.messages.create(
            #     body=f"[SmartLock] {title}: {message}",
            #     from_=self.config['sms']['twilio_phone_number'],
            #     to=user.phone_number
            # )
            # return message.sid is not None
            
            logger.info(f"SMS would be sent: {title}")
            return True
            
        except Exception as e:
            logger.error(f"SMS failed: {e}")
            return False
    
    def _send_webhook(self, title: str, message: str, data: Dict = None) -> bool:
        """Send webhook notification"""
        try:
            endpoints = self.config.get('webhook', {}).get('endpoints', [])
            
            for endpoint in endpoints:
                response = requests.post(
                    endpoint,
                    json={
                        'title': title,
                        'message': message,
                        'data': data,
                        'timestamp': datetime.now().isoformat()
                    },
                    timeout=5
                )
                if response.status_code not in [200, 201, 202]:
                    logger.warning(f"Webhook returned {response.status_code}: {endpoint}")
            
            return True
            
        except Exception as e:
            logger.error(f"Webhook failed: {e}")
            return False
    
    def send_alert(self, severity: str, message: str, data: Dict = None):
        """Send alert to all admins"""
        try:
            # Get all admin users
            # In production, you'd query the database
            admin_users = []  # [1, 2, 3]
            
            for user_id in admin_users:
                self.send(
                    user_id=user_id,
                    title=f"[{severity.upper()}] SmartLock Alert",
                    message=message,
                    data=data
                )
            
            logger.info(f"Alert sent: {severity} - {message}")
            return True
            
        except Exception as e:
            logger.error(f"Alert failed: {e}")
            return False
    
    def send_access_alert(self, device_id: str, user_id: int, 
                          granted: bool, method: str):
        """Send access event alert"""
        user = self.db.get_user(user_id) if hasattr(self, 'db') else None
        username = user.username if user else f"User {user_id}"
        
        if granted:
            title = "Access Granted"
            message = f"{username} granted access to {device_id} via {method}"
        else:
            title = "Access Denied"
            message = f"{username} denied access to {device_id} via {method}"
        
        # Send to user if they have notifications enabled
        if user:
            self.send(user_id, title, message, {'device_id': device_id, 'method': method})
        
        # Send alert to admins for denied access
        if not granted:
            self.send_alert("warning", f"Failed access attempt on {device_id}", {
                'user_id': user_id,
                'username': username,
                'method': method
            })
