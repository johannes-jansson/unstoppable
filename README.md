# Unstoppable

![Picture of the current prototype](https://i.imgur.com/EtjV4e9.jpg)

Every morning, I need two pieces of information. 
Will it rain today? 
And are there delays in public transport? 
This is my solution to not have to look at my phone to get them, and therefore become unstoppable. 

The green light indicates that the device is online and information is up to date. 
The yellow light indicates that there's a risk of rain. 
The red light indicates that there's a risk of delays on my train commute. 

## Implementation

The ESP32 connects to the internet via wifi. 
It then makes a request to wttr.in to read forecasted precipitation for the day, and if it finds a value larger than the threshold it lights the yellow led. 
This only happens once per cycle, since weather forecasts usually don't change minute by minute. 
It proceeds to ask for any deviations for the train between my home station and my work station, and if it finds any it lights the red led. 
The green led is lit to indicate that the device is working properly and information is up to date. 
The device then waits three minutes, and repeats the fetching of train deviations. 
After one hour, the device is put into deep sleep mode to preserve battery. 
To activate the device again, hit the reset button. 

## To make this your own

- Get your own hardware and make a similar setup with LED:s. 
- Specify your wifi credentials on lines 11 & 12. 
- Tweak URL:s on lines 14 & 15 (may require some tinkering). 
- Tweak the timers in the loop function. 

## Instructions (for myself)
To flash, run:

```
pio run --target
```

To monitor, run:

```
pio device monitor
```

(On nixos, `sudo chmod a+rw /dev/ttyUSB0` might be needed occasionally to allow flashing and monitoring). 
