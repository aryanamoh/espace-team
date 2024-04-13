#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

String cmd1 = "";
String cmd2 = "";
volatile bool scheduleCmd1Send = false;
volatile bool scheduleCmd2Send = false;

String cmdRecvd = "";
const String waitingCmd = "Wait for cmds";
bool redrawCmdRecvd = false;

int progress = 0;
bool redrawProgress = true;
int level = 1;  // Current game level
int pointsForNextLevel = 25;  // Points required for next level upgrade

int lastRedrawTime = 0;
volatile bool scheduleCmdAsk = true;
hw_timer_t * askRequestTimer = NULL;
volatile bool askExpired = false;
hw_timer_t * askExpireTimer = NULL;
int expireLength = 25;  // Initial expire length, decreases with each level

#define ARRAY_SIZE 10
const String commandVerbs[ARRAY_SIZE] = {"Buzz", "Engage", "Floop", "Bother", "Twist", "Jingle", "Jangle", "Yank", "Press", "Play"};
const String commandNounsFirst[ARRAY_SIZE] = {"foo", "dev", "bobby", "jaw", "tooty", "wu", "fizz", "rot", "tea", "bee"};
const String commandNounsSecond[ARRAY_SIZE] = {"bars", "ices", "pins", "nobs", "zops", "tangs", "bells", "wels", "pops", "bops"};

int lineHeight = 30;

#define BUTTON_LEFT 0
#define BUTTON_RIGHT 35

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void updateLevel() {
  level++;
  expireLength = max(5, expireLength - 5);  // Decrease time limit by 5 seconds, minimum limit is 5 seconds
  timerAlarmWrite(askExpireTimer, expireLength*1000000, true);  // Update the timer with the new expiration length

  // Display level up message
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Level Up!", 10, 40, 2);
  delay(4000);  // Display message for 4 seconds

  // Clear the message and return to game view
  tft.fillScreen(TFT_BLACK);
  drawControls();  // Redraw game controls after clearing the screen
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
  buffer[msgLen] = 0;
  String recvd = String(buffer);

  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.printf("Received message from: %s \n%s\n", macStr, buffer);

  if (recvd[0] == 'A' && cmdRecvd == waitingCmd && random(100) < 30) {
    recvd.remove(0,3);
    cmdRecvd = recvd;
    redrawCmdRecvd = true;
    timerStart(askExpireTimer);
  } else if (recvd[0] == 'D' && recvd.substring(3) == cmdRecvd) {
    timerWrite(askExpireTimer, 0);
    timerStop(askExpireTimer);
    cmdRecvd = waitingCmd;
    progress += 1;
    if (progress % pointsForNextLevel == 0) {
      updateLevel();  // Update level every 25 points
    }
    broadcast("P: " + String(progress));
    redrawCmdRecvd = true;
  } else if (recvd[0] == 'P') {
    recvd.remove(0,3);
    progress = recvd.toInt();
    redrawProgress = true;
  }
}

void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message) {
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress)) {
    esp_now_add_peer(&peerInfo);
  }
  esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
}

void IRAM_ATTR sendCmd1(){
  scheduleCmd1Send = true;
}

void IRAM_ATTR sendCmd2(){
  scheduleCmd2Send = true;
}

void IRAM_ATTR onAskReqTimer(){
  scheduleCmdAsk = true;
}

void IRAM_ATTR onAskExpireTimer(){
  askExpired = true;
  timerStop(askExpireTimer);
  timerWrite(askExpireTimer, 0);
}

void espnowSetup() {
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }
  esp_now_register_recv_cb(receiveCallback);
  esp_now_register_send_cb(sentCallback);
}

void buttonSetup(){
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), sendCmd1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), sendCmd2, FALLING);
}

void textSetup(){
  tft.init();
  tft.setRotation(0);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  drawControls();
  cmdRecvd = waitingCmd;
  redrawCmdRecvd = true;
}

void timerSetup(){
  askRequestTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(askRequestTimer, &onAskReqTimer, true);
  timerAlarmWrite(askRequestTimer, 5*1000000, true);
  timerAlarmEnable(askRequestTimer);

  askExpireTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(askExpireTimer, &onAskExpireTimer, true);
  timerAlarmWrite(askExpireTimer, expireLength*1000000, true);
  timerAlarmEnable(askExpireTimer);
  timerStop(askExpireTimer);
}

void setup() {
  Serial.begin(115200);
  textSetup();
  buttonSetup();
  espnowSetup();
  timerSetup();
}

String genCommand() {
  String verb = commandVerbs[random(ARRAY_SIZE)];
  String noun1 = commandNounsFirst[random(ARRAY_SIZE)];
  String noun2 = commandNounsSecond[random(ARRAY_SIZE)];
  return verb + " " + noun1 + noun2;
}

void drawControls() {
  cmd1 = genCommand();
  cmd2 = genCommand();
  tft.drawString("B1: " + cmd1.substring(0, cmd1.indexOf(' ')), 0, 90, 2);
  tft.drawString(cmd1.substring(cmd1.indexOf(' ')+1), 0, 90+lineHeight, 2);
  tft.drawString("B2: " + cmd2.substring(0, cmd2.indexOf(' ')), 0, 170, 2);
  tft.drawString(cmd2.substring(cmd2.indexOf(' ')+1), 0, 170+lineHeight, 2);
}

void loop() {
  if (scheduleCmd1Send) {
    broadcast("D: " + cmd1);
    scheduleCmd1Send = false;
  }
  if (scheduleCmd2Send) {
    broadcast("D: " + cmd2);
    scheduleCmd2Send = false;
  }
  if (scheduleCmdAsk) {
    String cmdAsk = random(2) ? cmd1 : cmd2;
    broadcast("A: " + cmdAsk);
    scheduleCmdAsk = false;
  }
  if (askExpired) {
    progress = max(0, progress - 1);
    broadcast(String(progress));
    cmdRecvd = waitingCmd;
    redrawCmdRecvd = true;
    askExpired = false;
  }
  if (redrawCmdRecvd || redrawProgress) {
    tft.fillRect(0, 0, 135, 90, TFT_BLACK);
    tft.drawString(cmdRecvd.substring(0, cmdRecvd.indexOf(' ')), 0, 0, 2);
    tft.drawString(cmdRecvd.substring(cmdRecvd.indexOf(' ')+1), 0, 0+lineHeight, 2);
    redrawCmdRecvd = false;
    
    if (progress >= 100) {
      tft.fillScreen(TFT_BLUE);
      tft.setTextSize(3);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);
      tft.drawString("GO", 45, 20, 2);
      tft.drawString("COMS", 20, 80, 2);
      tft.drawString("3930!", 18, 130, 2);
      delay(6000);
      ESP.restart();
    } else {
      tft.fillRect(15, lineHeight*2+5, 100, 6, TFT_GREEN);
      tft.fillRect(16, lineHeight*2+5+1, progress, 4, TFT_BLUE);
    }
    redrawProgress = false;
  }
}
