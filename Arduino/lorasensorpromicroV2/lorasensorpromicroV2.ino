/****************************************************************************************
* File:     LoRaWAN_V31-htu.ino
* Author:   Maarten Westenberg
* Company: Things4U
* Website:  http://www.ideetron.nl/LoRa
* E-mail:   mw12554@hotmail.com
*
* Publishing temperature and humidity to thethingsnetwork
* 
* Based on the example program made by Gerben of Ideetron
* Changed and extended to support HTU21d temperature/humidity sensors
* running on an Arduino Pro-Mini
****************************************************************************************/

/****************************************************************************************
* Created on:         20-01-2015
* Supported Hardware: Arduino Pro-Mini board with RFM95
* 
* Description
* 
* Minimal Uplink for LoRaWAN
* 
* This code demonstrates a LoRaWAN connection on a Nexus board. This code sends a messege every minute
* on channel 0 (868.1 MHz) Spreading factor 7.
* On every message the frame counter is raised
* 
* This code does not include
* Receiving packets and handeling
* Channel switching
* MAC control messages
* Over the Air joining* 
*
* Firmware version: 1.0
* First version
* 
* Firmware version 2.0
* Working with own AES routine
* 
* Firmware version 3.0
* Listening to receive slot 2 SF9 125 KHz Bw
* Created seperate file for LoRaWAN functions
****************************************************************************************/

/*
*****************************************************************************************
* INCLUDE FILES
*****************************************************************************************
*/
#include <SPI.h>
#include "AES-128_V10.h"
#include "Encrypt_V30.h"
#include "LoRaWAN_V30.h"
#include "RFM95_V20.h"
#include "LoRaMAC_V10.h"
#include "Waitloop_V10.h"
#include "LowPower.h"

#include "Wire.h"


/*
*****************************************************************************************
* GLOBAL VARIABLES
*****************************************************************************************
*/

// Sensor
#include <SFE_BMP180.h>
  SFE_BMP180 pressure;      // Init Sensor(s)

#define ALTITUDE 1655.0
    
// This key is for thethingsnetwork
unsigned char NwkSkey[16] = {
  0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
  0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// If we want, we can add our own key here
unsigned char AppSkey[16] = {
  0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
  0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// Things4U has Device Addresses 0x02 0x02 0x04 0x??
//
unsigned char DevAddr[4] = {
  0x01, 0x01, 0x01, 0x01
};

void setup() 
{
   //Initialize the UART
  Serial.begin(115200);

  Serial.println("setup: Init SPI port");
   //Initialise the SPI port
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000,MSBFIRST,SPI_MODE0));
  
  //Initialize I/O pins
  pinMode(DS2401,OUTPUT);
  pinMode(MFP,INPUT);
  pinMode(DIO0,INPUT);
  pinMode(DIO1,INPUT); 
  pinMode(DIO5,INPUT);
  pinMode(DIO2,INPUT);
  pinMode(CS,OUTPUT);
  pinMode(LED,OUTPUT);

  digitalWrite(DS2401,HIGH);
  digitalWrite(CS,HIGH);

  WaitLoop_Init();
  
  //Wait until RFM module is started
  WaitLoop(20); 
  Serial.println("setup: RFM module started");
  digitalWrite(LED,HIGH);  
  
 // Initialize the sensor (it is important to get calibration values stored on the device).

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
}

void loop()
{
  char status;
  double T,P,p0,a;

  // Loop here getting pressure readings every 10 seconds.

  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  Serial.println();
  Serial.print("provided altitude: ");
  Serial.print(" meters, ");
  Serial.print(ALTITUDE*3.28084,0);
  Serial.println(" feet");
  
 status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.print(" deg C, ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");

   status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          // Print out the measurement:
          Serial.print("absolute pressure: ");
          Serial.print(P,2);
          Serial.print(" mb, ");
          Serial.print(P*0.0295333727,2);
          Serial.println(" inHg");

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: a = altitude in m.

          a = pressure.altitude(P,p0);
          Serial.print("computed altitude: ");
          Serial.print(a,0);
          Serial.print(" meters, ");
          Serial.print(a*3.28084,0);
          Serial.println(" feet");
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

  //delay(5000);  // Pause for 5 seconds.

#define msg 1
  
  // Floats are not defined in Arduino printf
  int it = (int) status;            // Make integer part
  int ft = (int) ((status - it)*10);      // Fraction. Has same sign as integer part
  if (ft<0) ft = -ft;           // So if it is negative make fraction positive again.
  int ih = (int) status;            // Make integer part
  int fh = (int) ((status - ih)*10);      // Fraction. Has same sign as integer part
  if (fh<0) fh = -fh;           // So if it is negative make fraction positive again.
  
// sprintf(msg,"{\"t\":\"%d.%d\",\"h\":\"%d.%d\"}");
  //sprintf(msg,"{\"t\":\"%d.%d\",\"h\":\"%d.%d\"}",it,ft,ih,fh);
  
  unsigned char Test = 0x00;

  unsigned char Sleep_Sec = 0x00;
  unsigned char Sleep_Time = 0x01;

  unsigned char Data_Tx[256];
  unsigned char Data_Rx[64];
  unsigned char Data_Length_Tx;
  unsigned char Data_Length_Rx = 0x00;

  //Initialize RFM module
  RFM_Init();

  //Construct data
  memcpy(Data_Tx, msg, strlen(msg));
  Data_Length_Tx = strlen(msg);

  Serial.print("loop: sending message: ");
  Serial.println(msg);
  
  Data_Length_Rx = LORA_Cycle(Data_Tx, Data_Rx, Data_Length_Tx);
  
  if(Data_Length_Rx != 0x00)
  {
      Test = Data_Rx[0];
    Serial.println("loop: Data received");
  }

  //delay(90000);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}
