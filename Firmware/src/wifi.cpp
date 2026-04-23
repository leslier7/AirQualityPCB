#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h> //Need to use for insecure local testing
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
//Might want to move to the HttpsOTAUpdate library at some point since its part of the official arduino library
#include <ezTime.h>
#include "globals.h"

//Global wifi client
WiFiClientSecure wifi_client;

String generateJsonPacket(float ch4, float h2s, float nox, float vocs, float temp, float humidity){
    JsonDocument statusData;
    statusData["device_id"] = device_id;
    //statusData["timestamp"] = UTC.now(); // Probably should attach timestamp on the backend rather than on device
    statusData["ch4"] = ch4;
    statusData["h2s"] = h2s;
    statusData["nox"] = nox;
    statusData["vocs"] = vocs;
    statusData["temp"] = temp;
    statusData["humidity"] = humidity;

    String output;
    serializeJson(statusData, output);
    return output;
}

int sendData(HTTPClient client, String endpoint, String data){

    String sever = server_name + endpoint;

    Serial.println("Machine status update");
    Serial.println("Starting HTTPS connection...");
    if (!client.begin(wifi_client, sever)) {
        Serial.println("Failed to start HTTPS connection");
        client.end();
        return -1;
    }
    Serial.println("Connected to the server: transmitting data");

    // TODO do we need any kind of auth
    // String authHeader = "Bearer " + jwt;
    // client.addHeader("Authorization", authHeader);
    client.addHeader("Content-Type", "application/json");

    //Posting
    int httpCode = client.POST(data);

    Serial.print("Status HTTP Code: ");
    Serial.println(httpCode);
    String responseBody = client.getString();
    Serial.println("Response body: " + responseBody);
    client.end();
    if(httpCode > 0){
        if(httpCode == 200){
            Serial.println("Status updated successfully");
            // digitalWrite(led_1, HIGH);
            // delay(100);
            // digitalWrite(led_1, LOW);
        }
        else if (httpCode == 401){
            Serial.println("Unauthorized");
            //authenticated = false;
        } else if(httpCode == 400){
            Serial.println("Bad request");
            //TODO check client and building assignment if bad request
        } else if(httpCode == 500){
            Serial.println("Internal server error");
        } else {
            Serial.println("Failed to post: Unknown error");
        }
        return httpCode;
    } else{
        Serial.println("Error on HTTP request");
    }
    return -1;
}