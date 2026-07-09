#!/usr/bin/env python3
"""
SmartLock Pro - Configuration Management
Centralized configuration handling
"""

import os
import json
import yaml
from typing import Dict, Any, Optional
from pathlib import Path

class Config:
    """Configuration manager for SmartLock Pro"""
    
    def __init__(self, config_file=None):
        self.config = {}
        self.config_file = config_file or self._get_default_config_path()
        
        # Load configuration
        self.load()
    
    def _get_default_config_path(self) -> str:
        """Get default config path"""
        return os.path.join(os.path.dirname(__file__), '../config/config.yaml')
    
    def load(self):
        """Load configuration from file"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    if self.config_file.endswith('.yaml') or self.config_file.endswith('.yml'):
                        self.config = yaml.safe_load(f)
                    elif self.config_file.endswith('.json'):
                        self.config = json.load(f)
                    else:
                        # Default to YAML
                        self.config = yaml.safe_load(f)
            else:
                self.config = self._get_default_config()
                self.save()
        except Exception as e:
            print(f"Error loading config: {e}")
            self.config = self._get_default_config()
    
    def save(self):
        """Save configuration to file"""
        try:
            # Create directory if needed
            os.makedirs(os.path.dirname(self.config_file), exist_ok=True)
            
            with open(self.config_file, 'w') as f:
                if self.config_file.endswith('.yaml') or self.config_file.endswith('.yml'):
                    yaml.dump(self.config, f, default_flow_style=False)
                elif self.config_file.endswith('.json'):
                    json.dump(self.config, f, indent=2)
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def _get_default_config(self) -> Dict:
        """Get default configuration"""
        return {
            'server': {
                'host': '0.0.0.0',
                'port': 5000,
                'debug': False,
                'secret_key': os.getenv('SECRET_KEY', 'dev-secret-key-change-me')
            },
            'database': {
                'url': os.getenv('DATABASE_URL', 'sqlite:///smartlock.db'),
                'pool_size': 10,
                'max_overflow': 20
            },
            'mqtt': {
                'broker': os.getenv('MQTT_BROKER', 'localhost'),
                'port': int(os.getenv('MQTT_PORT', 1883)),
                'username': os.getenv('MQTT_USERNAME', ''),
                'password': os.getenv('MQTT_PASSWORD', ''),
                'tls_enabled': False,
                'keepalive': 60
            },
            'redis': {
                'url': os.getenv('REDIS_URL', 'redis://localhost:6379/0'),
                'enabled': True
            },
            'influxdb': {
                'url': os.getenv('INFLUXDB_URL', 'http://localhost:8086'),
                'token': os.getenv('INFLUXDB_TOKEN', ''),
                'org': os.getenv('INFLUXDB_ORG', 'smartlock'),
                'bucket': os.getenv('INFLUXDB_BUCKET', 'smartlock')
            },
            'notifications': {
                'email': {
                    'enabled': False,
                    'smtp_host': 'smtp.gmail.com',
                    'smtp_port': 587,
                    'username': '',
                    'password': '',
                    'from_email': 'smartlock@example.com'
                },
                'push': {
                    'enabled': False,
                    'fcm_api_key': '',
                    'apns_key': ''
                },
                'sms': {
                    'enabled': False,
                    'twilio_account_sid': '',
                    'twilio_auth_token': '',
                    'twilio_phone_number': ''
                }
            },
            'automation': {
                'enabled': True,
                'check_interval': 60,
                'max_rules': 50
            },
            'security': {
                'jwt_expiration': 3600,
                'refresh_expiration': 86400,
                'password_min_length': 8,
                'max_failed_attempts': 5,
                'lockout_duration': 300,
                'csrf_protection': True
            },
            'logging': {
                'level': 'INFO',
                'file': 'logs/smartlock.log',
                'max_size': 10485760,
                'backup_count': 10
            },
            'device': {
                'heartbeat_timeout': 120,
                'auto_discovery': True,
                'ota_enabled': True
            }
        }
    
    def get(self, key: str, default=None) -> Any:
        """Get configuration value by key"""
        parts = key.split('.')
        value = self.config
        
        for part in parts:
            if isinstance(value, dict) and part in value:
                value = value[part]
            else:
                return default
        
        return value
    
    def set(self, key: str, value: Any):
        """Set configuration value"""
        parts = key.split('.')
        config = self.config
        
        for part in parts[:-1]:
            if part not in config:
                config[part] = {}
            config = config[part]
        
        config[parts[-1]] = value
        self.save()
    
    def get_all(self) -> Dict:
        """Get all configuration"""
        return self.config
    
    def reload(self):
        """Reload configuration from file"""
        self.load()
    
    def validate(self) -> bool:
        """Validate configuration"""
        required = ['server.secret_key', 'database.url']
        
        for key in required:
            if not self.get(key):
                print(f"Missing required config: {key}")
                return False
        
        return True

# ============================================================
# ENVIRONMENT VARIABLE HELPERS
# ============================================================

def get_env(key: str, default=None) -> str:
    """Get environment variable with default"""
    return os.getenv(key, default)

def get_env_bool(key: str, default=False) -> bool:
    """Get boolean environment variable"""
    value = os.getenv(key, str(default)).lower()
    return value in ['true', '1', 'yes', 'on']

def get_env_int(key: str, default=0) -> int:
    """Get integer environment variable"""
    try:
        return int(os.getenv(key, str(default)))
    except ValueError:
        return default

# ============================================================
# SINGLETON INSTANCE
# ============================================================

_config = None

def get_config() -> Config:
    """Get singleton config instance"""
    global _config
    if _config is None:
        _config = Config()
    return _config
