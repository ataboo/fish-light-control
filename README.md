# Fish Light Control

ESP32 controlling the brightness of addressable LEDS depending on the time of day.  Intended as lighting for a fish tank.

## Hardware

- ESP32 dev kit
- WS2812B light strip
- SN74HCT04N HEX inverter

SN74HCT04N shifts the signal level from 3.3v to 5v by inverting twice.

## Install

Setup typical ESP-IDF environment (tested on IDF 4.2).  Menu options for `ESP-Fish-Light`.  Setup SSID/PW for time synchronizations. Set sunrise/set start/end in format `hhmm`.  Set timezone according to POSIX rules.  Set color of full brightness.

## Behaviour

Device will sleep until the start of sunrise or sunset, then wake every minute to update the color during the transitions.  WIFI will be activated on first start as time establishment is required, then after light update if 24 hours has passed since the last update.

Blue lights mean initial loading/time sync.  Red lights mean failed initial load.