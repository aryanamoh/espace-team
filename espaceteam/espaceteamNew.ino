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
  Serial.print("
