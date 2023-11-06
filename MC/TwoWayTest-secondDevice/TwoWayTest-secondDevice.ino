/*WiFiAccessPoint.ino creates a WiFi access point and provides a web server on
  it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or
  http://192.168.4.1/L to turn it off OR Run raw TCP "GET /H" and "GET /L" on
  PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <esp_now.h>

#define LED_BUILTIN 2  // Set the GPIO pin where you connected your test LED or comment this \
     // line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "ESP32 c8:f0:9e:9b:88:7c";
const char *password = "Password";
WiFiServer server(80);

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = { 0x40, 0x22, 0xd8, 0x05, 0x2b, 0x98 };


// Define variables to store incoming readings
int incomingMessage;

// Variable to store if sending data was successful
String success;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int message;
} struct_message;

struct_message incomingPacket;
struct_message outgoingPacket;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  } else {
    success = "Delivery Fail :(";
  }
}
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingPacket, incomingData, sizeof(incomingPacket));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingMessage = incomingPacket.message;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
  // Set device as a Wi-Fi Station and access point
  WiFi.mode(WIFI_AP_STA);
  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");


  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is
  // received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  WiFiClient client = server.available();  // listen for incoming clients
  esp_err_t result;
  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine =
      "";                         // make a String to hold incoming data from the client
    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {   // if there's bytes to read from the
                                  // client,
        char c = client.read();   // read a byte, then
        Serial.write(c);          // print it out the serial monitor
        if (c == '\n') {          // if the byte is a newline character

          // if the current line is blank, you got two newline
          // characters in a row. that's the end of the client HTTP
          // request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g.
            // HTTP/1.1 200 OK) and a content-type so the client
            // knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print(
              "Click <a href=\"/H1\">here</a> to turn ON the "
              "LED.<br>");
            client.print(
              "Click <a href=\"/L1\">here</a> to turn OFF the "
              "LED.<br>");
            client.print(
              "Click <a href=\"/H2\">here</a> to turn ON the "
              "second LED.<br>");
            client.print(
              "Click <a href=\"/L2\">here</a> to turn OFF the "
              "second LED.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a
                                 // carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H2")) {
          //digitalWrite(LED_BUILTIN, HIGH);  // GET /H turns the LED on
          Serial.println("Local Turn ON 2)");
        }
        if (currentLine.endsWith("GET /L2")) {
          //digitalWrite(LED_BUILTIN, LOW);  // GET /L turns the LED off
        Serial.println("local Turn OFF 2)");
        }
        if (currentLine.endsWith("GET /L1")) {
          outgoingPacket.message = 10;
          result =
            esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket,
                         sizeof(outgoingPacket));
          if (result == ESP_OK) {
            Serial.println("Sent with success (Turn OFF 1)");
          } else {
            Serial.println("Error sending the data");
          }
        }
        if (currentLine.endsWith("GET /H1")) {
          outgoingPacket.message = 11;
          result =
            esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket,
                         sizeof(outgoingPacket));
          if (result == ESP_OK) {
            Serial.println("Sent with success (Turn ON 1)");
          } else {
            Serial.println("Error sending the data");
          }
        }
        if (incomingMessage == 20) {
          Serial.println("recieved command to turn OFF 2");
          incomingMessage = -1;
        } else if (incomingMessage == 21) {
          Serial.println("recieved command to turn ON 2");
          incomingMessage = -1;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
