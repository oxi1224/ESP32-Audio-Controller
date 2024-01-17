# ESP32 Windows Audio Controller

## Table of Contents
1. [Description](#description)
2. [Physical Components](#physical-components)
3. [External Libraries](#external-libraries)
4. [Connections](#connections)
5. [Setup](#setup)
6. [Usage](#usage)
7. [Reference Image](#reference)
8. [License](#license)
9. [Authors](#authors)

## Description
ESP32 Windows Audio Controller uses a server running on the host PC to obtain data such as all running processes playing audio and the current track that is playing on spotify.
The esp32 captures that data to display it on the LCD screen and gives the ability to easily change it.

## Physical Components
- ESP32
- 320x240 LCD TFT with ILI9341 driver
- Encoder
- 5 buttons

## External Libraries

### C++ Libraries
- `cpp-httplib`: [https://github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib).
- `libcurl`: [https://github.com/curl/curl](https://github.com/curl/curl).
- `nlohmann json`: [https://github.com/nlohmann/json](https://github.com/nlohmann/json).

### Arduino Libraries
- `AiEsp32RotaryEncoder`: [https://github.com/igorantolic/ai-esp32-rotary-encoder](https://github.com/igorantolic/ai-esp32-rotary-encoder).
- `ArduinoJson`: [https://arduinojson.org/](https://arduinojson.org/).
- `TFT_eSPI by bodmer`: [https://github.com/Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI).

## Connections

### Buttons
Connect the following pins to the ESP32 (other side of each button is connected to GND):
- Previous Button: D25
- Pause Button: D14
- Next Button: D14
- Up Button: D22
- Down Button: D17

### Encoder
Connect the encoder to the ESP32 as follows:
- `GND`: GND
- `+`: 3.3V
- `SW`: D26
- `DT`: D21
- `CLK`: D32

### Screen
Wire the LCD TFT screen to the ESP32:
- `VCC`: 3.3V
- `GND`: GND
- `CS`: D15
- `RESET`: D4
- `DC`: D2
- `SDI (MOSI)`: D23
- `SCK`: D18
- `LED`: 3.3V

## Setup

### Spotify App
[Official Tutorial](https://developer.spotify.com/documentation/web-api/concepts/apps)
Save the following credentials somewhere safe:
- Client ID
- Client Secret
- Redirect URI (put: http://your-address:port/callback)

### Server
Replace clientId, clientSecret and redirectUri with the credentials from the spotify app.
Replace address with your local IP address.

##### Compiling
- Download [CMake](https://cmake.org/)
- I recommend to compile via [Visual Studio 2022](https://visualstudio.microsoft.com/pl/vs/) with the Windows APIs installed.
- I also recommend installing the necessary CPP libraries using [vcpkg](https://vcpkg.io/en/) (libhttp, nlohmann-json, curl)

### [Arduino IDE](https://www.arduino.cc/en/software)
- Open the provied .ino file
- Setup ESP32 if you have not ([tutorial](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/))
- Head over to the library manager and install ArduinoJson and AiEsp32RotaryEncoder
- You can find how to install and setup TFT_eSPI library in the github repo
- In the file, replace SSID with your WiFi network name, PASSWD with the password to your wifi and SERVER_IP with http://local-ip:port
- Upload the file to the ESP32
- (If the pins dont match up, tweak them to suit your model)

### [Node.JS](https://nodejs.org/en)
I've also included a ``js`` directory in the repository, it contains a simple javascript file to automatically verify on PC startup.
- Edit the ``auto_spotify_verify.js`` file and set the URL to your verification URL from [usage](#usage)
- Open CMD in the js directory and type ``npm i``
- Once it's finished press ``win`` + ``r`` and type ``shell:startup`` then move the shortcut to the ``.bat`` file in there.
- If you're doing this, also create a shortcut to the ``Server.exe`` file so the server starts first then the javascript script verifies it!

## Usage
After doing everything in [setup](#setup), you can run the ``Server.exe`` file that should be in the output directory.
If the file does not run, try to exclude it from Windows Defender as it sometimes tries to remove it.
You can then go to ``https://accounts.spotify.com/authorize?client_id=CLIENT_ID&response_type=code&redirect_uri=REDIRECT_URI&scope=user-read-playback-state%20user-read-currently-playing`` in your browser.
Make sure to change the needed credentials in the URL.
The ESP32 should now display the data on the LCD display.

## Reference
Reference image
![img](https://cdn.discordapp.com/attachments/1168285487790837843/1197261338129477652/IMG_20240117_202846.jpg?ex=65ba9f83&is=65a82a83&hm=2b404078f7f381379dadbba6c9f52bd2891011ce4ead9c186859e1c0d1ae28df&)

## License
[Attribution-NonCommercial 4.0 International License](LICENSE.md)

## Authors
- [Oxi1224](https://github.com/oxi1224)
