/*
 Arduino101_EnvironmentalWebDataLogger

 Use your Arduino/Genuino 101 with the Ethernet Shield and the DFRobot
 Accessory Shield as a powerful Web Environmental Monitor Data Logger.
 With Arduino/Genuino 101 + Ethernet Shield you can create a Web Server
 that sends to your Web Browser (Client) environmental data.
 Temperature and humidity data are read from the DHT11 sensor of
 Accessory Shield. Environmental data are also stored to the
 "datalog.txt" text file that is created on the SD card inserted in
 the SD slot of the Ethernet Shield.

 Hardware:
 * Arduino/Genuino 101
 * Ethernet Shield
 * DFRobot Accessory Shield

 created 10 Sept 2016
 by Biagio Montaruli

 this code is in the public domain

 */
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <AccessoryShield.h>

// uncomment the next line to print on the Serial Monitor the data written in the SD card
// #define VERIFY_DATA

// MAC Address of Ethernet Shield
static byte MacAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Instatiate a new object that represent an Ethernet Server
// listening on port 80 for client requests
// In this way Arduino/Genuino 101 acts as an Ethernet Server!
unsigned int ethPort = 80;
EthernetServer server(ethPort);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// if useDHCP is set to true we use DHCP to get the IP address
// otherwise we set a static IP
bool useDHCP = true;

// chip select (Slave Select) for the Wiznet W5100 ethernet module is pin 10
const byte EthChipSelect = 10;

// tempC -> temperature in Celsius degrees
// tempF -> temperature in Fareneith degrees
// tempK -> temperature in Kelvin degrees
// humidity -> relative humidity
float tempC, tempF, tempK, humidity;
int8_t dht11State;

// object and variables to get info about the SD card
Sd2Card card;
SdVolume volume;
SdFile root;
File dataloggerFile;
// chip select (Slave Select) for the SD card slot is pin 4
const byte SdChipSelect = 4;
// boolean variable to check if the SD card is present in SD slot
// and if it has been initialized
bool SdInitialized = false;

// variable to count how many times we have wrote the datalog.txt file
unsigned int writingCount = 0;
// variable to set how many times we can write the datalog.txt file
unsigned int maxWritings = 150;
// if updateData is set to true, we can write the new values of temperature
// and humidity on the SD card
bool updateData = false;

void setup() {
  // initialize Serial communication
  Serial.begin(9600);
  // wait for the Serial port to open. Needed for native USB only
  while(!Serial) ;

  // initialize the Accessory Shield library
  accessoryShield.begin();

  // clear the OLED display, otherwise we have have many dots
  // printed on the OLED display of Accessory Shield
  accessoryShield.clearOledDisplay();
  accessoryShield.oledPaint();

  // set the chip select pin of Wiznet W5100 as OUTPUT and turn the pin
  // HIGH to disable the SPI communication with the W5100
  pinMode(EthChipSelect, OUTPUT);
  pinMode(EthChipSelect, HIGH);
  
  // initialize the SD library
  Serial.println("Initializing SD card...");

  // print info about the SD card
  if (!card.init(SPI_FULL_SPEED, SdChipSelect)) {
    Serial.println("SD card initialization failed. Please check the following things :");
    Serial.println("* Check if the SD card is inserted");
    Serial.println("* Check if the SD card is formatted with a FAT16 or FAT32 filesystem");
    Serial.println("* Check if your wiring is correct including the chip select (SS) pin");
  } 
  else {
    Serial.println("Wiring is correct and a SD card is present.");
    SdInitialized = true;
    
    // print the type of card
    Serial.print("\nSD Card type: ");
    switch (card.type()) {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
      Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      // don't do anything more :
      SdInitialized = false;
    }
    else {
      // print the type and size of the first FAT-type volume
      uint32_t volumeSize;
      Serial.print("\nVolume type is FAT");
      Serial.println(volume.fatType(), DEC);
      Serial.println();

      volumeSize = volume.blocksPerCluster();    // clusters are collections of blocks
      volumeSize *= volume.clusterCount();       // we'll have a lot of clusters
      volumeSize *= 512;                         // SD card blocks are always 512 bytes
      // print the filesystem size in bytes
      Serial.print("Volume size (bytes): ");
      Serial.println(volumeSize);
      // print the filesystem size in Kbytes : 1 Kb = 1024 bytes
      Serial.print("Volume size (Kbytes): ");
      Serial.println(volumeSize/1024);
      // print the filesystem size in Mbytes : 1 Mb = 1024 Kb = 1024^2 bytes
      Serial.print("Volume size (Mbytes): ");
      Serial.println(volumeSize/pow(1024,2));
      // print the filesystem size in Gbytes : 1 Gb = 1024 Mb = 1024^2 Kb = 1024^3 bytes
      Serial.print("Volume size (Gbytes): ");
      Serial.println(volumeSize/pow(1024,3));


      Serial.println("\nFiles found on the card (name, date and size in bytes): ");
      Serial.println("Name\tDate\tSize");
      root.openRoot(volume);

      // list recursively all files in the card with date and size
      root.ls(LS_R | LS_DATE | LS_SIZE);

      // re-initialize the SD library and the SD card.
      // Needed to access to files on the SD card
      SD.begin(SdChipSelect);
    }
  }
  Serial.println();

  // Initializes the ethernet library and network settings
  // Try to set up a new Ethernet connection and obtain a new IP address
  // from the DHCP server. Passing only the MAC address of the
  // Ethernet Shield to begin() method use the DHCP server to obtain
  // the IP address of Genuino 101 with the Ethernet Shield.
  // The begin() method returns 1 on a successful DHCP connection, 
  // or 0 on failure.
  if((Ethernet.begin(MacAddress) == 1) && (useDHCP == true)) {
    Serial.println("New connection successfully initialized with DHCP!");
    Serial.println("Printing info about the new connection...");
    Serial.print("IP address of Genuino 101 Web Server: ");
    Serial.println(Ethernet.localIP());
    Serial.print("IPv4 address of DNS Server: ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.print("Gateway IPv4: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
  }
  else {
    Serial.println("Failed to configure a new Ethernet connection through DHCP!");
    Serial.println("Configuring the new connection using a static IPv4...");
    // using a static IP : 192.168.1.25
    // NOTE : set the static IP according your local ethernat configuration
    IPAddress staticIP(192, 168, 1, 25);
    Ethernet.begin(MacAddress, staticIP);
    Serial.print("IPv4 address of your Arduino/Genuino 101 Web Server: ");
    for(uint8_t octet = 0; octet < 4; octet++) {
      Serial.print(staticIP[octet]);
      if(octet != 3) {
        Serial.print("."); 
      }
      else {
        Serial.println();
      }
    }
    Serial.print("Gateway IPv4: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
  }
  Serial.println();

}

void loop() {
  
  // Arduino 101 (Server) listens for requests of new clients
  EthernetClient client = server.available();
  // if a new client wants to connect ...
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
        // a newline character followed by a blank line and then
        // you can send a response
        if((newData == '\n') && (detectBlankLine)) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          // Type of the content of the 
          client.println("Content-Type: text/html");
          // the connection will be closed after completion of the response
          client.println("Connection: close");
          // refresh the page automatically every 3 seconds
          client.println("Refresh: 3");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Genuino 101 Web Server</title>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>Temperature and Humidity Datalogger + Web Server</h1>");
          client.println("<p>Datalogger and Web Server realized with Genuino 101 + DFRobot Accessory Shield</p>");
          client.println("<h2>Info :</h2>");
          client.println("<p>Genuino 101 read temperature and humidity from the DHT11 sensor of Accessory Shield"
                         " and store the values in the text file Datalogger.txt on the SD card of Ethernet Shield</p>");
          // get temperature in Celsius degrees and huumidity from the DHT11 sensor
          dht11State = accessoryShield.getEnvironmentalData(tempC, humidity, CELSIUS);
          // convert temperature from Celsius to Fareneith 
          tempF = accessoryShield.convertTempCtoF(tempC);
          // convert temperature from Celsius to Kelvin
          tempK = accessoryShield.convertTempCtoK(tempC);
          // check if we have got valid data from the SD card
          if((dht11State != DHT11_DATA_READ) || (tempF == NAN) || (tempK == NAN)) {
            client.println("<p>Failed to get temperature and humidity from DHT11 sensor</p>");
            updateData = false;
          }
          // if we have read good data we can write it on the Sd card, so set the updateData variable to true
          else {
            updateData = true;
            // print temperature in Celsius. Fareneith and Kelvin and print also the relative humidity
            // in a table on the Web page of your Browser
            client.println("<table border=\"1\">");
            client.println("<caption>Environmental Data</caption>");
            client.println("<tr>");
            client.println("<td>Temperature (Celsius)</td>");
            client.print("<td>");
            client.print(tempC);
            client.println("</td>");
            client.println("</tr>");
            client.println("<tr>");
            client.println("<td>Temperature (Fareneith)</td>");
            client.print("<td>");
            client.print(tempF);
            client.println("</td>");
            client.println("</tr>");
            client.println("<tr>");
            client.println("<td>Temperature (Kelvin)</td>");
            client.print("<td>");
            client.print(tempK);
            client.println("</td>");
            client.println("</tr>");
            client.println("<tr>");
            client.println("<td>Relative humidity</td>");
            client.print("<td>");
            client.print(humidity);
            client.println("</td>");
            client.println("</tr>");
            client.println("</table>");
          }
          client.println("</body>");
          client.println("</html>");

          // if Sd card has beeen initialized && we read valid data from DHT11 sensor &&
          // if the number of times we can write the SD card is less than the value of maxWritings
          if(SdInitialized && updateData && writingCount <= maxWritings) {
            // Update data on the SD card
            if(SD.exists("datalog.txt")) {
              Serial.println("The datalog.txt file already exists !");
              Serial.println("Updating the file datalog.txt with the new environmental data");
              // since the datalog.txt file exists, we open the file in append and write mode
              dataloggerFile = SD.open("datalog.txt", O_APPEND | O_WRITE);
            }
            else {
              Serial.println("The datalogger.txt file doesn't exist.\nCreating a new file named datalog.txt ...");
              // else if the datalog.txt file doesn't exists, we create the file and we open it in write mode
              dataloggerFile = SD.open("datalog.txt", O_WRITE | O_CREAT);
            }
            // if the file has been opened correctly, write  the new temperature and humidity values in the file
            if(dataloggerFile) {
              writingCount++;
              dataloggerFile.print("Environmental measurement #");
              dataloggerFile.println(writingCount);
              dataloggerFile.print(tempC);
              dataloggerFile.print("\t");
              dataloggerFile.print(tempF);
              dataloggerFile.print("\t");
              dataloggerFile.print(tempK);
              dataloggerFile.print("\t");
              dataloggerFile.println(humidity);
              dataloggerFile.close();

              Serial.println("Environmental data successfully written on th SD card");

            // if VERIFY_DATA has been defined print on the Serial monitor all the data
            // written in the Sd card
            #ifdef VERIFY_DATA
              dataloggerFile = SD.open("datalog.txt", O_READ);
              if(dataloggerFile) {
                Serial.println("Data of datalog.txt file : ");
                while(dataloggerFile.available()) {
                  char val= dataloggerFile.read();
                  Serial.print(val);
                }
                Serial.println();
              }
            #endif  
            }
            else {
              Serial.println("Failed to write the environmental data to the datalog.txt file\n");
            }
          }
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
    // wait for the browser to receive data send by Genuino 101 Server
    delay(5);
    // close the connection with the client
    client.stop();
    Serial.println("Client disconnected");
    Serial.println();
  }
}
