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

The necessary hardware is a ESP32 board with at least 4 digital outputs, a relay board and power supply @ 24 VAC for valves. (go to the Wiki for more info)
This is a screenshot of the simple Web GUI:


![image](https://user-images.githubusercontent.com/7345884/233922321-c3b30910-b82b-4431-b5c7-5b97ca50ed94.png)

To use it in Home Assistant it is important to have installed the MQTT integration, then in configuration.yaml add the following script:

    mqtt:
      switch:
      - name: "Channel 1"
        state_topic: "tele/irrigation/1"
        command_topic: "cmnd/irrigation/1"
        availability_topic: "tele/irrigation/LWT"
        payload_available: "Online"
        payload_not_available: "Offline"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 2"
        state_topic: "tele/irrigation/2"
        command_topic: "cmnd/irrigation/2"
        availability_topic: "tele/irrigation/LWT"
        payload_available: "Online"
        payload_not_available: "Offline"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 3"
        state_topic: "tele/irrigation/3"
        command_topic: "cmnd/irrigation/3"
        availability_topic: "tele/irrigation/LWT"
        payload_available: "Online"
        payload_not_available: "Offline"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch
      - name: "Channel 4"
        state_topic: "tele/irrigation/4"
        command_topic: "cmnd/irrigation/4"
        availability_topic: "tele/irrigation/LWT"
        payload_available: "Online"
        payload_not_available: "Offline"
        qos: 1
        payload_on: "ON"
        payload_off: "OFF"
        retain: false
        icon: hass:switch

The final result in Home Assistant is shown here:

![image](https://user-images.githubusercontent.com/7345884/233920390-fd33751a-3d25-4059-ad20-167a53ce9c6d.png)




