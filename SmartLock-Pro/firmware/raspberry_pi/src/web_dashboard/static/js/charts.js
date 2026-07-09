// SmartLock Pro - Charts Configuration

class DashboardCharts {
    constructor() {
        this.charts = {};
    }
    
    createActivityChart(ctx, data) {
        this.charts.activity = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.labels || ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
                datasets: [{
                    label: 'Access Attempts',
                    data: data.values || [0, 0, 0, 0, 0, 0, 0],
                    borderColor: '#4e73df',
                    backgroundColor: 'rgba(78, 115, 223, 0.1)',
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        }
                    },
                    x: {
                        grid: {
                            display: false
                        }
                    }
                }
            }
        });
    }
    
    createMethodChart(ctx, data) {
        const colors = ['#4e73df', '#1cc88a', '#36b9cc', '#f6c23e', '#e74a3b'];
        
        this.charts.method = new Chart(ctx, {
            type: 'doughnut',
            data: {
                labels: data.labels || ['Fingerprint', 'RFID', 'Keypad', 'Mobile', 'Remote'],
                datasets: [{
                    data: data.values || [0, 0, 0, 0, 0],
                    backgroundColor: colors.slice(0, data.values.length),
                    hoverBackgroundColor: colors.slice(0, data.values.length).map(c => c + '99'),
                    borderWidth: 0
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            padding: 15,
                            usePointStyle: true,
                            pointStyle: 'circle'
                        }
                    }
                },
                cutout: '75%'
            }
        });
    }
    
    createDeviceChart(ctx, data) {
        this.charts.device = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: data.labels || ['Online', 'Offline', 'Maintenance'],
                datasets: [{
                    data: data.values || [0, 0, 0],
                    backgroundColor: ['#1cc88a', '#e74a3b', '#f6c23e'],
                    borderWidth: 0
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        }
                    },
                    x: {
                        grid: {
                            display: false
                        }
                    }
                }
            }
        });
    }
    
    updateChart(chartName, data) {
        if (this.charts[chartName]) {
            const chart = this.charts[chartName];
            
            if (data.labels) {
                chart.data.labels = data.labels;
            }
            
            if (data.values) {
                chart.data.datasets[0].data = data.values;
            }
            
            chart.update();
        }
    }
    
    destroyAll() {
        Object.values(this.charts).forEach(chart => {
            chart.destroy();
        });
        this.charts = {};
    }
}

// Initialize charts when DOM is ready
$(document).ready(function() {
    window.dashboardCharts = new DashboardCharts();
});
