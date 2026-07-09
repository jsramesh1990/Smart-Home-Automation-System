#!/usr/bin/env python3
"""
SmartLock Pro - Database Manager
Handles all database operations using SQLAlchemy
"""

import os
import json
import logging
from datetime import datetime, timedelta
from typing import List, Dict, Any, Optional, Tuple
from sqlalchemy import create_engine, Column, Integer, String, Float, Boolean, DateTime, JSON, Text, ForeignKey
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, relationship, Session
from sqlalchemy.sql import func

logger = logging.getLogger(__name__)

Base = declarative_base()

# ============================================================
# DATABASE MODELS
# ============================================================

class User(Base):
    """User model"""
    __tablename__ = 'users'
    
    id = Column(Integer, primary_key=True)
    username = Column(String(50), unique=True, nullable=False)
    email = Column(String(100), unique=True, nullable=False)
    password_hash = Column(String(255), nullable=False)
    full_name = Column(String(100))
    role = Column(String(20), default='user')
    access_level = Column(Integer, default=1)
    enabled = Column(Boolean, default=True)
    
    # Authentication methods
    fingerprint_id = Column(Integer, nullable=True)
    rfid_uid = Column(String(20), nullable=True)
    pin_code = Column(String(8), nullable=True)
    otp_secret = Column(String(32), nullable=True)
    
    # User settings
    notification_preferences = Column(JSON, default={})
    schedule = Column(JSON, default={})
    
    # Timestamps
    created_at = Column(DateTime, default=func.now())
    updated_at = Column(DateTime, default=func.now(), onupdate=func.now())
    last_login = Column(DateTime)
    last_access = Column(DateTime)
    
    # Relationships
    access_logs = relationship("AccessLog", back_populates="user")
    
    def to_dict(self):
        return {
            'id': self.id,
            'username': self.username,
            'email': self.email,
            'full_name': self.full_name,
            'role': self.role,
            'access_level': self.access_level,
            'enabled': self.enabled,
            'has_fingerprint': self.fingerprint_id is not None,
            'has_rfid': self.rfid_uid is not None,
            'has_pin': self.pin_code is not None,
            'has_otp': self.otp_secret is not None,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'last_login': self.last_login.isoformat() if self.last_login else None,
            'last_access': self.last_access.isoformat() if self.last_access else None
        }

class Device(Base):
    """Device model"""
    __tablename__ = 'devices'
    
    id = Column(String(20), primary_key=True)
    name = Column(String(50), nullable=False)
    location = Column(String(100))
    type = Column(String(20), default='smartlock')
    firmware_version = Column(String(20))
    
    # Device status
    status = Column(String(20), default='offline')
    last_seen = Column(DateTime)
    ip_address = Column(String(45))
    mac_address = Column(String(17))
    
    # Device configuration
    config = Column(JSON, default={})
    settings = Column(JSON, default={})
    
    # Timestamps
    created_at = Column(DateTime, default=func.now())
    updated_at = Column(DateTime, default=func.now(), onupdate=func.now())
    
    # Relationships
    access_logs = relationship("AccessLog", back_populates="device")
    
    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'location': self.location,
            'type': self.type,
            'status': self.status,
            'firmware_version': self.firmware_version,
            'last_seen': self.last_seen.isoformat() if self.last_seen else None,
            'ip_address': self.ip_address,
            'mac_address': self.mac_address,
            'config': self.config,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }

class AccessLog(Base):
    """Access log model"""
    __tablename__ = 'access_logs'
    
    id = Column(Integer, primary_key=True)
    user_id = Column(Integer, ForeignKey('users.id'))
    device_id = Column(String(20), ForeignKey('devices.id'))
    
    # Access details
    method = Column(String(20))  # fingerprint, rfid, keypad, mobile, remote, otp
    granted = Column(Boolean)
    reason = Column(Text)
    
    # Location info
    ip_address = Column(String(45))
    user_agent = Column(String(255))
    
    # Additional data
    metadata = Column(JSON, default={})
    
    # Timestamp
    timestamp = Column(DateTime, default=func.now())
    
    # Relationships
    user = relationship("User", back_populates="access_logs")
    device = relationship("Device", back_populates="access_logs")
    
    def to_dict(self):
        return {
            'id': self.id,
            'user_id': self.user_id,
            'username': self.user.username if self.user else None,
            'device_id': self.device_id,
            'device_name': self.device.name if self.device else None,
            'method': self.method,
            'granted': self.granted,
            'reason': self.reason,
            'ip_address': self.ip_address,
            'timestamp': self.timestamp.isoformat() if self.timestamp else None
        }

class EventLog(Base):
    """System event log model"""
    __tablename__ = 'event_logs'
    
    id = Column(Integer, primary_key=True)
    device_id = Column(String(20), ForeignKey('devices.id'))
    
    # Event details
    event_type = Column(String(50))
    severity = Column(String(20), default='info')
    message = Column(Text)
    
    # Additional data
    metadata = Column(JSON, default={})
    
    # Timestamp
    timestamp = Column(DateTime, default=func.now())
    
    def to_dict(self):
        return {
            'id': self.id,
            'device_id': self.device_id,
            'event_type': self.event_type,
            'severity': self.severity,
            'message': self.message,
            'timestamp': self.timestamp.isoformat() if self.timestamp else None
        }

class AutomationRule(Base):
    """Automation rule model"""
    __tablename__ = 'automation_rules'
    
    id = Column(Integer, primary_key=True)
    name = Column(String(100), nullable=False)
    description = Column(Text)
    enabled = Column(Boolean, default=True)
    
    # Rule conditions (JSON)
    conditions = Column(JSON, nullable=False)
    
    # Rule actions (JSON)
    actions = Column(JSON, nullable=False)
    
    # Schedule (optional)
    schedule = Column(JSON, default={})
    
    # Priority
    priority = Column(Integer, default=0)
    
    # Timestamps
    created_at = Column(DateTime, default=func.now())
    updated_at = Column(DateTime, default=func.now(), onupdate=func.now())
    
    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'description': self.description,
            'enabled': self.enabled,
            'conditions': self.conditions,
            'actions': self.actions,
            'schedule': self.schedule,
            'priority': self.priority,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }

# ============================================================
# DATABASE MANAGER
# ============================================================

class DatabaseManager:
    """Database manager class"""
    
    def __init__(self, app=None):
        self.app = app        self.engine = None
        self.Session = None
        
        if app:
            self.init_app(app)
    
    def init_app(self, app):
        """Initialize with Flask app"""
        database_url = app.config.get('SQLALCHEMY_DATABASE_URI')
        self.engine = create_engine(database_url)
        self.Session = sessionmaker(bind=self.engine)
        
        # Create tables
        Base.metadata.create_all(self.engine)
        
        logger.info(f"Database initialized: {database_url}")
    
    def get_session(self) -> Session:
        """Get database session"""
        return self.Session()
    
    def close(self):
        """Close database connection"""
        if self.engine:
            self.engine.dispose()
    
    def check_connection(self) -> bool:
        """Check database connection"""
        try:
            session = self.get_session()
            session.execute('SELECT 1')
            session.close()
            return True
        except Exception as e:
            logger.error(f"Database connection check failed: {e}")
            return False
    
    # ============================================================
    # USER OPERATIONS
    # ============================================================
    
    def create_user(self, username: str, email: str, password_hash: str,
                   full_name: str = None, role: str = 'user') -> Optional[User]:
        """Create a new user"""
        session = self.get_session()
        try:
            user = User(
                username=username,
                email=email,
                password_hash=password_hash,
                full_name=full_name,
                role=role
            )
            session.add(user)
            session.commit()
            logger.info(f"User created: {username}")
            return user
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to create user: {e}")
            return None
        finally:
            session.close()
    
    def get_user(self, user_id: int) -> Optional[User]:
        """Get user by ID"""
        session = self.get_session()
        try:
            return session.query(User).filter(User.id == user_id).first()
        finally:
            session.close()
    
    def get_user_by_username(self, username: str) -> Optional[User]:
        """Get user by username"""
        session = self.get_session()
        try:
            return session.query(User).filter(User.username == username).first()
        finally:
            session.close()
    
    def get_user_by_email(self, email: str) -> Optional[User]:
        """Get user by email"""
        session = self.get_session()
        try:
            return session.query(User).filter(User.email == email).first()
        finally:
            session.close()
    
    def get_user_by_fingerprint(self, fingerprint_id: int) -> Optional[User]:
        """Get user by fingerprint ID"""
        session = self.get_session()
        try:
            return session.query(User).filter(
                User.fingerprint_id == fingerprint_id
            ).first()
        finally:
            session.close()
    
    def get_user_by_rfid(self, rfid_uid: str) -> Optional[User]:
        """Get user by RFID UID"""
        session = self.get_session()
        try:
            return session.query(User).filter(User.rfid_uid == rfid_uid).first()
        finally:
            session.close()
    
    def update_user(self, user_id: int, **kwargs) -> bool:
        """Update user details"""
        session = self.get_session()
        try:
            user = session.query(User).filter(User.id == user_id).first()
            if not user:
                return False
            
            for key, value in kwargs.items():
                if hasattr(user, key):
                    setattr(user, key, value)
            
            user.updated_at = func.now()
            session.commit()
            logger.info(f"User updated: {user_id}")
            return True
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to update user: {e}")
            return False
        finally:
            session.close()
    
    def delete_user(self, user_id: int) -> bool:
        """Delete a user"""
        session = self.get_session()
        try:
            user = session.query(User).filter(User.id == user_id).first()
            if user:
                session.delete(user)
                session.commit()
                logger.info(f"User deleted: {user_id}")
                return True
            return False
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to delete user: {e}")
            return False
        finally:
            session.close()
    
    def get_user_count(self) -> int:
        """Get total user count"""
        session = self.get_session()
        try:
            return session.query(User).count()
        finally:
            session.close()
    
    # ============================================================
    # DEVICE OPERATIONS
    # ============================================================
    
    def create_device(self, device_id: str, name: str, location: str = None,
                     device_type: str = 'smartlock') -> Optional[Device]:
        """Create a new device"""
        session = self.get_session()
        try:
            device = Device(
                id=device_id,
                name=name,
                location=location,
                type=device_type
            )
            session.add(device)
            session.commit()
            logger.info(f"Device created: {device_id}")
            return device
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to create device: {e}")
            return None
        finally:
            session.close()
    
    def get_device(self, device_id: str) -> Optional[Device]:
        """Get device by ID"""
        session = self.get_session()
        try:
            return session.query(Device).filter(Device.id == device_id).first()
        finally:
            session.close()
    
    def get_all_devices(self) -> List[Device]:
        """Get all devices"""
        session = self.get_session()
        try:
            return session.query(Device).all()
        finally:
            session.close()
    
    def update_device(self, device_id: str, **kwargs) -> bool:
        """Update device details"""
        session = self.get_session()
        try:
            device = session.query(Device).filter(Device.id == device_id).first()
            if not device:
                return False
            
            for key, value in kwargs.items():
                if hasattr(device, key):
                    setattr(device, key, value)
            
            device.updated_at = func.now()
            session.commit()
            logger.info(f"Device updated: {device_id}")
            return True
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to update device: {e}")
            return False
        finally:
            session.close()
    
    def update_device_status(self, device_id: str, status: str, **kwargs) -> bool:
        """Update device status"""
        session = self.get_session()
        try:
            device = session.query(Device).filter(Device.id == device_id).first()
            if not device:
                return False
            
            device.status = status
            device.last_seen = datetime.now()
            
            for key, value in kwargs.items():
                if hasattr(device, key):
                    setattr(device, key, value)
            
            session.commit()
            return True
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to update device status: {e}")
            return False
        finally:
            session.close()
    
    def delete_device(self, device_id: str) -> bool:
        """Delete a device"""
        session = self.get_session()
        try:
            device = session.query(Device).filter(Device.id == device_id).first()
            if device:
                session.delete(device)
                session.commit()
                logger.info(f"Device deleted: {device_id}")
                return True
            return False
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to delete device: {e}")
            return False
        finally:
            session.close()
    
    def get_device_count(self) -> int:
        """Get total device count"""
        session = self.get_session()
        try:
            return session.query(Device).count()
        finally:
            session.close()
    
    # ============================================================
    # ACCESS LOG OPERATIONS
    # ============================================================
    
    def log_access(self, user_id: int, device_id: str, method: str,
                   granted: bool, reason: str = None, **kwargs) -> Optional[AccessLog]:
        """Log an access attempt"""
        session = self.get_session()
        try:
            log = AccessLog(
                user_id=user_id,
                device_id=device_id,
                method=method,
                granted=granted,
                reason=reason,
                metadata=kwargs
            )
            session.add(log)
            session.commit()
            
            # Update user's last access
            if granted:
                user = session.query(User).filter(User.id == user_id).first()
                if user:
                    user.last_access = datetime.now()
                    session.commit()
            
            logger.info(f"Access logged: {user_id} -> {device_id} ({granted})")
            return log
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to log access: {e}")
            return None
        finally:
            session.close()
    
    def get_access_logs(self, user_id: int = None, device_id: str = None,
                        granted: bool = None, start_date: datetime = None,
                        end_date: datetime = None, limit: int = 100) -> List[AccessLog]:
        """Get access logs with filters"""
        session = self.get_session()
        try:
            query = session.query(AccessLog)
            
            if user_id:
                query = query.filter(AccessLog.user_id == user_id)
            if device_id:
                query = query.filter(AccessLog.device_id == device_id)
            if granted is not None:
                query = query.filter(AccessLog.granted == granted)
            if start_date:
                query = query.filter(AccessLog.timestamp >= start_date)
            if end_date:
                query = query.filter(AccessLog.timestamp <= end_date)
            
            return query.order_by(AccessLog.timestamp.desc()).limit(limit).all()
        finally:
            session.close()
    
    def get_events_today(self) -> int:
        """Get number of events today"""
        session = self.get_session()
        try:
            today = datetime.now().date()
            return session.query(AccessLog).filter(
                func.date(AccessLog.timestamp) == today
            ).count()
        finally:
            session.close()
    
    def cleanup_old_logs(self, days: int = 30):
        """Delete old logs"""
        session = self.get_session()
        try:
            cutoff = datetime.now() - timedelta(days=days)
            deleted = session.query(AccessLog).filter(
                AccessLog.timestamp < cutoff
            ).delete()
            session.commit()
            logger.info(f"Deleted {deleted} old logs")
            return deleted
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to cleanup logs: {e}")
            return 0
        finally:
            session.close()
    
    # ============================================================
    # EVENT LOG OPERATIONS
    # ============================================================
    
    def log_event(self, device_id: str, event_type: str, message: str,
                  severity: str = 'info', **kwargs) -> Optional[EventLog]:
        """Log a system event"""
        session = self.get_session()
        try:
            event = EventLog(
                device_id=device_id,
                event_type=event_type,
                severity=severity,
                message=message,
                metadata=kwargs
            )
            session.add(event)
            session.commit()
            return event
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to log event: {e}")
            return None
        finally:
            session.close()
    
    def get_event_logs(self, device_id: str = None, event_type: str = None,
                       severity: str = None, limit: int = 100) -> List[EventLog]:
        """Get event logs with filters"""
        session = self.get_session()
        try:
            query = session.query(EventLog)
            
            if device_id:
                query = query.filter(EventLog.device_id == device_id)
            if event_type:
                query = query.filter(EventLog.event_type == event_type)
            if severity:
                query = query.filter(EventLog.severity == severity)
            
            return query.order_by(EventLog.timestamp.desc()).limit(limit).all()
        finally:
            session.close()
    
    # ============================================================
    # AUTOMATION RULE OPERATIONS
    # ============================================================
    
    def create_rule(self, name: str, conditions: Dict, actions: Dict,
                   description: str = None, schedule: Dict = None) -> Optional[AutomationRule]:
        """Create an automation rule"""
        session = self.get_session()
        try:
            rule = AutomationRule(
                name=name,
                description=description,
                conditions=conditions,
                actions=actions,
                schedule=schedule or {}
            )
            session.add(rule)
            session.commit()
            logger.info(f"Automation rule created: {name}")
            return rule
        except Exception as e:
            session.rollback()
            logger.error(f"Failed to create rule: {e}")
            return None
        finally:
            session.close()
    
    def get_active_rules(self) -> List[AutomationRule]:
        """Get all active automation rules"""
        session = self.get_session()
        try:
            return session.query(AutomationRule).filter(
                AutomationRule.enabled == True
            ).order_by(AutomationRule.priority.desc()).all()
        finally:
            session.close()
