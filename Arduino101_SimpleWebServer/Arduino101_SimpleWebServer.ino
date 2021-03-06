/*
 Arduino101_SimpleWebServer

 Use your Arduino/Genuino 101 with the Ethernet Shield to create a
 simple Web Server

 Hardware:
 * Arduino/Genuino 101
 * Ethernet Shield

 created 10 Sept 2016
 by Biagio Montaruli
 updated 17 August 2017

 this code is in the public domain

 */
#include <SPI.h>
#include <Ethernet.h>

// MAC Address of Ethernet Shield
static byte MacAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Instatiate a new object that represent an Ethernet Server
// listening on port 80 for client requests
// In this way your Arduino 101 acts as an Ethernet Server!
unsigned int ethPort = 80;
EthernetServer server(ethPort);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// if useDHCP is set to true we use DHCP to get the IP address
// otherwise we set a static IP
bool useDHCP = true;

// chip select (Slave Select) for the SD card slot is pin 4
const uint8_t SdChipSelect = 4;

void setup() {
  // initialize Serial communication
  Serial.begin(9600);
  // wait for the Serial port to open. Needed for native USB only
  while(!Serial) ;
  // set the Slave Select pin for the SD card of the Ethernet Shield
  // as OUTPUT and turn HIGH the Slave Select pin in order to not
  // use the SD card
  pinMode(SdChipSelect, OUTPUT);
  digitalWrite(SdChipSelect, HIGH);
  
  // Initializes the ethernet library and network settings
  // Try to set up a new Ethernet connection and obtain a new IPv4 address
  // from the DHCP server. Passing only the MAC address of the
  // Ethernet Shield to begin() method use the DHCP server to obtain
  // the IP address of Genuino 101 with the Ethernet Shield.
  // The begin() method returns 1 on a successful DHCP connection, 
  // or 0 on failure.
  if((Ethernet.begin(MacAddress) == 1) && (useDHCP == true)) {
    Serial.println("New Ethernet connection successfully initialized with DHCP!");
    Serial.println("Printing info about the new connection...");
    Serial.print("IPv4 address of Genuino 101 Web Server: ");
    Serial.println(Ethernet.localIP());
    Serial.print("IPv4 address of DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.print("Gateway IPv4: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
  }
  else {
    Serial.println("Failed to configure a new Ethernet connection through DHCP !");
    Serial.println("Configuring the new connection using a static IPv4...");
    // using a static IPv4 : 192.168.1.25
    // NOTE : set the static IP according your local network configuration
    IPAddress staticIP(192, 168, 1, 25);
    Ethernet.begin(MacAddress, staticIP);
    Serial.print("Static IPv4 address of your Arduino 101 Web Server: ");
    for(uint8_t octet = 0; octet < 4; octet++) {
      Serial.print(staticIP[octet]);
      if(octet != 3) {
        Serial.print("."); 
      }
      else {
        Serial.println();
      }
    }
    Serial.print("Gateway IPv4 ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
  }
}

void loop() {
  // Arduino 101 (Server) listens for requests of new clients
  EthernetClient client = server.available();
  // if a new client wants to connect...
  if(client) {
    Serial.println("Getting the connection request from a new client...");
    bool detectBlankLine = true; 
    // while the client is connected to the Arduino 101 Web Server
    while (client.connected()) {

      // if the client has sent some data
      if (client.available()) {

        // read the incoming byte and print it in the Serial monitor
        char newData = client.read();
        Serial.write(newData);
        
        // the http request of the client ends when the server receives
        // a newline character followed by a blank line.
        // Then you can send the HTTP Response and a simple Web Page
        if((newData == '\n') && (detectBlankLine)) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          // Type of the content of the 
          client.println("Content-Type: text/html");
          // the connection will be closed after completion of the response
          client.println("Connection: close");
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Arduino 101 Web Server</title>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>Hello Arduino 101 Web Server !</h1>");
          client.println("<p>Simple Web server build with Arduino 101 and the Ethernet Shield</p>");
          client.println("</body>");
          client.println("</html>");
          // break connection with the client
          break;
        }

        if(newData == '\n') {
          detectBlankLine = true;
        }
        else if(newData != '\r') {
          detectBlankLine = false;
        }
      }
    }
    // client has been disconnected ...
    // wait for the browser to receive data send by Arduino 101 Server
    delay(5);
    // close the connection with the client
    client.stop();
    Serial.println("Client disconnected");
  }
}
