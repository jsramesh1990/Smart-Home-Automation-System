import React, { useState, useEffect } from 'react';
import {
    View,
    Text,
    StyleSheet,
    TouchableOpacity,
    ScrollView,
    Alert,
    SafeAreaView
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import LockControl from '../components/LockControl';
import AccessLogs from '../components/AccessLogs';
import GuestCodes from '../components/GuestCodes';
import VoiceControl from '../components/VoiceControl';

const HomeScreen = () => {
    const [lockStatus, setLockStatus] = useState('locked');
    const [logs, setLogs] = useState([]);
    const [guestCodes, setGuestCodes] = useState([]);
    
    const toggleLock = async () => {
        try {
            const newStatus = lockStatus === 'locked' ? 'unlocked' : 'locked';
            const response = await fetch(`${API_URL}/api/locks/toggle`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${userToken}`
                },
                body: JSON.stringify({ lockId: 'main', status: newStatus })
            });
            
            if (response.ok) {
                setLockStatus(newStatus);
                addLog({
                    action: newStatus === 'locked' ? 'LOCKED' : 'UNLOCKED',
                    user: 'Owner',
                    timestamp: new Date().toISOString()
                });
            }
        } catch (error) {
            Alert.alert('Error', 'Failed to toggle lock');
        }
    };
    
    const addGuestCode = (code, expiresIn) => {
        setGuestCodes([...guestCodes, {
            code,
            created: new Date(),
            expires: new Date(Date.now() + expiresIn),
            used: false
        }]);
    };
    
    return (
        <SafeAreaView style={styles.container}>
            <View style={styles.header}>
                <Text style={styles.title}>SmartLock Pro</Text>
                <Ionicons name="settings-outline" size={24} color="#333" />
            </View>
            
            <ScrollView>
                {/* Lock Control Section */}
                <LockControl
                    status={lockStatus}
                    onToggle={toggleLock}
                    autoLockDelay={30} // 30 seconds
                />
                
                {/* Voice Control Section */}
                <VoiceControl
                    onCommand={(command) => handleVoiceCommand(command)}
                    onVoiceRegister={() => navigation.navigate('VoiceSetup')}
                />
                
                {/* Guest Codes Section */}
                <GuestCodes
                    codes={guestCodes}
                    onAddCode={addGuestCode}
                    onRevokeCode={(codeId) => revokeGuestCode(codeId)}
                />
                
                {/* Access Logs Section */}
                <AccessLogs logs={logs} />
            </ScrollView>
        </SafeAreaView>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#f5f5f5'
    },
    header: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        padding: 20,
        backgroundColor: '#fff',
        borderBottomWidth: 1,
        borderBottomColor: '#e0e0e0'
    },
    title: {
        fontSize: 24,
        fontWeight: 'bold',
        color: '#333'
    }
});

export default HomeScreen;
