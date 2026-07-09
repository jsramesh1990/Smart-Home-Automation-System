#!/usr/bin/env python3
"""
SmartLock Pro - Logging Configuration
Centralized logging setup for the entire application
"""

import os
import sys
import logging
import logging.handlers
from datetime import datetime

def setup_logging(log_level=None):
    """Setup logging configuration"""
    
    # Set default log level
    if log_level is None:
        log_level = os.getenv('LOG_LEVEL', 'INFO').upper()
    
    numeric_level = getattr(logging, log_level, logging.INFO)
    
    # Create logger
    logger = logging.getLogger('smartlock')
    logger.setLevel(numeric_level)
    
    # Remove existing handlers
    logger.handlers = []
    
    # Create formatters
    detailed_formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(filename)s:%(lineno)d - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    
    simple_formatter = logging.Formatter(
        '%(asctime)s - %(levelname)s - %(message)s',
        datefmt='%H:%M:%S'
    )
    
    # Console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setLevel(numeric_level)
    console_handler.setFormatter(simple_formatter)
    logger.addHandler(console_handler)
    
    # File handler (rotation)
    log_dir = os.getenv('LOG_DIR', 'logs')
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    
    log_file = os.path.join(log_dir, 'smartlock.log')
    file_handler = logging.handlers.RotatingFileHandler(
        log_file,
        maxBytes=10*1024*1024,  # 10MB
        backupCount=10
    )
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(detailed_formatter)
    logger.addHandler(file_handler)
    
    # Error log file
    error_log_file = os.path.join(log_dir, 'errors.log')
    error_handler = logging.handlers.RotatingFileHandler(
        error_log_file,
        maxBytes=10*1024*1024,
        backupCount=5
    )
    error_handler.setLevel(logging.ERROR)
    error_handler.setFormatter(detailed_formatter)
    logger.addHandler(error_handler)
    
    # JSON log file (for structured logging)
    json_log_file = os.path.join(log_dir, 'structured.log')
    json_handler = logging.handlers.RotatingFileHandler(
        json_log_file,
        maxBytes=10*1024*1024,
        backupCount=5
    )
    json_handler.setLevel(logging.INFO)
    
    import json
    class JSONFormatter(logging.Formatter):
        def format(self, record):
            log_data = {
                'timestamp': self.formatTime(record, '%Y-%m-%d %H:%M:%S'),
                'level': record.levelname,
                'logger': record.name,
                'module': record.module,
                'function': record.funcName,
                'line': record.lineno,
                'message': record.getMessage()
            }
            
            if hasattr(record, 'extra'):
                log_data.update(record.extra)
            
            return json.dumps(log_data)
    
    json_handler.setFormatter(JSONFormatter())
    logger.addHandler(json_handler)
    
    # Syslog handler (for Docker)
    if os.getenv('SYSLOG_ENABLED', 'false').lower() == 'true':
        syslog_handler = logging.handlers.SysLogHandler(
            address='/dev/log'
        )
        syslog_handler.setLevel(logging.INFO)
        syslog_handler.setFormatter(detailed_formatter)
        logger.addHandler(syslog_handler)
    
    # Avoid duplicate logs
    logger.propagate = False
    
    # Log system info
    logger.info(f"Logging initialized (level={log_level})")
    logger.info(f"Python version: {sys.version}")
    logger.info(f"Working directory: {os.getcwd()}")
    
    return logger

def get_logger(name):
    """Get a logger instance"""
    return logging.getLogger(f'smartlock.{name}')

class LogContext:
    """Context manager for logging with extra context"""
    
    def __init__(self, **kwargs):
        self.kwargs = kwargs
        self.logger = logging.getLogger('smartlock')
    
    def __enter__(self):
        # Add extra fields to logger
        self.old_factory = self.logger.makeRecord
        
        def makeRecord(name, level, fn, lno, msg, args, exc_info, func=None, extra=None, sinfo=None):
            if extra is None:
                extra = {}
            extra.update(self.kwargs)
            return self.old_factory(name, level, fn, lno, msg, args, exc_info, func, extra, sinfo)
        
        self.logger.makeRecord = makeRecord
        return self.logger
    
    def __exit__(self, *args):
        self.logger.makeRecord = self.old_factory

# ============================================================
# LOGGING HELPER FUNCTIONS
# ============================================================

def log_access(user_id: str, device_id: str, granted: bool, method: str):
    """Log access events specifically"""
    logger = logging.getLogger('smartlock.access')
    
    logger.info(
        f"ACCESS: user={user_id} device={device_id} "
        f"granted={granted} method={method}",
        extra={
            'event_type': 'access',
            'user_id': user_id,
            'device_id': device_id,
            'granted': granted,
            'method': method
        }
    )

def log_security_event(event_type: str, details: dict):
    """Log security events"""
    logger = logging.getLogger('smartlock.security')
    
    logger.warning(
        f"SECURITY: {event_type} - {details}",
        extra={
            'event_type': 'security',
            'security_event': event_type,
            'details': details
        }
    )

def log_system_event(event_type: str, message: str, level='info'):
    """Log system events"""
    logger = logging.getLogger('smartlock.system')
    
    log_func = getattr(logger, level, logger.info)
    log_func(
        f"SYSTEM: {event_type} - {message}",
        extra={
            'event_type': 'system',
            'system_event': event_type,
            'level': level
        }
    )

# ============================================================
# DEBUGGING HELPER
# ============================================================

def debug_dump(obj, message=""):
    """Dump an object to logs for debugging"""
    logger = logging.getLogger('smartlock.debug')
    
    import pprint
    output = pprint.pformat(obj, indent=2, width=120)
    
    if message:
        logger.debug(f"{message}\n{output}")
    else:
        logger.debug(f"Object dump:\n{output}")
