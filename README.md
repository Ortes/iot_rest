# IOT Project REST

## Install

### ESP32
First refer to [Espressif documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) to install ESP32 SDK  
After setting up the shell you can run the following:
```shell script
git clone git@github.com:Ortes/iot_mqtt.git
cd iot_mqtt
idf.py flash && idf.py monitor
```
REST routes ESP32 side are:
- GET /led/on turn on the led
- GET /led/off turn off the led

REST routes server side are:
- POST /esp/temp push temperature

### Server

#### NodeRed
The port 1880 must be open.
```shell script
docker run -it -p 1880:1880 --name mynodered nodered/node-red
```

