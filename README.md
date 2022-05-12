# Power-mon

This is an extremely rough-but-functional system I'm currently using to monitor power usage.

* collector:
    - runs on RPI
    - reads data from the Atmel power meter (ATM90E32) over SPI
    - Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-46003-SE-M90E32AS-Datasheet.pdf
    - Refernece library: https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/libraries/ATM90E32
    - Gathers up a cache of readings, publishing to MQTT broker every once in a while
    - Assume infinite memory if MQTT ever completely breaks down
    - Requires hand-calibration of each channel against known good (or acceptable) measurement (ideally a reference current, I just eye-balled against a cheap clamp-on hand-held meter)
* MQTT broker: Acts as ellastic storage/delivery between collector & aggregator
* aggregator:
    - Runs on server, subscribes to MQTT broker
    - Pushes data to SQL DB for long term storage (also considered infinite)
* frontend:
    - api/data.php runs on server, pulls from DB for web-app
    - index.html/js polls data.php for latest (inefficient, but works)
