# Packet-Potato

The Packet Potato is a Wi-Fi packet analyzer! Or... is it a spectrum analyzer? Well, nobody really knows, but either way, it's the worst one of those two things, ever.

The Packet Potato uses an ESP8266 that listens for 802.11 frames on a specific channel. The channel is configurable by pressing the `+` and `-` buttons, and displayed on the 14-segment display. As it receives a frame, it blinks the appropriate LED's to indicate some things about the 802.11 frame that it heard.

The Packet Potato has five sets of indicators:
* DSSS - Blinks once whenever a data rate of 1, 2, 5.5, and 11 is heard
* OFDM - Blinks once whenever a data rate of 6, 9, 12, 18, 24, 36, 48, 54, or any MCS date is heard
* DATA - Blinks once whenever a data frame is heard
* MGMT - Blinks once whenever a management frame is heard
* CTRL - Blinks once whenever a control frame is heard

You can power the Packet Potato with either a 3.7v lipo battery, or microUSB. Just be careful not to power it with both at the same time... it probably would not end well. Also, be careful about polarity, as JST connectors don't have standardized polarity. If you get a battery from Adafruit, you should be good.

Note that the TX/RX pins on the WeMos D1 Mini are used to drive the CTRL and DATA LED's, so whenever serial is in use, those two LED's will illuminate. It won't hurt anything... we just ran out of pins on the WeMos.

# Flashing the WeMos D1 Mini

The heart of the Packet Potato is the WeMos D1 Mini, which is a breakout board for the ESP8266. The ESP8266 is the brains of the operation.

1. Get the Arduino software from https://arduino.cc/en/main/software.
2. Launch the Arduino software.
3. Go to `Arduino` > `Preferences`
4. In the `Additional Boards Manager URLs` field, paste or type this URL:
`http://arduino.esp8266.com/stable/package_esp8266com_index.json`
5. Click `Ok`
6. Go to `Tools` > `Board` > `Boards Manager…`
7. In the search box at the top, type `ESP8266`, and press enter.
8. When the installation is complete, click `Close`.
9. Windows Users Only: Download the CH350 driver (which is the serial adapter built into the WeMos D1 Mini): https://wiki.wemos.cc/downloads.
10. **Windows Users Only:** Extract the contents of the .zip file using Windows Explorer.
11. **Windows:** Using the Search function in the Windows Start menu, open up Device Manager.
12. **Windows:** Connect the WeMos D1 Mini to your computer via MicroUSB cable.
13. **Windows:** Find the new hardware device in the list, right-click it, and select `Update Device`.
14. **Windows:** Select Browse my computer for driver software, and point Windows towards the folder that you unzipped earlier.
15. **Windows:** Complete the driver installation.
16. Back in the Arduino application, go to `Tools` > `Ports`, and select `WeMos D1 R1` in the menu.
17. Go to `Tools` > `Ports`, and select the appropriate port *(Note: this is the easiest step to get wrong, so double-check that you've selected the right port. If something doesn't work, come back to this step and try another port.)*
18. Download the Packet Potato firmware from the releases page: https://github.com/PotatoFi/Packet-Potato/releases
18. Load the Packet Potato Arduino Sketch, compile, and upload.

# Usage

Apply power, and after a short startup animation, the Packet Potato will begin packet analysis! Or spectrum analysis. Which is it again?

## Display Modes

There are three display modes:
- **Channel**: Displays the current channel on screen (default)
- **Signal Strength**: Displays the signal strength of the last frame, in RSSI (negative number is implied)
- **Rate/MCS**: Displays the data rate or MCS of the last frame

To switch modes, long-press the `+` button. The DATA, CTRL, and DATA LEDs will blink to indicate which mode is selected.

- **DATA**: Channel
- **CTRL**: Signal Strength
- **MGMT**: Rate/MCS

## Serial Output

The Packet Potato will output some data about the frames it is hearing over serial at 115200 baud. Enable serial output mode by holding down `-` during the start-up animation, specifically while the red MGMT LED turns red.
