# BioMidi

Dispositivo werable que coleta informações biológicas e interage como um controlador MIDI.
Permite uma nova interface musical baseado em biofeedback

## Form Factor

Bracelete

# Alimentação

* Bateria Lítio

  https://www.filipeflop.com/blog/alimentar-esp32-com-bateria/
  https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/
  https://www.filipeflop.com/produto/modulo-carregador-de-bateria-de-litio-tp4056/

## Sleep Modes
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html

# Comunicação

* MIDI por BLE

https://developer.android.com/guide/topics/connectivity/bluetooth-le?hl=pt-br
https://learn.sparkfun.com/tutorials/midi-ble-tutorial/all
https://www.novelbits.io/bluetooth-gatt-services-characteristics/


# Dados coletados

* Movimento
* Orientação
* Rotação
* Batimento cardiaco
* Oxigenação
* Temperatura
* Sons do Musculo
* Distancia entre os dispositivos, por RSSI, no caso de multiplos aparelhos

# Hardware

* Esp32-WROOM-32
* Acelerometro e Giroscópio - MPU-6050
* Carregador de bateria - TP4056
* Microfone I2S -  MEMS MSM261S4030H0
* Temperatura - Bmp280
* Frequencia cardíaca - Sensor optico
* Led RGB

## MPU

https://howtomechatronics.com/tutorials/arduino/arduino-and-mpu6050-accelerometer-and-gyroscope-tutorial/

# Interface

## Dispositivo

Led RGB para representar
* WS
* Conectado para configuração
* Nivel de bateria

Touch



## Desktop App
* Electron (GUI) + Python (MIDI)

# Infos

* sound frequencies that compose the muscle sound (from 1 Hz up to 140 Hz)


# I2S

https://diyi0t.com/i2s-sound-tutorial-for-esp32/
https://diyi0t.com/reduce-the-esp32-power-consumption/

https://www.youtube.com/watch?v=QREKVWaZLi4
https://github.com/ikostoski/esp32-i2s-slm

https://github.com/fakufaku/esp32-fft

# Libs

* BLE - MIDI
https://github.com/lathoub/Arduino-BLE-MIDI


# BLE

## Attribute Protocol (ATT)

ATT defines how a server exposes its data to a client and how this data is structured. There are two roles within the ATT:

  * Server:

    Exposes the data it controls or contains. Accepts incoming commands from a peer device and sends responses, notifications, and indications

  * Client:

    Interfaces with the server with the purpose of reading the server’s exposed data and/or controlling the server’s behavior

The data that the server exposes is structured as attributes:

  * Attribute type (Universally Unique Identifier or UUID):

    This is a 16-bit number (in the case of Bluetooth SIG-Adopted Attributes), or 128-bit number (in the case of custom attribute types defined by the developer, also sometimes referred to as vendor-specific UUIDs)

  * Attribute Handle:

    This is a 16-bit value that the server assigns to each of its attributes — think of it as an address.

  * Attribute Permissions:

    Permissions determine whether an attribute can be read or written to, whether it can be notified or indicated, and what security levels are required for each of these operations. These permissions are not defined or discovered via the Attribute Protocol (ATT) but rather defined at a higher layer (GATT layer or Application layer).

## The Generic Attribute Profile (GATT)

