#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <string.h>
#include <stdlib.h>

const char* SSID = "";
const char* PASSWORD = "";

String connectToTelegram(String command) {
  IPAddress server(149, 154, 167, 220);
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  String message = "";
  if (https.begin(*client, "https://api.telegram.org/<token>/" + command)) {
    int httpCode = https.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        message = https.getString();
      }
    }
    else {
      Serial.printf("GET error: %s\n\r", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  return message;
}

String sendMessage(String chatId, String message) {
  return connectToTelegram("sendMessage?chat_id=" + chatId + "&text=" + message);
}

String sendMessage(String chatId, String message, String parseMode) {
  return connectToTelegram("sendMessage?chat_id=" + chatId + "&text=" + message + "&parse_mode=" + parseMode);
}

String lastUpdate = "";
String hlList[50][2];
int hlListCount = 0;

int getIndexInHlList(String userId) {
  // find index
  int index = 0;
  while (index < hlListCount) {
    if (hlList[index][0].equals(userId)) {
      break;
    }
    ++index;
  }
  return index; 
}

void getUpdates() {
  String response = lastUpdate == "" ? connectToTelegram("getUpdates?limit=1") : connectToTelegram("getUpdates?limit=1&offset=" + lastUpdate);
  if (response != "" && WiFi.status() == WL_CONNECTED) {
    DynamicJsonDocument doc(8192);
    deserializeJson(doc, response);
    String updateId = doc["result"][0]["update_id"];
    if (updateId != "null") {
      int updateIdInt = updateId.toInt() + 1;
      String updateIdString(updateIdInt);
      lastUpdate = updateIdString;
      String message = doc["result"][0]["message"]["text"];
      int chatIdIndex = response.indexOf("\"chat\":{\"id\":") + 13;
      String chatId = response.substring(chatIdIndex, chatIdIndex + 14);
      String userId = doc["result"][0]["message"]["from"]["id"];
      String userName = doc["result"][0]["message"]["from"]["first_name"];
      Serial.println("ID: " + lastUpdate + " Chat: " + chatId + " Message: " + message);
      message.replace("@<bot-name>", "");
      if (message.equals("/hi")) {
        sendMessage(chatId, "Hello!");
      }
      else if (message.equals("/hl")) {
        if (hlListCount > 48) {
          sendMessage(chatId, "Sorry, the HL List is full!");
        }
        else {
          int index = getIndexInHlList(userId);
          if (index < hlListCount) {
            sendMessage(chatId, "You're already enrolled in the list, " + userName + " :P");
          }
          else {
            hlList[hlListCount][0] = userId;
            hlList[hlListCount++][1] = userName;
            sendMessage(chatId, "Added " + userName + " to the list!");
          }          
        }
      }
      else if (message.equals("/nohl")) {
        int index = getIndexInHlList(userId);
        if (index < hlListCount) {
          // overwrite
          while (index < hlListCount) {
            hlList[index][0] = hlList[index + 1][0];
            hlList[index][1] = hlList[index + 1][1];
            ++index;
          } 
          --hlListCount;
          sendMessage(chatId, "Removed " + userName + " from the list!");
        }
        else {
          sendMessage(chatId, "You weren't enrolled in the first place :P");
        }
      }
      else if (message.equals("/spam")) {
        String response = "*" + userName + "* has started a game!%0AJoin up!%0A%0A";
        for (int i = 0; i < hlListCount; ++i) {
          Serial.println(response.concat("[" + hlList[i][1] + "](tg://user?id=" + hlList[i][0] + ")%0A"));
        }
        sendMessage(chatId, response, "Markdown");
      }
    }
  }
  else {
    Serial.println("Failed to update!");
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  delay(3000);
  Serial.print("Starting\n");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("WiFi connected!\n");
}

void loop() {
  getUpdates();
  delay(1500);
}
