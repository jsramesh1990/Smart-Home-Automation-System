// SmartLock Pro - Main JavaScript

// Socket.IO Connection
let socket = null;

$(document).ready(function() {
    // Initialize Socket.IO
    initSocket();
    
    // Handle notifications
    window.toastr = {
        success: function(message) { showToast(message, 'success'); },
        error: function(message) { showToast(message, 'danger'); },
        info: function(message) { showToast(message, 'info'); },
        warning: function(message) { showToast(message, 'warning'); }
    };
});

function initSocket() {
    socket = io({
        transports: ['websocket', 'polling']
    });
    
    socket.on('connect', function() {
        console.log('WebSocket connected');
    });
    
    socket.on('disconnect', function() {
        console.log('WebSocket disconnected');
    });
    
    socket.on('device_status', function(data) {
        // Update device status in UI
        if (data.device_id) {
            updateDeviceUI(data);
        }
    });
    
    socket.on('access_event', function(data) {
        // Show access event notification
        const status = data.granted ? 'success' : 'danger';
        const message = `${data.user} ${data.granted ? 'granted' : 'denied'} access to ${data.device}`;
        showToast(message, status);
    });
}

function showToast(message, type = 'info') {
    const colors = {
        success: '#1cc88a',
        info: '#36b9cc',
        warning: '#f6c23e',
        danger: '#e74a3b'
    };
    
    const icon = {
        success: 'fa-check-circle',
        info: 'fa-info-circle',
        warning: 'fa-exclamation-triangle',
        danger: 'fa-times-circle'
    };
    
    const toast = `
        <div class="toast align-items-center text-white border-0 mb-2" 
             style="background-color: ${colors[type]};" 
             role="alert" aria-live="assertive" aria-atomic="true">
            <div class="d-flex">
                <div class="toast-body">
                    <i class="fas ${icon[type]} me-2"></i>${message}
                </div>
                <button type="button" class="btn-close btn-close-white me-2 m-auto" 
                        data-bs-dismiss="toast"></button>
            </div>
        </div>
    `;
    
    // Remove old toasts if too many
    const container = $('#toast-container');
    if (container.find('.toast').length > 5) {
        container.find('.toast:first').remove();
    }
    
    // Add toast
    container.append(toast);
    const toastElement = container.find('.toast:last');
    const bsToast = new bootstrap.Toast(toastElement, { delay: 5000 });
    bsToast.show();
}

function updateDeviceUI(data) {
    // Update device card if it exists
    const card = $(`.device-card[data-device="${data.device_id}"]`);
    if (card.length) {
        // Update status badge
        const statusBadge = card.find('.badge:first');
        if (data.status) {
            statusBadge.removeClass('bg-success bg-danger');
            statusBadge.addClass(data.status === 'online' ? 'bg-success' : 'bg-danger');
            statusBadge.html(`<i class="fas fa-${data.status === 'online' ? 'circle' : 'circle'} me-1"></i>${data.status}`);
        }
        
        // Update lock state
        const lockBadge = card.find('.badge:last');
        if (data.lock_state) {
            lockBadge.removeClass('bg-success bg-danger');
            lockBadge.addClass(data.lock_state === 'locked' ? 'bg-danger' : 'bg-success');
            lockBadge.html(`<i class="fas fa-${data.lock_state === 'locked' ? 'lock' : 'lock-open'} me-1"></i>${data.lock_state}`);
        }
    }
}

function subscribeToDevice(deviceId) {
    if (socket) {
        socket.emit('subscribe', { device_id: deviceId });
    }
}

function unsubscribeFromDevice(deviceId) {
    if (socket) {
        socket.emit('unsubscribe', { device_id: deviceId });
    }
}

// Utility functions
function formatDate(date) {
    return new Date(date).toLocaleString('en-US', {
        year: 'numeric',
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
}

function formatDuration(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    
    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m`;
    } else {
        return `${minutes}m`;
    }
}

function getStatusColor(status) {
    const colors = {
        'online': 'success',
        'offline': 'danger',
        'maintenance': 'warning',
        'error': 'danger'
    };
    return colors[status] || 'secondary';
}

function getLockIcon(state) {
    return state === 'locked' ? 'lock' : 'lock-open';
}

function getMethodIcon(method) {
    const icons = {
        'fingerprint': 'fingerprint',
        'rfid': 'id-card',
        'keypad': 'keyboard',
        'mobile': 'mobile-alt',
        'remote': 'globe'
    };
    return icons[method] || 'lock';
}
