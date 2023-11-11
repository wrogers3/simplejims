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


#define LED 26
#define closeGate1 10
#define openGate1 11
#define queryThis 22
#define closeGate2 20
#define openGate2 21
#define queryOther 12
#define ledPin 3
#define thisNode 2
#define otherNode 1
#define notConnected -1
#define reset -1
const long timeoutTime = 1000;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;

int g1 = notConnected;
int g2 = false;



String header;
// Set these to your desired credentials.
const char *ssid = "ESP32 c8:f0:9e:9b:88:7c (node 2)";
const char *password = "Password";
WiFiServer server(80);

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x40, 0x22, 0xd8, 0x05, 0x2b, 0x98};

// Define variables to store incoming readings
int incomingMessage = notConnected;

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
	Serial.print("\rLast Packet Send Status:\t");
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
	Serial.println(incomingPacket.message);
	incomingMessage = incomingPacket.message;
	esp_err_t result;
	if (incomingMessage == queryThis) {
		outgoingPacket.message = g2;
		result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket,
		                      sizeof(outgoingPacket));
		if (result == ESP_OK) {
			Serial.println("Query Response (ESP-NOW)" + String(g2));
		}
	}  // else if (incomingMessage == queryOther) {
	// 	outgoingPacket.message = g2;
	// }

	// if receive messsage from another device
	if (incomingMessage == openGate2) {
		Serial.println("RECEIVED: open Gate2 (ESP-NOW)");
		g2 = true;
		gateOpen(thisNode);
	}
	if (incomingMessage == closeGate2) {
		Serial.println("RECEIVED: close Gate2 (ESP-NOW)");
		g2 = false;
		gateClose(thisNode);
	}
}

void gateClose(int gateNum) {
	// check if gate is closed
digitalWrite (LED, LOW);
	Serial.println("Gate " + String(gateNum) + " motors moving (closing)");
	//
}

void gateOpen(int gateNum) {
	digitalWrite (LED, HIGH);
	Serial.println("Gate " + String(gateNum) + " motors moving (opening)");
}

void setup() {
	pinMode(LED, OUTPUT);

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
	} else {
		Serial.println("ESP_NOW initialized");
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

	// ping other system to get status on startup
	// close own gate
	Serial.println("INITIALIZATION: gate(s) on this Node CLOSING");
	gateClose(thisNode);
	g2 = false;
	esp_err_t result;
	outgoingPacket.message = queryOther;
	Serial.println("INITIALIZATION: Pinging other nodes for gate information");
	result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket,
	                      sizeof(outgoingPacket));
	bool flag = false;
	if (success == "Delivery Fail :(") {
		// mark as not connected
		g1 = notConnected;
		Serial.println("ERROR: Unable to establish connection!");
		// continue
	} else {
		currentTime = millis();
		previousTime = currentTime;
		while ((currentTime - previousTime) <= timeoutTime) {
			currentTime = millis();
			// wait
			if (incomingMessage == true || incomingMessage == false) {
				flag = true;
				Serial.println(
				    "INITIALIZATION: received information about other gate(s)");
				g1 = incomingMessage;
				incomingMessage = reset;
				break;
			}
		}
		if (flag == false) {
			Serial.println("Did not receive Response");
		}
	}
}

void loop() {
	WiFiClient client = server.available();  // listen for incoming clients
	esp_err_t result;
	if (client) {     
				currentTime = millis();
		previousTime = currentTime;                   // if you get a client,
		Serial.println("New Client.");  // print a message out the serial port
		String currentLine = "";

		// make a String to hold incoming data from the client
		while (client.connected() &&
		       currentTime - previousTime <=
		           timeoutTime) {  // loop while the client's connected
			currentTime = millis();
			if (client.available()) {  // if there's bytes to read from the//
				                       // client,// read a byte, then

				char c = client.read();
				// Serial.write(c);
				header += c;
				if (c == '\n') {  // if the byte is a newline character
					// if the current line is blank, you got two newline
					// characters in a row. that's the end of the client HTTP
					// request, so send a response:
					if (currentLine.length() == 0) {
						// HTTP headers always start with a response code (e.g.
						// HTTP/1.1 200 OK) and a content-type so the client
						// knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();

						// response to query of gate 1
						// if (incomingMessage == true) {
						// 	Serial.println("query result: Gate is open\n");
						// 	incomingMessage = -1;
						// 	g2 = true;
						// } else if (incomingMessage == false) {
						// 	Serial.println("query result: Gate is closed\n");
						// 	incomingMessage = -1;
						// 	g2 = false;
						// }

						// Opens gate 1
						if (header.indexOf("GET /gate1/open") >= 0) {
							header = "";
							Serial.println(
							    "Attempting to establish ESP-NOW connection");
							outgoingPacket.message = openGate1;
							result = esp_now_send(broadcastAddress,
							                      (uint8_t *)&outgoingPacket,
							                      sizeof(outgoingPacket));
							if (success == "Delivery Success :)") {
								Serial.println(
								    " COMMAND: open Gate1 (ESP-NOW)");
								g1 = true;
							} else {
								Serial.println(
								    "Unable to establish ESP-NOW connection "
								    "(open gate 1)");
								g1 = notConnected;
							}

						} else if (header.indexOf("GET /gate1/close") >= 0) {
							header = "";

							Serial.println(
							    "Attempting to establish ESP-NOW connection");
							outgoingPacket.message = closeGate1;
							result = esp_now_send(broadcastAddress,
							                      (uint8_t *)&outgoingPacket,
							                      sizeof(outgoingPacket));
							if (success == "Delivery Success :)") {
								Serial.println(
								    " COMMAND: close Gate1 (ESP-NOW)");
								g1 = false;
							} else {
								Serial.println(
								    "Unable to establish ESP-NOW connection "
								    "(close gate 1)");
								g1 = notConnected;
							}
						} else if (header.indexOf("GET /gate2/open") >= 0) {
							header = "";
							Serial.println(
							    "RECEIVED: Open gate 2 (web server)");
							g2 = true;
							gateOpen(2);
						} else if (header.indexOf("GET /gate2/close") >= 0) {
							header = "";
							Serial.println(
							    "RECEIVED: Close gate 2 (web server)");
							g2 = false;
							gateClose(2);
						} else if (header.indexOf("GET /gate1/status") >= 0) {
							header = "";
							Serial.println("Trying to get gate 1 status");
							outgoingPacket.message = queryOther;
							result = esp_now_send(broadcastAddress,
							                      (uint8_t *)&outgoingPacket,
							                      sizeof(outgoingPacket));
							bool flag = false;
							if (success == "Delivery Fail :(") {
								// mark as not connected
								g1 = notConnected;
								Serial.println(
								    "ERROR: Unable to establish connection!");
								// continue
							} else {
								currentTime = millis();
								previousTime = currentTime;
								while ((currentTime - previousTime) <=
								       timeoutTime) {
									currentTime = millis();
									// wait
									if (incomingMessage == true ||
									    incomingMessage == false) {
										flag = true;
										Serial.println("Success!");
										g1 = incomingMessage;
										incomingMessage = reset;
									}
								}
								if (flag == false) {
									Serial.println("Did not receive Response");
									g1 = notConnected;
								}
							}

						} else if (header.indexOf("GET /gate2/status") >= 0) {
							header = "";
							Serial.println("Trying to get gate 2 status");
						}

						esp_err_t result;
						outgoingPacket.message = queryOther;
						Serial.println(
						    "Pinging other nodes for gate information");
						result = esp_now_send(broadcastAddress,
						                      (uint8_t *)&outgoingPacket,
						                      sizeof(outgoingPacket));
						bool flag = false;
						if (success == "Delivery Fail :(") {
							// mark as not connected
							g1 = notConnected;
							Serial.println(
							    "ERROR: Unable to establish connection!");
							// continue
						} else {
							currentTime = millis();
							previousTime = currentTime;
							while ((currentTime - previousTime) <=
							       timeoutTime) {
								currentTime = millis();
								// wait
								if (incomingMessage == true ||
								    incomingMessage == false) {
									flag = true;
									Serial.println(
									    "Received information about other "
									    "gate(s)");
									g1 = incomingMessage;
									incomingMessage = reset;
									break;
								}
							}
							if (flag == false) {
								Serial.println("Did not receive Response");
								g1 = notConnected;
							}
						}
						Serial.println("Making website now! g1 is " + String(g1));
						// Display the HTML web page
						client.println("<!DOCTYPE html><html>");
						client.println(
						    "<head><meta name=\"viewport\" "
						    "content=\"width=device-width, "
						    "initial-scale=1\">");
						client.println("<link rel=\"icon\" href=\"data:,\">");
						// CSS to style the on/off buttons
						// Feel free to change the background-color and
						// font-size attributes to fit your preferences
						client.println(
						    "<style>html { font-family: Roboto; display: "
						    "inline-block; margin: 0px auto; text-align: "
						    "center;}");
						client.println(
						    ".button { background-color: #4CAF50; border: "
						    "none; color: white; padding: 16px 40px;");
						client.println(
						    "text-decoration: none; font-size: 30px; "
						    "margin: "
						    "2px; cursor: pointer;}");
						client.println(
						    ".button2 {background-color: "
						    "#555555;}</style></head>");

						// Web Page Heading
						client.println("<body><h1>Simple Jim's</h1>");

						// Display current state, and ON/OFF buttons for
						// GPIO 26
						String state;
						if (g1 == true) {
							state = "Open";
						} else if (g1 == false) {
							state = "Closed";
						} else if (g1 == notConnected) {
							state = "Not Connected";
						}
						client.println("<p>Gate 1 - State " + state + "</p>");
						// If the output26State is off, it displays the ON
						// button
						if (state == "Closed") {
							client.println(
							    "<p><a href=\"/gate1/open\"><button "
							    "class=\"button\">OPEN</button></a></p>");
						} else if (state == "Open") {
							client.println(
							    "<p><a href=\"/gate1/close\"><button "
							    "class=\"button "
							    "button2\">CLOSE</button></a></p>");
						} else {
							client.println(
							    "<p><a href=\"/gate1/status\"><button "
							    "class=\"button "
							    "button2\">Connect</button></a></p>");
						}

						if (g2 == true) {
							state = "Open";
						} else if (g2 == false) {
							state = "Closed";
						} else if (g2 == notConnected) {
							state = "Not Connected";
						}
						// Display current state, and ON/OFF buttons for
						// GPIO 27
						client.println("<p>Gate 2 - State " + state + "</p>");
						// If the output27State is off, it displays the ON
						// button
						if (state == "Closed") {
							client.println(
							    "<p><a href=\"/gate2/open\"><button "
							    "class=\"button\">OPEN</button></a></p>");
						} else if (state == "Open") {
							client.println(
							    "<p><a href=\"/gate2/close\"><button "
							    "class=\"button "
							    "button2\">CLOSE</button></a></p>");
						} else {
							client.println(
							    "<p><a href=\"/gate2/status\"><button "
							    "class=\"button "
							    "button2\">Connect</button></a></p>");
						}
						client.println("</body></html>");

						// The HTTP response ends with another blank line
						client.println();
						// Break out of the while loop
						break;
					} else {  // if you got a newline, then clear
						      // currentLine
						currentLine = "";
					}
				} else if (c != '\r') {  // if you got anything else but a
					                     // carriage return character,
					currentLine += c;    // add it to the end of the currentLine
				}
			}
		}
		// Clear the header variable
		header = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}
