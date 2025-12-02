import React, { useState, useEffect } from 'react';
import {
    View,
    Text,
    TouchableOpacity,
    StyleSheet,
    Alert
} from 'react-native';
import { Audio } from 'expo-av';
import * as Speech from 'expo-speech';
import { Ionicons } from '@expo/vector-icons';
import VoiceProcessor from '../services/VoiceProcessor';

const VoiceControl = ({ onCommand, onVoiceRegister }) => {
    const [isListening, setIsListening] = useState(false);
    const [recognizedText, setRecognizedText] = useState('');
    const [recording, setRecording] = useState(null);
    const [users, setUsers] = useState([]);
    
    const startListening = async () => {
        try {
            await Audio.requestPermissionsAsync();
            const { status } = await Audio.getPermissionsAsync();
            
            if (status !== 'granted') {
                Alert.alert('Permission needed', 'Microphone permission is required');
                return;
            }
            
            const recording = new Audio.Recording();
            await recording.prepareToRecordAsync(
                Audio.RECORDING_OPTIONS_PRESET_HIGH_QUALITY
            );
            await recording.startAsync();
            setRecording(recording);
            setIsListening(true);
            
            // Start speech recognition
            const processor = new VoiceProcessor();
            const result = await processor.recognizeSpeech();
            
            if (result) {
                setRecognizedText(result.text);
                processCommand(result.text);
            }
            
        } catch (error) {
            console.error('Error starting recording:', error);
        }
    };
    
    const stopListening = async () => {
        if (recording) {
            await recording.stopAndUnloadAsync();
            setRecording(null);
        }
        setIsListening(false);
    };
    
    const processCommand = (text) => {
        const command = text.toLowerCase();
        
        // Basic voice commands
        if (command.includes('lock the door') || command.includes('lock door')) {
            onCommand('LOCK');
        } else if (command.includes('unlock the door') || command.includes('open door')) {
            onCommand('UNLOCK');
        } else if (command.includes('check status')) {
            onCommand('STATUS');
        } else if (command.includes('create guest code')) {
            onCommand('GUEST_CODE');
        } else {
            Speech.speak("I didn't understand that command", {
                language: 'en',
                pitch: 1,
                rate: 0.8
            });
        }
    };
    
    const registerVoice = () => {
        Speech.speak("Please say your verification phrase three times", {
            language: 'en'
        });
        onVoiceRegister();
    };
    
    return (
        <View style={styles.container}>
            <View style={styles.header}>
                <Text style={styles.title}>Voice Control</Text>
                <TouchableOpacity onPress={registerVoice}>
                    <Ionicons name="person-add" size={24} color="#007AFF" />
                </TouchableOpacity>
            </View>
            
            <TouchableOpacity
                style={[styles.micButton, isListening && styles.listening]}
                onPress={isListening ? stopListening : startListening}
            >
                <Ionicons
                    name={isListening ? "mic-off" : "mic"}
                    size={32}
                    color="#fff"
                />
            </TouchableOpacity>
            
            {recognizedText ? (
                <Text style={styles.recognizedText}>
                    Recognized: "{recognizedText}"
                </Text>
            ) : null}
            
            <View style={styles.commandList}>
                <Text style={styles.sectionTitle}>Available Commands:</Text>
                <Text style={styles.command}>• "Lock the door"</Text>
                <Text style={styles.command}>• "Unlock the door"</Text>
                <Text style={styles.command}>• "Check status"</Text>
                <Text style={styles.command}>• "Create guest code"</Text>
            </View>
            
            {users.length > 0 && (
                <View style={styles.usersSection}>
                    <Text style={styles.sectionTitle}>Recognized Users:</Text>
                    {users.map((user, index) => (
                        <View key={index} style={styles.userItem}>
                            <Ionicons name="person-circle" size={24} color="#666" />
                            <Text style={styles.userName}>{user.name}</Text>
                        </View>
                    ))}
                </View>
            )}
        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        backgroundColor: '#fff',
        margin: 15,
        padding: 20,
        borderRadius: 15,
        shadowColor: '#000',
        shadowOffset: { width: 0, height: 2 },
        shadowOpacity: 0.1,
        shadowRadius: 4,
        elevation: 3
    },
    header: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: 20
    },
    title: {
        fontSize: 18,
        fontWeight: '600',
        color: '#333'
    },
    micButton: {
        alignSelf: 'center',
        backgroundColor: '#007AFF',
        width: 70,
        height: 70,
        borderRadius: 35,
        justifyContent: 'center',
        alignItems: 'center',
        marginVertical: 15
    },
    listening: {
        backgroundColor: '#FF3B30'
    },
    recognizedText: {
        textAlign: 'center',
        color: '#666',
        fontStyle: 'italic',
        marginVertical: 10
    },
    commandList: {
        marginTop: 20
    },
    sectionTitle: {
        fontWeight: '600',
        color: '#333',
        marginBottom: 8
    },
    command: {
        color: '#666',
        marginLeft: 10,
        marginVertical: 3
    },
    usersSection: {
        marginTop: 20,
        paddingTop: 15,
        borderTopWidth: 1,
        borderTopColor: '#e0e0e0'
    },
    userItem: {
        flexDirection: 'row',
        alignItems: 'center',
        marginVertical: 5
    },
    userName: {
        marginLeft: 10,
        color: '#333'
    }
});

export default VoiceControl;
