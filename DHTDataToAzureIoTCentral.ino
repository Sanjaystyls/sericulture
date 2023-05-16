// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include "DHT.h"

#define DHTPIN 2

#define DHTTYPE DHT11   // DHT 11
int fan = D0;
int bulb = D1;  
int pump = D2;   

#define WIFI_SSID "sanjay"
#define WIFI_PASSWORD "sericulture"
 
const char* SCOPE_ID = "One009B4818";
const char* DEVICE_ID = "sfm1";
const char* DEVICE_KEY = "Crfj4Y4F4+ygKMy5vtq5jlnyVB011X+QMPVLUwtnI8E=";

DHT dht(DHTPIN, DHTTYPE);

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(fan, OUTPUT); 
  pinMode(bulb, OUTPUT); 
  pinMode(pump, OUTPUT); 

  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;  // set timer in the past to enable first telemetry a.s.a.p
  }
   dht.begin();
}

void loop() {

float h = dht.readHumidity();
float t = dht.readTemperature();

  if(t>25){       //read sensor signal
    digitalWrite(fan, HIGH);   // if sensor is LOW, then turn on
    digitalWrite(bulb, LOW);
  }
  else if (t<20){
    digitalWrite(fan, LOW);   // if sensor is LOW, then turn on
    digitalWrite(bulb, HIGH);
  }
  else{
    digitalWrite(fan, LOW);    // if sensor is HIGH, then turn off the led
    digitalWrite(bulb, LOW);
  }
  
  if(t>28){       //read sensor signal
    digitalWrite(pump, HIGH);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, LOW);
  }
  else{
    digitalWrite(pump, LOW);   // if sensor is LOW, then turn on
    //digitalWrite(bulb, HIGH);
  }

  if (isConnected) {

    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  // send telemetry every 10 seconds
      char msg[64] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;
      if (loopId++ % 2 == 0) {  // send telemetry
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Temperature\": %f}",
                       t);
        errorCode = iotc_send_telemetry(context, msg, pos);
        
        pos = snprintf(msg, sizeof(msg) - 1, "{\"Humidity\":%f}",
                       h);
        errorCode = iotc_send_telemetry(context, msg, pos);
          
      } else {  // send property
        
      } 
  
      msg[pos] = 0;

      if (errorCode != 0) {
        LOG_ERROR("Sending message has failed with error code %d", errorCode);
      }
    }

    iotc_do_work(context);  // do background work for iotc
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }

}
