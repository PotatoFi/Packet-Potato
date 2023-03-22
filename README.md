# Packet-Potato

The Packet Potato is a Wi-Fi packet analyzer! Or... is it a spectrum analyzer? The answer is unclear, but either way, it's the worst one of those two things, ever.

The Packet Potato was originally created as a soldering project for a Deep Dive at the Wirless LAN Pros Conference. It is also intended to be a wearable electornic badge for WLPC, much like the digital badges that are common at infosec conferences.

## Modulation Indicators

The Packet Potato uses an ESP8266 to capture 802.11 frames. When it hears a frame, it looks at the data rate, which tells it which type of modulation was used. On a real spectrum analyzer, DSSS (Direct Sequence Spread Spectrum) makes a curve shape, and OFDM (Orthoganol Frequency Division Multiplexing) makes a flat table-top shape.

- 1, 2, 5.5, and 11 mbps are all 802.11b rates, and use DSSS, so the Packet Potato blinks the blue LEDs in a curve shape. 
- 6, 9, 12, 18, 24, 36, 48, and 54 mbps are 802.11g rates, and used OFDM, so the Packet Potato blinks the green LEDs in a flat table-top shape.
- 802.11n data rates also use OFDM, so they also blink the green LED's in a flat table-top shape.

In this way, the Packet Potato emulates the look of a spectrum analyzer.

## Frame Type Indicators

In addition to the DSSS and OFDM signatures, the Packet Potato indicates the current received frame type with one of the three frame type LEDs:

* DATA - Blinks once whenever a data frame is heard
* MGMT - Blinks once whenever a management frame is heard
* CTRL - Blinks once whenever a control frame is heard

## Switching Channels

By default, the Packet Potato starts analyzing packets - er, sorry, **frames** on channel 6, with the current channel number being visible on the display. Press the `-` and `+` buttons on either side of the display to switch channels.

## Display Modes

Long-press the `-` and `+` buttons to switch display modes.

1. **Channel**: Displays the current channel on screen (default).
2. **Live Data Rate/MCS**: Displays the data rate or MCS of the last frame.
3. **Retry Rate**: Displays the retry rate as a percentage, measuring the last 200 data frames.
4. **Live Signal Strength**: Displays the signal strength of the last frame, in RSSI (negative number is implied)

When switching modes, the DATA, CTRL, and DATA LEDs will blink, and the screen will show characters to indicate which mode is selected:

1. **All LEDs** and `ch`: Channel
3. **DATA** and `ra`: Live Rate/MCS
4. **MGMT** and `rr`: Retry Rate
4. **CTRL** and `St`: Live Signal Strength

### Channel Mode

Shows the current channel. Press the buttons to change it. Yawn.

### Live Data Rate

This is my favorite. In Live Data Rate mode, the Packet Potato shows the data rate or MCS ([Modulation Coding Scheme](https://mcsindex.com/)) of the last packet that it heard, in real-time. 802.11b and 802.11g data rates simply appear as a number, while 802.11n data rates appear as an MCS. To indicate that they are an MCS, the right decimal point also blinks, so you can tell if the frame was using an MCS or not.

In Live Data Rate mode, the `-` and `+` buttons adjust the channel (just like in Channnel mode).

Things to try:

- Look at what data rates the MGMT (Management) frames are transmitted at. The lowest one you see is likely your Minimum Basic Rate, e.g. the slowest rate on your network. This is typically an old rate like 1 or 11 to ensure backwards compatibibility.
- With a 2.4 GHz 802.11n device, get it to move some data (by changing a setting, starting a throughput test, or transmitting some data). You should see MCS rates. Single-stream clients in the 5 to 7 range are happy clients, and two-stream clients closer to 15 are examples of happy clients.

_Note: Due to a limitation in the display controller code, 5.5 Mbps will only appear on the screen as 5. In Serial Mode, it will appear correctly as 5.5._

### Retry Rate ###

The Packet Potato tracks the retry rate of the last 200 data frames that it hears, and displays the retry rate as a percentage. It uses a simple ring buffer to track how many of the frames were retries. Only data frames are included in the ring buffer.

The primary function of the Packet Potato is to blink the LEDs whenever a frame is heard. Because of persistence of vision, the Packet Potato will only blink every 60 milliseconds. For the purposes of blinking, any frames heard during a blink will be ignored. However, all data frames heard (whether they are blinked or not) are recorded in the ring buffer.

The retry rate is only calculated and updated on the display whenever a frame is blinked. but the update might not result in the number changing if the channel is relatively quiet. A busy channel should see a constantly-updating data frame.

_Note: In the future, the Retry Rate feature might be updated to use a sliding timespan like 10 or 30 seconds, instead of a simple 200 frames._

In Live Data Rate mode, the `-` and `+` buttons adjust the channel (just like in Channnel and Live Data Rate modes). Whenver the channel is changed, the ring buffer is reset.

### Live Signal Strength

Whoa, the display shows the signal strength in real-time! The screen will show the signal strength of the last frame blinked, in dBm. The negative is implied, of course, because you know... two characters. Is potato.

By default, the Packet Potato blinks for any frame above `-99 dBm` (which should be everything it can hear). You can set a cutoff signal strength, causing the Packet Potato to ignore any frames that are quieter than the cutoff that you configure.

Press `+` to increase the cutoff to higher signal strengths (remember that we are working in negatives, so it feels backward), and press `-` to decrease the cutoff to lower signal strengths. Each button press will change the threshold by `5 dB`. The changes will take effect in real-time, and persist when you switch to other modes. The configured number will blink 4 times, at which point the Packet Potato will return to showing you live signal strength.

Things to try:

- Try setting the Packet Potato to `-60 dBm`. It should only blink for APs and active clients in the same room.
- Try `-40 dBm`, and wave it near an AP or active client. It should only blink when you're very close to the AP or active client.
- If the Packet Potato doesn't blink, go back to Channel Mode and hop through the channels, one at a time. You should be able to figure out what channel your network is on in less than a minute!

## Serial Mode

The Packet Potato will output some data about the frames it is hearing over serial at 115200 baud. Enable serial output mode by holding down `-` around the time that "Potato" disappears. You don't need to hold down the button for anything other than that, but it also won't hurt anything to hold the button down for a long time during the startup process.

Note that the TX/RX pins on the WeMos D1 Mini are used to drive the CTRL and DATA LED's, so whenever serial is in use, those two LED's will illuminate. There weren't enough pins on the WeMos. Is potato.

# Assembly

[Step-by-step Assembly Instructions](https://docs.google.com/document/d/1kVgwYJ2VQNqdhtW-Op0LsA10GBszojh-9hCqgSDIf1k/edit?usp=sharing)

## General Assembly Instructions

- Nearly all components are soldered to the front of the board (the side that says "Packet Potato")
- The pin headers, JST battery connector, and power switch are installed on the back of the board (the side with Fry)
- The female pin headers are soldered directly to the Packet Potato, and the male headers are soldered to the WeMos D1 Mini
- The ESP8266 radio sheild antenna should face outward


## Power

You can power the Packet Potato with either a 3.7v lipo battery, or microUSB. Just be careful not to power it with both at the same time... it probably would not end well. Also, be careful about polarity, as JST connectors don't have standardized polarity. If you get a battery from Adafruit, you should be good.

## Display

The display is controlled with a MAX7219, which is [common and well-documented](https://www.analog.com/media/en/technical-documentation/data-sheets/max7219-max7221.pdf). It's the big integrated circuit on the front of the Packet Potato, above the 7-segment displays.

# Flashing the WeMos D1 Mini

The heart of the Packet Potato is the WeMos D1 Mini, which is a breakout board for the ESP8266. The ESP8266 is the brains of the operation.

## Arduino IDE 2.x

### macOS

1. Download and install the Ardunio IDE from https://arduino.cc/en/main/software.
2. Download and unzip the latest Packet Potato release from the releases page: https://github.com/PotatoFi/Packet-Potato/releases
3. In the Ardunio IDE, point to Ardunio IDE > Open and select Packet_Potato.ino from the unzipped files.
4. Click the **Select Board** dropdown, and click `Selet other board and port...`
5. In the search box under **Boards**, search for `LOLIN(WEMOS) D1 R2 & mini`
5. Under **Ports**, select `/dev/cu.usbserial-14340 Serial Port (USB)` or similar
6. Click **Tools > CPU Frequency > 160 MHz**
6. Click **Upload**

### Windows

Idk. It's probably similar to macOS, but you might have to get a CH340 driver or something.

## Ardunio 1.x

1. Download and install the Ardunio 1.0 software.
2. Launch the Arduino software.
3. Go to `Arduino` > `Preferences`
4. In the `Additional Boards Manager URLs` field, paste or type this URL:
`http://arduino.esp8266.com/stable/package_esp8266com_index.json`
5. Click `Ok`
6. Go to `Tools` > `Board` > `Boards Manager…`
7. In the search box at the top, type `ESP8266`, and press enter.
8. When the installation is complete, click `Close`.
9. **Windows Users Only**: Download the CH350 driver (which is the serial adapter built into the WeMos D1 Mini): https://www.wemos.cc/en/latest/ch340_driver.html
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
