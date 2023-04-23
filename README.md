# Irrigo

Irrigo is a garden irrigation system based on ESP32, it is Open Source and compatible with Home Assistant.

These are some interesting features of Irrigo:

 - WiFi connectivity
 - List item
 - Simple Web Interface for configuration
 - MQTT enabled
 - 4 Channels
 - Simple Web Firmware Update
 - Compatible with Home Assistant
 - Flexible
 - Supports 24V AC valves

The necessary hardware is a ESP32 board with at least 4 digital outputs, a relay board and power supply @ 24 VAC for valves.

To use it in Home Assistant it is important to have installed the MQTT integration, then in configuration.yaml add the following script:

    mqtt:
      switch:
      - name: "Channel 1"
        state_topic: "tele/irrigation/1"
        command_topic: "cmnd/irrigation/1"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 2"
        state_topic: "tele/irrigation/2"
        command_topic: "cmnd/irrigation/2"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 3"
        state_topic: "tele/irrigation/3"
        command_topic: "cmnd/irrigation/3"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 4"
        state_topic: "tele/irrigation/4"
        command_topic: "cmnd/irrigation/4"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch




