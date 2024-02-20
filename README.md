# Description:

That is ESP based menergy meter monitor that reads pulse output (blinking red light on meter).

It builds consuption graph in Watts with one minute resolution and shows WiFi signal and battery level. It supports OTA for wireless updates.

If you have solar then check if your meter pulses only during energy import (some pulse during export as well).

# Connections:

Connect 4xAA battery holder to Vin and GND. That would power board for 10-20 hours.

Connect Arduino photoresistor/photodiode module output to D1 (change SENSOR_PIN in settings.h if needed) and module's Vin and GND to board 3V3 and GND. Adjust module's potentiometer to make it react to light. Photoresistor module seems to work well enough, but I used photodiode s it is faster and more sensetive.

# Setup:

Run

  cp secrets_example.h secrets.h

And add you WiFi and OTA creds to `secrets.h`.


Run before compilation and after every change in "index.html"

  python3 convert.py index.html

that will create `index.html.h` file with `char* index_html` variable.

Upload code to ESP, find out IP in router and open http://fill_esp_ip

# Common API endpoints:

	/

serves html


	/bin_seconds
	/impulse_per_kwh

see `settings.h` for BIN_SECONDS and IMPULSE_PER_KWH comments

	/data

accumulated data as space separated ints


	/wifi_signal
	/battery

WiFi and battery levels

Service endpoints:

	/impulse

shows state of SENSOR_PIN for a second at 1 msec resolution - use that to check your meter pulse duration

	/pin

digital state of SENSOR_PIN

	/durations

count of recieved pulses for durations in msec, maybe some meters would use pulse duration to differentiate solar/peak/off_peak?

	/reboot

reboots the board

# Utilities:

  python3 wifi_calibrate.py YOUR_ESP_IP

that will read `/wifi_signal` endpoint and show "min_value  current_value  max_walue". I used that to check `WiFi.RSSI()` return values.