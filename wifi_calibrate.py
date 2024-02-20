import requests
import sys
import time


mn, mx = 1_000_000, -1_000_000
while True:
	r = requests.get(f'http://{sys.argv[1]}/wifi_signal')
	if r.status_code != 200:
		print('no response')
		continue
	r = int(r.text.replace('%', ''))
	mn = min(mn, r)
	mx = max(mx, r)
	print(mn, r, mx)
	time.sleep(1)