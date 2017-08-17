/*
 Use your Arduino/Genuino 101 to control sensors and actuators of
 DFRobot Accessory Shield though BLE.

 Hardware:
 * Arduino/Genuino 101
 * DFRobot Accessory Shield

 created 10 Sept 2016
 by Biagio Montaruli
 updated 18 August 2017

 this code is in the public domain
 */
#include <AccessoryShield.h>
#include <CurieBLE.h>

#define SKETCH_DEBUG

#define TEMP_UNIT DHT11_TEMP_CELSIUS
#define LED_NUMBER 3
#define BUZZER_VALUES 3
#define DHT11_VALUES 4

// create a new array setting the initial state of leds :
// when the sketch starts the red, green and blue leds are OFF
byte RGBledState[] = {0, 0, 0};
String ledColors[] = {"Red", "Green", "Blue"};

typedef struct {
  unsigned int freq;
  unsigned int delayTime;
  bool state;
} BuzzerData;
BuzzerData buzzer = {125, 2, false};
unsigned char buzzerValues[BUZZER_VALUES];

typedef struct {
  float temp;
  TemperatureUnit tempUnit;
  float humidity;
  float heatIndex;
} DHT11sensor;
DHT11sensor dht11;
unsigned char DHT11data[DHT11_VALUES];

unsigned int potValue;

String joystickInitialStatus, joystickNewStatus;
#if (defined(ARDUINO_ARCH_ARC32) || defined(__SAM3X8E__) || defined(ARDUINO_ARCH_SAMD))
#define MAX_DATA_LENGTH 13
#else
#define MAX_DATA_LENGTH 7
#endif

bool relayStatus;

unsigned long startTime, currentTime;

BLEService AccessoryShieldControl("6BC62B12-E4CE-41C3-9906-475A843245CA");

BLECharacteristic RGBled("6BC62B12-E4CE-41C3-9906-475A843245CA",
                         BLEWrite | BLERead,
                         sizeof(RGBledState));

BLECharacteristic DHT11("6BC62B15-E4CE-41C3-9906-475A843245CA",
                        BLERead | BLENotify,
                        sizeof(DHT11data));

BLECharacteristic Buzzer("6BC62B18-E4CE-41C3-9906-475A843245CA",
                         BLEWrite | BLERead,
                         sizeof(buzzerValues));

BLEUnsignedIntCharacteristic Pot("6BC62B1b-E4CE-41C3-9906-475A843245CA",
                                 BLERead | BLENotify);

BLECharacteristic Joystick("6BC62B1e-E4CE-41C3-9906-475A843245CA",
                            BLERead | BLENotify,
                            MAX_DATA_LENGTH);

BLEByteCharacteristic Relay("6BC62B21-E4CE-41C3-9906-475A843245CA",
                             BLERead | BLEWrite);

void setup() {
  // Initialize Serial communication at 115200 bps
  Serial.begin(115200);
  // wait for the serial port to connect, Needed for USB native only
  while (!Serial) ;
  // initialize the Accessory Shield library
  accessoryShield.begin();

  accessoryShield.clearOledDisplay();
  accessoryShield.oledPaint();
  
  // Starting CurieBLE library
  BLE.begin();

  BLE.setLocalName("AccessoryShield");
  BLE.setDeviceName("Genuino101");
  BLE.setAdvertisedService(AccessoryShieldControl);
  // BLE.setAppearance(BLE_GAP_APPEARANCE_TYPE_GENERIC_COMPUTER);
  BLE.setEventHandler(BLEConnected, BleConnectedCallback);
  BLE.setEventHandler(BLEDisconnected, BleDisconnectedCallback);

  // Add the RGBchar Characteristic to AccessoryShieldControl service
  AccessoryShieldControl.addCharacteristic(RGBled);
  RGBled.setValue(RGBledState, LED_NUMBER);
  RGBled.setEventHandler(BLEWritten, RGBWrittenCallback);

  dht11.tempUnit = TEMP_UNIT;
  dht11.temp = accessoryShield.getTemperature(dht11.tempUnit);
  dht11.humidity = accessoryShield.getHumidity();
  dht11.heatIndex = accessoryShield.computeHeatIndex(dht11.tempUnit);
#ifdef SKETCH_DEBUG
  Serial.print("\nTemperature: ");
  Serial.print(dht11.temp);
  Serial.println(getTempUnitSymbol(dht11.tempUnit));
  Serial.print("Humidity: ");
  Serial.println(dht11.humidity);
  Serial.print("Heat index: ");
  Serial.println(dht11.heatIndex);
#endif

  DHT11data[0] = dht11.temp;
  DHT11data[1] = dht11.tempUnit;
  DHT11data[2] = dht11.humidity;
  DHT11data[3] = dht11.heatIndex;

  // Add the DHT11 Characteristic to AccessoryShieldControl Service
  AccessoryShieldControl.addCharacteristic(DHT11);
  DHT11.setValue(DHT11data, DHT11_VALUES);

#ifdef SKETCH_DEBUG
  Serial.print("\nBuzzer initial frequency: ");
  Serial.println(buzzer.freq);
  Serial.print("Buzzer initial delay = ");
  Serial.println(buzzer.delayTime);
  Serial.print("Buzzer initial status: ");
  if (buzzer.state) {
    Serial.println("Activated (HIGH)");
  }
  else {
    Serial.println("Disabled (LOW)");
  }
#endif

  buzzerValues[0] = buzzer.freq;
  buzzerValues[1] = buzzer.delayTime;
  buzzerValues[2] = buzzer.state;

  // Add the Buzzer Characteristic to AccessoryShieldControl Service
  AccessoryShieldControl.addCharacteristic(Buzzer);
  Buzzer.setValue(buzzerValues, BUZZER_VALUES);
  Buzzer.setEventHandler(BLEWritten, BuzzerWrittenCallback);

  potValue = accessoryShield.readPot();
#ifdef SKETCH_DEBUG
  Serial.print("\nRotary potentiometer value: ");
  Serial.println(potValue);
#endif
  // Add the Pot Characteristic to AccessoryShieldControl Service
  AccessoryShieldControl.addCharacteristic(Pot);
  Pot.setValue(potValue);

  joystickInitialStatus = accessoryShield.getJoystickStateStr();
#ifdef SKETCH_DEBUG
  Serial.print("\nJoystick initial status: ");
  Serial.println(joystickInitialStatus);
#endif  
  // Add the Joystick Characteristic to AccessoryShieldControl Service
  AccessoryShieldControl.addCharacteristic(Joystick);
  Joystick.setValue(joystickInitialStatus.c_str());

  relayStatus = accessoryShield.getRelayState();
#ifdef SKETCH_DEBUG
  Serial.print("\nRelay initial status: ");
  if (relayStatus) {
    Serial.println("Activated (ON)");
  }
  else {
    Serial.println("Disabled (OFF)");
  }
#endif
  AccessoryShieldControl.addCharacteristic(Relay);
  Relay.setValue(relayStatus);
  Relay.setEventHandler(BLEWritten, RelayWrittenCallback);

  // Add the AccessoryShieldControl serivce to Attribute Table
  BLE.addService(AccessoryShieldControl);

  BLE.advertise();
  Serial.println("Start advertising, listening for central devices and waiting for connections ...");
}

void loop() {
  BLE.poll();
  
  updateAccessoryShieldData();
}

String getTempUnitSymbol(TemperatureUnit tempUnit) {
  String symbol = "°C";
  if (tempUnit == DHT11_TEMP_FARENEITH) {
    symbol.replace("C", "F");
  }
  else if (tempUnit == DHT11_TEMP_KELVIN) {
    symbol.replace("°C", "K");
  }
  return symbol;
}

void updateAccessoryShieldData(void) {
  int DHT11state = accessoryShield.getEnvironmentalData(dht11.temp, dht11.humidity, dht11.tempUnit);
  dht11.heatIndex = accessoryShield.computeHeatIndex(dht11.tempUnit);
#ifdef SKETCH_DEBUG
  Serial.print("Temperature = ");
  Serial.print(dht11.temp);
  Serial.println(getTempUnitSymbol(dht11.tempUnit));
  Serial.print("Humidity = ");
  Serial.println(dht11.humidity);
  Serial.print("Heat Index = ");
  Serial.println(dht11.heatIndex);
#endif
  if ((DHT11state == DHT11_DATA_READ) && (dht11.heatIndex != NAN)) {
    DHT11data[0] = dht11.temp;
    DHT11data[2] = dht11.humidity;
    DHT11data[3] = dht11.heatIndex;
    DHT11.setValue(DHT11data, DHT11_VALUES);
  }

  potValue = accessoryShield.readPot();
#ifdef SKETCH_DEBUG
  Serial.print("\nRotary potentiometer value: ");
  Serial.println(potValue);
#endif
  Pot.setValue(potValue);

  joystickNewStatus = accessoryShield.getJoystickStateStr();
#ifdef SKETCH_DEBUG
  Serial.print("\nJoystick status: ");
  Serial.println(joystickNewStatus);
#endif
  if (!joystickNewStatus.equals(joystickInitialStatus)) {
    Joystick.setValue(joystickNewStatus.c_str());
    joystickInitialStatus = String(joystickNewStatus);
  }

  relayStatus = accessoryShield.getRelayState();
#ifdef SKETCH_DEBUG
  Serial.print("\nRelay status: ");
  if (relayStatus) {
    Serial.println("Activated (ON)");
  }
  else {
    Serial.println("Disabled (OFF)");
  }
#endif
  Relay.setValue(relayStatus);
}

void BleConnectedCallback(BLEDevice central) {
  Serial.println("\nArduino 101 (Peripheral) connected to Central device with ");
  Serial.print(central.address());
  Serial.println(" Bluetooth device address.");
}

void BleDisconnectedCallback(BLEDevice central) {
  Serial.println("\nArduino 101 (Peripheral) disconnected from Central device with ");
  Serial.print(central.address());
  Serial.println(" Bluetooth device address.");
}

void RGBWrittenCallback(BLEDevice central, BLECharacteristic RGBchar) {
  const unsigned char *valuesUpdated = RGBchar.value();
  unsigned int dataLength = RGBchar.valueLength();
  Serial.print("\nCentral device has send ");
  Serial.print(dataLength);
  Serial.println(" values");
  if (dataLength == LED_NUMBER) {
    Serial.println("RGBled values updated:");
    for (byte led = 0; led < LED_NUMBER; led++) {
      RGBledState[led] = valuesUpdated[led];
      Serial.print(ledColors[led]);
      Serial.print(" led status : ");
      Serial.println(RGBledState[led]);
    }
    Serial.println("Updating RGB led status...");
    accessoryShield.setRGB(RGBledState[0], RGBledState[1], RGBledState[2]);
  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void BuzzerWrittenCallback(BLEDevice central, BLECharacteristic buzzerChar) {
  const unsigned char *valuesUpdated = buzzerChar.value();
  unsigned int dataLength = buzzerChar.valueLength();
  Serial.print("\nCentral device has send ");
  Serial.print(dataLength);
  Serial.println(" values");
  if (dataLength == BUZZER_VALUES) {
    Serial.println("Buzzer values updated by central device :");
    buzzer.freq = valuesUpdated[0];
    Serial.print("Buzzer frequency: ");
    Serial.println(buzzer.freq);
    buzzer.delayTime = valuesUpdated[1];
    Serial.print("Buzzer delay: ");
    Serial.println(buzzer.delayTime);
    Serial.print("Buzzer status: ");
    if (valuesUpdated[2]) {
      Serial.println("ON");
      buzzer.state = true;
      Serial.println("Playing the buzzer...");
      accessoryShield.playBuzzer(buzzer.freq, buzzer.delayTime);
    }
    else {
      Serial.println("OFF");
      buzzer.state = false;
      accessoryShield.buzzerOFF();
    }
  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void RelayWrittenCallback(BLEDevice central, BLECharacteristic relayChar) {
  const unsigned char *updatedValue = relayChar.value();
  relayStatus = (bool) *updatedValue;
  Serial.println("\nRelay status updated by central device");
  if (relayStatus) {
    Serial.println("Activating the relay...");
    accessoryShield.relayON();
  }
  else {
    Serial.println("Disabling the relay...");
    accessoryShield.relayOFF();
  }
}

