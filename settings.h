#ifndef _SETTINGS_H_
#define _SETTINGS_H_

// comment that out if you use board that cannot not read battery level with ADC, e.g. ESP-01
#define BATTERY_VOLTAGE

// check you energy meter - how many pulses it shows per kWh, use String(int) value
#define IMPULSE_PER_KWH "1600"

// pin that gets photoresistor/photodiode digital signal
// ESP Node MCU GPI5 (D1)   ESP-01 GPIO2 2
#define SENSOR_PIN 5

// set min/max ADC battery voltage readings to calibrate battery widget
#define BATTERY_0_PERCENT_VOLTAGE   2500
#define BATTERY_100_PERCENT_VOLTAGE 3000

//TODO
// filter out signals shorter than that
#define DEBOUNCE_MIN_MSEC -1

// change that to make graph less/more granular
#define BIN_SECONDS "60"

// how many bins of data we keep
#define DATA_LEN_BINS 2048

// ESP blinks it's LED for LED_BLINK_MSEC msec after it detects impulse
#define LED_BLINK_MSEC 100

// if "Power Saving" is on the WiFi will be turned on only for POWER_SAVING_ACTIVE seconds
// once every POWER_SAVING_INTERVAL seconds
#define POWER_SAVING_INTERVAL 300
#define POWER_SAVING_ACTIVE    30

#endif//_SETTINGS_H_
