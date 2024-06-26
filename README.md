# Enhanced Medibox: ESP32-based Medication Management System

This repository contains the implementation of an enhanced Medibox system using ESP32, designed to assist users in managing their medication schedules effectively. The project is part of the EN2853: Embedded Systems and Applications course.

## 🎯 Objective

The objective of this project is to design, implement, and verify an enhanced Medibox system that monitors light intensity, adjusts a shaded sliding window, and provides a user interface for customization.

## 🔑 Key Features

- 📡 ESP32-based implementation
- 💡 Light intensity monitoring using LDRs
- 🪟 Automated shaded sliding window control
- 📊 Real-time data visualization on Node-RED dashboard
- 🎛️ User-customizable settings for different medicines

## 🛠️ Hardware and Software Requirements

### Hardware:
- ESP32 development board
- 2x Light Dependent Resistors (LDRs)
- Servo motor
- OLED display
- Push button
- Temperature and humidity sensor

### Software:
- Arduino IDE
- Node-RED
- MQTT broker (test.mosquitto.org)

## 📁 Project Structure

- `Arduino/`: Contains the ESP32 code for the Medibox system
- `Node-RED/`: Contains the Node-RED flow as a JSON file
- `Documentation/`: Contains project documentation and reports

## 🎛️ Functionality

1. **Light Intensity Monitoring**
   - Measure light intensity using two LDRs
   - Display highest light intensity on Node-RED dashboard
   - Indicate which LDR detects the highest intensity

2. **Shaded Sliding Window Control**
   - Adjust servo motor angle based on light intensity
   - Implement dynamic light regulation algorithm

3. **User Interface**
   - Node-RED dashboard for real-time data visualization
   - Customizable settings for minimum angle and controlling factor
   - Preset options for common medicines

4. **Basic Medibox Features**
   - Time synchronization with NTP server
   - Alarm setting and management
   - Temperature and humidity monitoring

## 📊 Dashboard Features

- Gauge for real-time highest light intensity
- Plot for visualizing past light intensity variations
- Sliders for adjusting minimum angle and controlling factor
- Dropdown menu for selecting preset medicine options

## 🔗 System Architecture

The system uses MQTT for communication between the ESP32 and the Node-RED dashboard. The MQTT broker used is test.mosquitto.org.

## 📂 Implementation Steps

1. Set up the ESP32 development environment
2. Implement light intensity monitoring using LDRs
3. Program servo motor control based on the provided equation
4. Create Node-RED flow for dashboard and controls
5. Implement MQTT communication between ESP32 and Node-RED
6. Test and verify all functionalities

## 📚 References

- EN2853: Embedded Systems and Applications course materials
- ESP32 documentation
- Node-RED documentation
- MQTT protocol specification

  ---

This project enhances the basic Medibox system to provide advanced medication management features, including light sensitivity protection and customizable settings for different medicines.
