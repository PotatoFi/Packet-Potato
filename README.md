# Packet-Potato

The Packet Potato is a Wi-Fi packet analyzer! Or... is it a spectrum analyzer? Well, nobody really knows, but either way, it's the worst one of those two things, ever.

The Packet Potato uses an ESP8266 that listens for 802.11 frames on a specific channel. The channel is configurable by pressing the "+" and "-" buttons, and displayed on the 14-segment display. As it receives a frame, it blinks the appropriate LED's to indicate some things about the 802.11 frame that it heard.

The Packet Potato has five sets of indicators:
* DSSS - Blinks once whenever a data rate of 1, 2, 5.5, and 11 is heard
* OFDM - Blinks once whenever a data rate of 6, 9, 12, 18, 24, 36, 48, 54, or any MCS date is heard
* DATA - Blinks once whenever a data frame is heard
* MGMT - Blinks once whenever a management frame is heard
* CTRL - Blinks once whenever a control frame is heard

So yeah... I guess it's more of a packet analyzer.

# Unfinished Code

There are a lot of features that are un-implemented on the Packet Potato.

* Data, Management, and Control Frame parsing are completely unimplemented
* I think I'm reading the bit that indicates data rate incorrectly
* Timing, as in when LED's are turned on and off, doesn't seem to be working correctly at all

There's a lot of work left to do!
