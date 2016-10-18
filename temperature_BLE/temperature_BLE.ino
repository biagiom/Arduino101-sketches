// include all class and methods related to Arduino/Genuino 101 BLE
#include <CurieBLE.h>

// use Arduino/Genuino 101 as peripheral
BLEPeripheral Genuino101peripheral;

// Define a new service in order to get temperature value
// Use a custom random UUID
BLEService tempService("6F182000-D588-4779-A953-19799C1A112C");
// Define a characteristic attribute with property Read/Notify :
// In this way we can get temperature value every time the value
// read from the sensor is different from the previous value
BLEFloatCharacteristic tempChar("6F182002-D588-4779-A953-19799C1A112C", BLERead | BLENotify);

// the LM35 temperature sensor is connected to analog pin A0
const uint8_t sensorPin = A0;
// variable to store the prevoius temperature value
float oldTempValue;
long startTime;

void setup() {
  // start Serial comunication with 9600 bps baudrate
  Serial.begin(9600);
  // wait for opening the Serial port since Arduino/Genuino 101
  // has a native USB and uses virtual serial port
  while(!Serial) ;

  // get the start time
  startTime = millis();
  
  // Set the local name using for advertising
  // This name will appear in advertising packets
  // and can be used by remote devices to identify Genuino 101
  Genuino101peripheral.setLocalName("BleTemp");
  Genuino101peripheral.setAppearance(BLE_GAP_APPEARANCE_TYPE_GENERIC_COMPUTER);
  // Set the name of the BLE Peripheral Device
  Genuino101peripheral.setDeviceName("GENUINO 101");
  // set the UUID of temperature service
  Genuino101peripheral.setAdvertisedServiceUuid(tempService.uuid());

  // Add the new declarated attribute : 
  // Temperature Service and its characteristics
  Genuino101peripheral.addAttribute(tempService);
  Genuino101peripheral.addAttribute(tempChar);

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, LOW);
  // get temperature value :
  // ADC of Arduino/Genuino 101 has 10-bit resolution, so
  // analogRead() returns a value from 0 to 1024.
  // Using the following formula we can get the analog read value :
  // 3.30 : 1023 = Vout : analogReadvalue;
  // moreover according to the LM35 datasheet the LM35 temperature
  // sensor produces voltages from 0 to +1V using this
  // relationship between the temperature in Celsius degrees and
  // the Vout (Voltage output of LM35) : 1°C => 10mV (0.01 V)
  // We can get the temperature value using this formula :
  // 10mV : 1°C = Vout (V) : temp (°C)
  // temp = (Vout * 1°C) / 0.01V  ( 10mV = 0,01 V);
  // but we can rewrite the above equation in this way :
  // temp = (Vout * 1°C) * 100V since 1/0.01 V = 100 V
  // Furthermore Vout = (analogReadValue * 3.30) / 1023, 
  // so replacing the Vout expression in the first formula 
  // we obtain the temperature value :
  // temp = (3.30 * analogRead(sensorPin) * 100.00) / 1023.00
  oldTempValue = (3.30 * analogRead(sensorPin) * 100.00) / 1023.00;
  Serial.print("Initial temperature value : ");
  Serial.print(oldTempValue);
  Serial.println(" °C");
  
  // add the initial temperature value to Characteristic Value
  tempChar.setValue(oldTempValue);
  
  // start advertising BLE Temperature service
  Genuino101peripheral.begin();

  Serial.println("Starting Genuino 101 advertising service.");
  Serial.println("Waiting for connection with other BLE devices ...");
}

void loop() {
  // listen for BLE peripherals to connect:
  BLECentral centralDevice = Genuino101peripheral.central();

  // if a central is connected to peripheral (Genuino 101):
  if(centralDevice) {
    Serial.print("Connected to central BLE device with address : ");
    // print the central's MAC address:
    Serial.println(centralDevice.address());

    // while the central device is still connected to peripheral:
    while(centralDevice.connected()) {
      // if 400 milliseconds have passed, read the new sensor value
      // and send the last read value to the Central device
      long newTime = millis();
      if((newTime - startTime) > 400) {
        startTime = newTime;
        // update temperature value
        updateTempValue();
      }
    }
    Serial.print("Disconnected from central BLE device with address : ");
    Serial.println(centralDevice.address());
  }
}

void updateTempValue(void) {
  // get the new value to the temperature sensor
  float newTempValue = (3.30 * analogRead(sensorPin) * 100.00) / 1023.00;
  // if the new temperature value which is different from the previous one
  if(newTempValue != oldTempValue) {
    // print in the Serial Monitor the new temperature value
    Serial.print("New temperature value : ");
    Serial.print(newTempValue);
    Serial.println(" °C");
    // send the new value to the BLE Central device
    tempChar.setValue(newTempValue);
    // update the old temperature value
    oldTempValue = newTempValue;
  }
}

