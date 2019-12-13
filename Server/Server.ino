
/*
 *  ESP8266 Wi-Fi Server that can receive http data and send it via i2c to the ADAU1401 audio processor
 *  
 *  Board: NodeMCU 1.0 (ESP-12E) / 115200 
 *  
 */
#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Credentials.h"
const char *ssid = UN;
const char *password = PW;

ESP8266WebServer server(80);

#include "Sigma.h"
#include "Project_IC_1.h"
#include "Project_IC_1_REG.h"
#include "Project_IC_1_PARAM.h"

#include "index.h"
const int led = 13;

// Memory settings for eqband0-5, bassleft, bassright (with default values)
byte mem[8] = { 10, 10, 10, 10, 10, 10, 30, 30 };

void handleRoot() {
  String s = MAIN_page; 
  server.send(200, "text/html", s); 
}

void getEQ() {
  String message = "{\n";
  for (byte i=0; i<6; i++) {
    message += "\"band" + String(i) + "\":\"" + String(mem[i]) + "\"" + ((i<5) ? "," : "") + "\n";
  }  
  message += "}";
  server.send(200, "application/json; charset=utf-8", message);
}

void setEQ() {
  byte band = server.arg(0).toInt();  // band: 0-5
  byte eqval = server.arg(1).toInt(); // eqval: 0-21
  EEPROM.put(band, eqval);
  EEPROM.commit();
  mem[band] = eqval;
      
  Serial.print("EQ (band, value): ");
  Serial.print(band);
  Serial.print(", ");
  Serial.println(eqval);  
  
  writeSigmaRegisterEQ(DEVICE_ADDR_IC_1, PARAM_ADDR_IC_1 + MOD_MIDEQ1_ALG0_STAGE0_B0_ADDR, band, eqval);  // Base address for EQ is used

  String message = "EQ " + server.arg(0) + " changed"; 
  server.send(200, "text/plain", message);
}

void getBass() {
  String message = "{\n";
  message += "\"bassleft\":\"" + String(mem[6]) + "\",\n";
  message += "\"bassright\":\"" + String(mem[7]) + "\"\n";
  message += "}";
  server.send(200, "application/json; charset=utf-8", message);
}

void setBass() {
  writeSigmaRegisterBassGain(DEVICE_ADDR_IC_1, PARAM_ADDR_IC_1 + MOD_MULTIPLE1_ALG0_GAIN1940ALGNS1_ADDR, server.arg(0));  // Left
  writeSigmaRegisterBassGain(DEVICE_ADDR_IC_1, PARAM_ADDR_IC_1 + MOD_MULTIPLE1_ALG1_GAIN1940ALGNS2_ADDR, server.arg(1));  // Right

  mem[6] = server.arg(0).toInt();
  mem[7] = server.arg(1).toInt();
  EEPROM.put(6, mem[6]);
  EEPROM.put(7, mem[7]);
  EEPROM.commit();

  Serial.print("Bass gain changed: ");
  Serial.print(mem[6]);
  Serial.print(",");  
  Serial.println(mem[7]);
      
  String message = "Bass gain changed.";
  server.send(200, "text/plain", message);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  
  Serial.begin(115200);
  
  Wire.begin();
  // On boot load program and parameter data to the ADAU
  // Check the function "default_download_IC_1()" under in the (generated) "Project_IC_1.h". In this file the correct order is used.
  Serial.println("Writing Core Register R0...");
  Sigma_write_register( DEVICE_ADDR_IC_1, REG_COREREGISTER_IC_1_ADDR, REG_COREREGISTER_IC_1_BYTE, R0_COREREGISTER_IC_1_Default );
  Serial.println("Writing Program data...");
  Sigma_write_register_block( DEVICE_ADDR_IC_1, PROGRAM_ADDR_IC_1, PROGRAM_SIZE_IC_1, Program_Data_IC_1 );
  Serial.println("Writing Parameter data...");
  Sigma_write_register_block( DEVICE_ADDR_IC_1, PARAM_ADDR_IC_1, PARAM_SIZE_IC_1, Param_Data_IC_1 );
  Serial.println("Writing Core Register R3...");
  Sigma_write_register( DEVICE_ADDR_IC_1, REG_COREREGISTER_IC_1_ADDR , R3_HWCONFIGURATION_IC_1_SIZE, R3_HWCONFIGURATION_IC_1_Default );
  Serial.println("Writing Core Register R4...");
  Sigma_write_register( DEVICE_ADDR_IC_1, REG_COREREGISTER_IC_1_ADDR, REG_COREREGISTER_IC_1_BYTE, R4_COREREGISTER_IC_1_Default );

  EEPROM.begin(sizeof(mem));
  for (byte i=0; i<sizeof(mem); i++) {
    EEPROM.get(i, mem[i]);   
    Serial.println(mem[i]); 
  } 

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/getEq", getEQ);  
  server.on("/setEq", setEQ);  
  server.on("/getBass", getBass);
  server.on("/setBass", setBass);  
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
