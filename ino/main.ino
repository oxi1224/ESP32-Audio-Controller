#include <WiFi.h>
#include <HTTPClient.h>
#include "SPI.h"
#include <TFT_eSPI.h>
#include "AiEsp32RotaryEncoder.h"
#include <ArduinoJson.h>

#define B_PREV 25
#define B_PLAY 14
#define B_NEXT 13
#define B_DOWN 22
#define B_UP 17

#define EN_SW 26
#define EN_DT 21
#define EN_CLK 32

const char* SSID = "NAME";
const char* PASSWD = "PASSWORD";
const String SERVER_IP = "http://YOUR-LOCAL-IP:PORT";

int SELECTED = 0;

TFT_eSPI tft = TFT_eSPI();
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(EN_CLK, EN_DT, EN_SW, -1, 4);
DynamicJsonDocument GlobalDoc(1024);
DynamicJsonDocument SpotifyDoc(512);

long refreshLoop = 0;
long inputLoop = 0;
long volRefreshLoop = 0;
long s_refreshLoop = 0;
long s_drawLoop = 0;
bool drawSpotify = false;
bool spotifyRunning = false;

unsigned long trackDuration = 0;
unsigned long trackProgress = 0;
unsigned long startMillis = 0;
unsigned long lastUpdate = 0;

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

struct FoundDoc {
  const char* key;
  JsonVariant data;
};

FoundDoc findByIndex(int index, DynamicJsonDocument obj);
FoundDoc findByIndex(int index, DynamicJsonDocument obj) {
  int i = 0;
  FoundDoc retData;
  for (JsonPair kvp : obj.as<JsonObject>()) {
    if (index == i) {
      retData.key = kvp.key().c_str();
      retData.data = kvp.value();
      return retData;
    }
    i += 1;
  }
  return retData;
}

void getAudioData() {
  HTTPClient http;
  http.begin(SERVER_IP + "/get-processes");

  int code = http.GET();
  if (code > 0) {
    String payload = http.getString();
    deserializeJson(GlobalDoc, payload);
    JsonObject obj = GlobalDoc.as<JsonObject>();
    for (JsonPair kvp : obj) {
      JsonVariant data = kvp.value();
      if (data["name"].as<String>() == "Spotify.exe") {
        spotifyRunning = true;
        break;
      } else {
        spotifyRunning = false;
      }
    }
    Serial.println("[Audio-Server]: RESPONSE " + String(code));
  } else {
    Serial.println("[Audio-Server]: ERROR " + String(code));
  }
  http.end();
}

void getSpotifyStatus() {
  HTTPClient http;

  http.begin(SERVER_IP + "/spotify-status");
  int code = http.GET();
  if (code > 0) {
    String payload = http.getString();

    Serial.println("[Audio-Server-Spotify]: RESPONSE " + String(code));
    if (payload.isEmpty()) {
      SpotifyDoc["trackName"] = "ERROR";
      SpotifyDoc["trackArtist"] = "ERROR";
      SpotifyDoc["trackProgress"] = 0;
      SpotifyDoc["trackDuration"] = 600000;
    } else {
      deserializeJson(SpotifyDoc, payload);
    }
    
    trackDuration = SpotifyDoc["trackDuration"].as<int>();
    trackProgress = SpotifyDoc["trackProgress"].as<int>();
    startMillis = millis();
    drawSpotify = true;
  } else {
    Serial.println("[Audio-Server-Spotify]: ERROR " + String(code));
    drawSpotify = false;
  }
  http.end();
}

void execAction(int procIndex, String action) {
  FoundDoc data = findByIndex(procIndex, GlobalDoc);
  String pID = String(data.key);  
  HTTPClient http;
  String args = "?pID=" + pID + "&action=" + action;
  http.begin(SERVER_IP + "/exec-action" + args);
  int code = http.GET();
  if (code > 0) {
    Serial.println("[Audio-Server-Exec]: RESPONSE " + String(code));
  } else {
    Serial.println("[Audio-Server-Exec]: ERROR " + String(code));
  }
  http.end();
}

void setVol(int procIndex, float vol) {
  FoundDoc data = findByIndex(procIndex, GlobalDoc);
  String pID = String(data.key);
  HTTPClient http;
  String args = "?pID=" + pID + "&vol=" + String(vol);
  http.begin(SERVER_IP + "/set-vol" + args);

  int code = http.GET();
  if (code > 0) {
    Serial.println("[Audio-Server-Vol]: RESPONSE " + String(code));
    tft.fillRect(10, 17 + (32 * SELECTED), 60, 15, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(0);
    tft.drawCentreString(String((int)(vol * 100)) + "%", 40, 16 + (32 * SELECTED), 2);
  } else {
    Serial.println("[Audio-Server-Vol]: ERROR " + String(code));
  }
  http.end();
}

TFT_eSprite volSprite = TFT_eSprite(&tft);
void drawVol() {
  volSprite.fillSprite(TFT_BLACK);
  int vol = rotaryEncoder.readEncoder() * -1;
  volSprite.setTextSize(2);
  volSprite.setTextColor(TFT_WHITE);
  String text = "Curent Vol: " + String(vol) + "/100%";
  volSprite.drawCentreString(text, volSprite.width() / 2, 8, 1);
  volSprite.pushSprite(0, tft.height() - 32);
}

TFT_eSprite processSprite = TFT_eSprite(&tft);
void draw() {
  processSprite.fillSprite(TFT_BLACK);

  int i = 0;
  int size = 32;
  JsonObject obj = GlobalDoc.as<JsonObject>();

  int box_x0 = 0;
  int box_x1 = processSprite.width();
  int box_h = 32;
  int box_w = processSprite.width();
  int data_w = 80;

  for (JsonPair kvp : obj) {
    const char* key = kvp.key().c_str();
    JsonVariant data = kvp.value();

    int box_y0 = 32 * i;
    int box_y1 = box_y0 + 32;

    // TOP LINE
    processSprite.drawLine(box_x0, box_y0, box_x1, box_y0, i == SELECTED ? TFT_BLACK : TFT_WHITE);
    processSprite.setTextSize(0);

    // DATA BOX
    processSprite.fillRect(box_x0, box_y0, data_w, box_h, i == SELECTED ? TFT_BLACK : TFT_WHITE);
    // MID LINE
    processSprite.drawLine(box_x0 + 15, 16 + box_y0, data_w - 15, 16 + box_y0, i == SELECTED ? TFT_WHITE : TFT_BLACK);

    // BOT LINE WHITE
    processSprite.drawLine(box_x0, box_y1, box_x1, box_y1, i == SELECTED ? TFT_BLACK : TFT_WHITE);
    // BOT LINE BLACK
    if (i != 0) {
      processSprite.drawLine(box_x0, box_y0, (data_w - 1), box_y0, i == SELECTED ? TFT_WHITE : TFT_BLACK);
    }

    if (i == SELECTED) {
      processSprite.fillRect(data_w, box_y0, box_w, box_h, TFT_WHITE);

      processSprite.setTextColor(TFT_WHITE);
      processSprite.drawCentreString(key, (data_w / 2), box_y0, 2);

      processSprite.drawCentreString(data["vol"].as<String>() + "%", (data_w / 2), 16 + box_y0, 2);

      processSprite.setTextColor(TFT_BLACK);
      processSprite.setTextSize(2);
      processSprite.drawString(data["name"].as<String>(), (data_w + 5), box_y0 + 8, 1);
    } else {
      processSprite.setTextColor(TFT_BLACK);
      processSprite.drawCentreString(key, (data_w / 2), box_y0, 2);

      processSprite.drawCentreString(data["vol"].as<String>() + "%", (data_w / 2), 16 + box_y0, 2);

      processSprite.setTextColor(TFT_WHITE);
      processSprite.setTextSize(2);
      processSprite.drawString(data["name"].as<String>(), (data_w + 5), box_y0 + 8, 1);
    }

    processSprite.drawRect(0, 32 * SELECTED, box_w, 32 + 1, TFT_GREEN);
    processSprite.drawRect(1, (32 * SELECTED) + 1, box_w - 2, 32 - 1, TFT_GREEN);
    i += 1;
  }
  
  processSprite.pushSprite(0, 0);
}

TFT_eSprite spotifySprite = TFT_eSprite(&tft);
void drawSpotifyStatus() {
  spotifySprite.fillSprite(TFT_BLACK);

  int area_y0 = 0;
  int area_y1 = 96;
  int width = spotifySprite.width();
  spotifySprite.setTextSize(2);
  spotifySprite.setTextColor(TFT_GREEN);
  spotifySprite.drawCentreString(SpotifyDoc["artistName"].as<String>(), width / 2, area_y0 + 8, 1);
  spotifySprite.drawCentreString(SpotifyDoc["trackName"].as<String>(), width / 2, area_y0 + 40, 1);

  int percentDone = ceil((static_cast<float>(trackProgress) / static_cast<float>(trackDuration)) * 100.0);
  int done_w = ceil((width - 10) * (static_cast<float>(percentDone) / 100.0)); // 5px padding on each side

  spotifySprite.fillRect(5, area_y0 + 64, done_w, 32, TFT_GREEN);
  spotifySprite.fillRect(done_w + 5, area_y0 + 64, (width - done_w - 10), 32, TFT_WHITE);

  spotifySprite.pushSprite(0, tft.height() - 128);
}

void changeSelected(int change) {
  int objLen = 0;
  for (JsonPair kvp : GlobalDoc.as<JsonObject>()) {
    objLen += 1;
  }

  if ((SELECTED + change) < 0) {
    return;
  }
  if ((SELECTED + change) > (objLen - 1)) {
    return;
  }
  SELECTED += change;
  draw();
  refreshLoop = millis();
}

bool buttonStates[] = { 1, 1, 1, 1, 1, 1 };
int debounceDelay = 200;
unsigned long debounceTimes[] = { 0, 0, 0, 0, 0, 0 };
int buttonPins[] = { B_PREV, B_PLAY, B_NEXT, B_DOWN, B_UP, EN_SW };

void setup() {
  Serial.begin(115200);

  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(-100, 0, false);
  rotaryEncoder.setAcceleration(25);

  tft.begin();
  spotifySprite.createSprite(tft.width(), 96);
  processSprite.createSprite(tft.width(), tft.height() - 128);
  volSprite.createSprite(tft.width(), 32);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setViewport(0, 0, 240, 320);
  tft.setRotation(90);
  tft.drawCentreString("Connecting to WiFI", 120, 80, 3);

  WiFi.begin(SSID, PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  for (int i = 0; i < 5; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    delay(1000);
  }
  tft.fillScreen(TFT_BLACK);
  getAudioData();
  getSpotifyStatus();
  drawVol();
}

void loop() {
  if (rotaryEncoder.encoderChanged()) {
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());
    inputLoop = millis();
    drawVol();
  }

  for (int i = 0; i < 6; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (reading != buttonStates[i]) {
      if (millis() - debounceTimes[i] > debounceDelay) {
        buttonStates[i] = reading;
        if (reading == LOW) {
          switch (i) {
            case 0:
              // Prev
              Serial.println("[INPUT]: prev");
              execAction(SELECTED, "prev");
              if (spotifyRunning) {
                delay(150);
                getSpotifyStatus();
                drawSpotifyStatus();
                s_refreshLoop = millis();
                s_drawLoop = millis();  
              }
              break;
            case 1:
              // Play/Pause
              Serial.println("[INPUT]: play/pause");
              execAction(SELECTED, "pause");
              break;
            case 2:
              // Next
              Serial.println("[INPUT]: next");
              execAction(SELECTED, "next");
              if (spotifyRunning) {
                delay(150);
                getSpotifyStatus();
                drawSpotifyStatus();
                s_refreshLoop = millis();
                s_drawLoop = millis();  
              }
              break;
            case 3:
              // Down
              Serial.println("[INPUT]: down");
              changeSelected(1);
              inputLoop = millis();
              break;
            case 4:
              // Up
              Serial.println("[INPUT]: up");
              changeSelected(-1);
              inputLoop = millis();
              break;
            case 5:
              // Enc switch
              Serial.println("[INPUT]: encoder-sw");
              setVol(SELECTED, (rotaryEncoder.readEncoder() * -1) / 100.0);
              refreshLoop = millis();
            default:
              break;
          }
        }
        debounceTimes[i] = millis();
      }
    }
  }

  if ((millis() - refreshLoop) > 2000 && (millis() - inputLoop) > 500) {
    getAudioData();
    draw();
    refreshLoop = millis();
    inputLoop = 0;
  }

  if (spotifyRunning == true && drawSpotify == false && (millis() - s_drawLoop) > 2500) {
    getSpotifyStatus();
    drawSpotifyStatus();
    s_drawLoop = millis();
  }

  if (spotifyRunning == true && drawSpotify == true && (millis() - s_drawLoop) > 5000 && (millis() - inputLoop) > 500) {
    trackProgress = trackProgress + (millis() - startMillis);
    startMillis = millis();
    if (trackProgress >= trackDuration) {
      getSpotifyStatus();
      s_refreshLoop = millis();
    }
    drawSpotifyStatus();
    s_drawLoop = millis();
    inputLoop = 0;
  }

  if (spotifyRunning == true && drawSpotify == true && (millis() - s_refreshLoop) > 30000 && (millis() - inputLoop) > 500) {
    getSpotifyStatus();
    drawSpotifyStatus();
    s_refreshLoop = millis();
    s_drawLoop = millis();
    inputLoop = 0;
  }
}