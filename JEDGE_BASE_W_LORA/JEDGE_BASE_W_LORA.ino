/*This Code is for an ESP32 LoRa Transciever

Be aware of the devices address and the address being sent to

11/3/20 set the device address to 900 and sending to address 901
11/3/20 set the transmitting data to '5' and waiting for an incoming '1'
*/

#define BLYNK_PRINT Serial
#define BLYNK_USE_DIRECT_CONNECT

#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins
//HardwareSerial Serial1( 1 );
#define SERIAL1_RXPIN 16 // TO LORA TX
#define SERIAL1_TXPIN 17 // TO LORA RX

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "CL--V39yclPNHj0Ekmj-Q7XPhjPVawGZ";

BlynkTimer LoRaListen; // created a timer object called "LoraListen"

String received; // string used to record data coming in
String tokenStrings[5]; // used to break out the incoming data for checks/balances

int sendcounter = 0;
int receivecounter = 0;
//***********************************
//***** BASE UNIQUE DECLARATIONS ****
//***********************************
int Posession = 99; // Red, Yellow, Green, Blue
int PreviousPosession = 99; // same as above but used for checking turn overs
int Red = 0; // points
int Blu = 0; // points
int Grn = 0; // points
int Ylw = 0; // points
int MaxScore = 32700; // sets maximum integer value for scoring to around 8 hours
int GameTimer = 0; // sets game timer to zero
int GameMaxTime = 3700; // sets max game time to 3,700 seconds
int playercounter = 100; // used for score sync requests
bool ENABLESCORESYNC = false;
bool BYSHOTS = false; // default game scoring
bool BYTIME = true; // default game scoring
bool BYDAMAGE = false; // default game scoring
bool PLENTYOFTIME = true; // trigger to enable game functions
bool STARTTIMER = false; // trigger to run the game clock
bool NEWTAG = false; // used to enable blynk loop to update app with new tag info
bool TURNOVER = false; // used to enable blynk loop to transmit lora
bool POSTSCORES = false; // used to enable updating blynk app scores
bool IRDEBUG = false; // enable reporting to blynk app the protocol data for each shot received
int Mode = 0; // General Settings or Mode from Blynk App: Domination (default) = 1, Domination Slave = 2, Capture the Flag = 3, Loot Box = 4

// Define Variables used for the game functions for the BRX
int IRActivation = 1; // used for selecting the utility box activation method
int IRInterval = 0; // used for interval timing of IR emitter
int IRLastTime = 0; // used to store the last time emitter was used
int TeamAlignment = 99; // used to set utility box team alignment, default is 99 or null
int TagFunction = 1; // used for utility box function settings
int A[4]; // since this is device A we are using A to define the scores
int team=0; // this is for team recognition, team 1 = red, team 2 = blue, team 3 = green
int Power=0; // used for identifying the tag power bits
int gamestatus=0; // used to turn the ir reciever procedure on and off
int PlayerID=0; // used to identify player
int PID[6]; // used for recording player bits for ID decifering
int DamageID=0; // used to identify weapon
int Critical = 0; // used to identify tag
int ShotType=0; // used to identify shot type
int F4A[64]; // used for free for all scoring
int DID[8]; // used for recording weapon bits for ID decifering
int BID[4]; // used for recording shot type bits for ID decifering
const byte IR_Sensor_Pin = 16; // this is int input pin for ir receiver
int B[4]; // bullet type bits for decoding Ir
int P[6]; // Player bits for decoding player id
int T[2]; // team bits for decoding team 
int D[8]; // damage bits for decoding damage
int C[1]; // Critical hit on/off from IR
int U[2]; // Power bits
int Z[2]; // parity bits depending on number of 1s in ir
int X[1]; // bit to used to confirm brx ir is received
String BRXCMD; // used for commands sent to BRX
int UBID[10]; // used for communicating with other utility boxes
int IRledPin =  17;    // LED connected to digital pin 17
const int led = 2; // ESP32 Pin to which onboard LED is connected

BlynkTimer ScoreAccumulator; // created a timer object called "ScoreAccumulator"
BlynkTimer GameTimeAccumulator; // created a timer object called "GameTimeAccumulator"
BlynkTimer RequestScores; // used to request scores from taggers
//************************************
//**** BASE UNIQUE DELCARATIONS ******
//************************************

BLYNK_WRITE(V0) { // clear scores, start over
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) {
    ClearScores();
    Serial.println("cleared scores");
      if (Mode == 2) {
        TransmitStatus();
        Serial.println("Transmitted clear scores to slaves");
      }
  }
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V1) { // stop game, freeze points
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) {
    Posession = 99;
    Serial.println("cleared scores");
      if (Mode == 2) {
        TransmitStatus();
        Serial.println("Transmitted clear scores to slaves");
      }
  }
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V2) { // Modify Scoring Method
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // default mode for score by time
    BYTIME = true;
    BYSHOTS = false;
    BYDAMAGE = false;
    Serial.println("Domination Scoring set to Time");
  }
  if (b==2) { // default mode for score by time
    BYTIME = false;
    BYSHOTS = true;
    BYDAMAGE = false;
    Serial.println("Domination Scoring set to Shots");
  }
  if (b==3) { // default mode for score by time
    BYTIME = false;
    BYSHOTS = false;
    BYDAMAGE = true;
    Serial.println("Domination Scoring set to Damage");
  }
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V3) { // sets parameters for game limits for winning/ending a game
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // unlimited time but really around 8 hours
    MaxScore = 32700;
  }
  if (b==2) { // three minutes or 180 seconds/shots/damage
    MaxScore = 180;
  }
  if (b==3) { // 5 minutes or 300 seconds/shots/damage
    MaxScore = 300;
  }
  if (b==4) { // 10 minutes or 600 shots/seconds/damage
    MaxScore = 600;
  }
  if (b==5) { // 20 minutes or 1200 shots/seconds/damage
    MaxScore = 1200;
  }
  if (b==6) { // 40 minutes or 2400 shots/seconds/damage
    MaxScore = 2400;
  }
  if (b==7) { // 80 minutes or 4800 shots/seconds/damage
    MaxScore = 4800;
  }
  if (b==8) { // 9600 minutes or 600 shots/seconds/damage
    MaxScore = 9600;
  }
  Serial.println("Max Score set to: " + String(MaxScore));
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V4) { // Game Time Limit Setting
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // unlimited time but really around 8 hours
    GameMaxTime = 32700;
  }
  if (b==2) { // three minutes or 180 seconds/shots/damage
    GameMaxTime = 180;
  }
  if (b==3) { // 5 minutes or 300 seconds/shots/damage
    GameMaxTime = 300;
  }
  if (b==4) { // 10 minutes or 600 shots/seconds/damage
    GameMaxTime = 600;
  }
  if (b==5) { // 20 minutes or 1200 shots/seconds/damage
    GameMaxTime = 1200;
  }
  Serial.println("Game Time Limit set to: " + String(GameMaxTime));
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V5) { // Sets Device General Setting Function Mode
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // Set to Domination Standalone, this is already default
    Mode = 1;
  }
  if (b==2) { // Set to Domination Master Mode
    Mode = 2;
  }
  if (b==3) { // Set To Domination Slave Mode
    Mode = 3;
  }
  if (b==4) { // Set to Capture the Flag Mode
    Mode = 4;
  }
  if (b==5) { // Set to Utility Box Mode
    Mode = 5;
  }
  if (b==6) { // Set to BombSquad mode
    Mode = 6;
  }
  if (b==7) { // Set to Just Debug
    Mode = 7;
    IRDEBUG = true;
  }
  if (b==8) { // Set to Own The Zone mode
    Mode = 8;
    IRInterval = 5;
  }
  if (b==9) { // Set to JEDGE
    Mode = 9;
  }
  Serial.println("Mode Set To: " + String(Mode));
  digitalWrite(led, LOW);
}
BLYNK_WRITE(V6) { // Sets IR Debug Mode on/off
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // disable ir debug (default)
    IRDEBUG = false;
    Serial.println("Disabled IR Debug");
  }
  if (b==2) { // enables ir debug
    IRDEBUG = true;
    Serial.println("Enabled IR Debug");
  }
}
BLYNK_WRITE(V7) { // Sets Utility Box Team Alignment
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b == 1) {
    TeamAlignment = 99;
  }
  if (b == 2) {
    TeamAlignment = 0;
  }
  if (b == 3) {
    TeamAlignment = 1;
  }
  if (b == 4) {
    TeamAlignment = 2;
  }
  if (b == 5) {
    TeamAlignment = 3;
  }
  Serial.println("UBox Team Alignment set to: " + String(TeamAlignment));
}
BLYNK_WRITE(V8) { // Sets Utility Box tag function or IR output
  digitalWrite(led, HIGH);
  TagFunction = param.asInt();
  // 1 = respawn
  // 2 = Medic
  // 3 = Armor
  // 4 = Sheilds
  // 5 = Star Power
  // 6 = Ammo Pouch
  // 7 = Weapon Pickup
  // 8 = Proximity Mine
  // 9 = Security Alarm
  // 10 = Tear Gas
  Serial.println("Ubox Tag Function set to: " + String(TagFunction));
}
BLYNK_WRITE(V10) { // Sets Utility Box Activation Method/mode
  digitalWrite(led, HIGH);
  int b=param.asInt();
  if (b==1) { // Activate by tag/button (default)
    IRActivation = 1;
  }
  if (b==2) { // Auto - Every 5 Seconds
    IRActivation = 2;
    IRInterval = 5;
  }
  if (b==3) { // Auto - Every 10 Seconds
    IRActivation = 3;
    IRInterval = 10;
  }
  if (b==4) { // Auto - Every 15 Seconds
    IRActivation = 4;
    IRInterval = 15;
  }
  if (b==5) { // Auto - Every 30 Seconds
    IRActivation = 5;
    IRInterval = 30;
  }
  if (b==6) { // Auto - Every 60 Seconds
    IRActivation = 6;
    IRInterval = 60;
  }
  if (b==7) { // Auto - Every 90 Seconds
    IRActivation = 7;
    IRInterval = 90;
  }
  Serial.println("IR Activation Mode Set to: " + String(IRActivation));
}
BLYNK_WRITE(V12) { // Sets Weapon 1, slot 0 - trigger activated
  int b=param.asInt();
  if (Mode == 9) {
  if(b < 10) {
    BRXCMD = ("AT+SEND=0,1," + String(b) + "\r\n");
  }
  if(b > 9) {
    BRXCMD = ("AT+SEND=0,2," + String(b) + "\r\n");
  }
  Serial.println("Sending the following BRX Command: " + String(b));
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V13) { // Sets Weapon 2, slot 1 - trigger activated
  int b = param.asInt() + 100;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V14) { // Sets Weapon 3, slot 6 - trigger activated
  int b = param.asInt() + 199;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V15) { // Sets Weapon 4, slot 7 - trigger activated
  int b = param.asInt() + 299;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V16) { // Sets Melee, slot 4 - left/gyro activated
  int b = param.asInt() + 800;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V17) { // Sets Lives Count
  int b = param.asInt() + 400;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V18) { // Sets Game Time
  int b = param.asInt() + 500;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V19) { // Sets Delayed Start Timer
  int b = param.asInt() + 1000;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,4," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V20) { // Sets Ammo Mode
  int b = param.asInt() + 1300;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,4," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V21) { // Sets Outdoor/Indoor
  int b = param.asInt() + 600;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V22) { // Sets Team Settings
  int b = param.asInt() + 700;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V23) { // Sets Respawn Mode
  int b = param.asInt() + 900;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,3," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V24) { // Sets Volume
  int b = param.asInt() + 1500;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,4," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V25) { // Sets freindly Fire on/off
  int b = param.asInt() + 1400;
  if (Mode == 9) {
  BRXCMD = ("AT+SEND=0,4," + String(b) + "\r\n");
  BRXCommand(); // runs the LoRa Transciever to send the specified command
  Serial.println("Sending the following BRX Command: " + String(b));
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V26) { // Enables Sync Score
  int b = param.asInt();
  if (Mode == 9) {
  if (b == 1) {
    Serial.println("Syncing Scores");
    ENABLESCORESYNC = true;
  }
  } else {
    Serial.println("Incorrect Mode, Switch to JEDGE");
  }
}
BLYNK_WRITE(V27) { // Starts/Ends game
  int b = param.asInt();
    if (Mode == 9) {
      if (b == 1) { // start game
        BRXCMD = ("AT+SEND=0,4,99\r\n");
      }
      if (b == 2) { // end game
        BRXCMD = ("AT+SEND=0,4,98\r\n");
      }
    BRXCommand(); // runs the LoRa Transciever to send the specified command
    Serial.println("Sending the following BRX Command: " + String(b));
    } else {
      Serial.println("Incorrect Mode, Switch to JEDGE");
    }
}
BLYNK_WRITE(V28) { // Update OTA
  int b = param.asInt();
    if (Mode == 9) {
      if (b == 1) {
        BRXCMD = ("AT+SEND=0,4,151\r\n");
      }
      BRXCommand(); // runs the LoRa Transciever to send the specified command
      Serial.println("Sending the following BRX Command: " + String(b));
    } else {
      Serial.println("Incorrect Mode, Switch to JEDGE");
    }
}
//*********************************************************************************************************************
// This procedure sends a 38KHz pulse to the IRledPin 
// for a certain # of microseconds. We'll use this whenever we need to send codes
void pulseIR(long microsecs) {
  // we'll count down from the number of microseconds we are told to wait
  cli();  // this turns off any background interrupts
  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
   digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
   digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
   // so 26 microseconds altogether
   microsecs -= 26;
  }
  sei();  // this turns them back on
}
//********************************************
void RedRespawn() { // sends an IR tag protocol to respawn red team players
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
}
//******************************************
void BlueRespawn() { // Respawn Blue Team Players or Free For All
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//*********************************
void GreenRespawn() { // respawns green team players
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
}
//**************************************
void YellowRespawn() { // respawns Yellow team players
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
}
//********************************
void MotionSensor() { // alarm from yellow team
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//***********************************
void BlueGrenade() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//********************************
void RedGrenade() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//********************************
void YellowGrenade() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//********************************
void GreenGrenade() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//********************************
void BlueMedkit() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // t0
  delayMicroseconds(500);
  pulseIR(1000); // t1
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // z0
  delayMicroseconds(500);
  pulseIR(500); // z1
}
//******************************8
void RedMedkit() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // t0
  delayMicroseconds(500);
  pulseIR(500); // t1
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // z0
  delayMicroseconds(500);
  pulseIR(1000); // z1
}
//******************************8
void YellowMedkit() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // t0
  delayMicroseconds(500);
  pulseIR(1000); // t1
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // z0
  delayMicroseconds(500);
  pulseIR(1000); // z1
}
//******************************8
void GreenMedkit() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // t0
  delayMicroseconds(500);
  pulseIR(500); // t1
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // z0
  delayMicroseconds(500);
  pulseIR(500); // z1
}
//******************************8
void Medigel() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//*********************************
void BlueNano() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//*****************************88
void RedNano() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//*****************************88
void YellowNano() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//*****************************88
void GreenNano() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//*****************************88
void BlueSheild() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//****************************
void RedSheild() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); // t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//****************************
void YellowSheild() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//****************************
void GreenSheild() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); // t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//****************************
void controlpointlost() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//********************************
void CaptureTheFlag() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//****************************
void BlueGas() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000); //z
  delayMicroseconds(500);
  pulseIR(500); //z
}
//*********************************8
void RedGas() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500); //z
  delayMicroseconds(500);
  pulseIR(1000); //z
}
//*********************************8
void YellowGas() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000); //z
  delayMicroseconds(500);
  pulseIR(500); //z
}
//*********************************8
void GreenGas() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500); //z
  delayMicroseconds(500);
  pulseIR(1000); //z
}
//*******************************8
void BlueAlarm() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(1000); // t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //z
  delayMicroseconds(500);
  pulseIR(1000); //z
}
//*******************************
void RedAlarm() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //t
  delayMicroseconds(500);
  pulseIR(500); // t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //z
  delayMicroseconds(500);
  pulseIR(500); //z
}
//*******************************
void GreenAlarm() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(1000); // t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //z
  delayMicroseconds(500);
  pulseIR(500); //z
}
//*******************************
void YellowAlarm() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000); //t
  delayMicroseconds(500);
  pulseIR(500); // t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500); //z
  delayMicroseconds(500);
  pulseIR(1000); //z
}
//*******************************
void OwnTheZone() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
}
//****************************************
void BlueStar() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//*****************************************************
void RedStar() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//*****************************************************
void YellowStar() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);//z
  delayMicroseconds(500);
  pulseIR(1000);//z
}
//*****************************************************
void GreenStar() {
  pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);//t
  delayMicroseconds(500);
  pulseIR(500);//t
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);//z
  delayMicroseconds(500);
  pulseIR(500);//z
}
//*****************************************************
void AmmoPouch() {
    pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
}
//*****************************************
void RandomWeapon() {
    pulseIR(2000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(1000);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(500);
  delayMicroseconds(500);
  pulseIR(1000);
}
//*************************************
void SendTag() {
  if (TagFunction == 1) { // checking to see if we are sending respawns
    if (TeamAlignment == 0) {
      RedRespawn();
    }
    if (TeamAlignment == 1) {
      BlueRespawn();
    }
    if (TeamAlignment == 2) {
      YellowRespawn();
    }
    if (TeamAlignment == 3) {
      GreenRespawn();
    }
  }
  if(TagFunction == 2) { // checking if we are sending HP
    if (TeamAlignment == 0) {
      RedMedkit();
    }
    if (TeamAlignment == 1) {
      BlueMedkit();
    }
    if (TeamAlignment == 2) {
      YellowMedkit();
    }
    if (TeamAlignment == 3) {
      GreenMedkit();
    }
  }
  if(TagFunction == 3) { // checking if we are sending Armor
    if (TeamAlignment == 0) {
      RedNano();
    }
    if (TeamAlignment == 1) {
      BlueNano();
    }
    if (TeamAlignment == 2) {
      YellowNano();
    }
    if (TeamAlignment == 3) {
      GreenNano();
    }
  }
  if(TagFunction == 4) { // checking if we are sending Sheilds
    if (TeamAlignment == 0) {
      RedSheild();
    }
    if (TeamAlignment == 1) {
      BlueSheild();
    }
    if (TeamAlignment == 2) {
      YellowSheild();
    }
    if (TeamAlignment == 3) {
      GreenSheild();
    }
  }
  if(TagFunction == 5) { // checking if we are sending Star Power
    if (TeamAlignment == 0) {
      RedStar();
    }
    if (TeamAlignment == 1) {
      BlueStar();
    }
    if (TeamAlignment == 2) {
      YellowStar();
    }
    if (TeamAlignment == 3) {
      GreenStar();
    }
  }
  if(TagFunction == 6) { // checking if we are sending Ammo
    AmmoPouch();
  }
  if(TagFunction == 7) { // checking if we are sending Weapon Swap
    RandomWeapon();
  }
  if(TagFunction == 8) { // checking if we are sending Explosive Damage
    if (TeamAlignment == 0) {
      RedGrenade();
    }
    if (TeamAlignment == 1) {
      BlueGrenade();
    }
    if (TeamAlignment == 2) {
      YellowGrenade();
    }
    if (TeamAlignment == 3) {
      GreenGrenade();
    }
  }
  if(TagFunction == 9) { // checking if we are sending Security Alarm
    if (TeamAlignment == 0) {
      RedAlarm();
    }
    if (TeamAlignment == 1) {
      BlueAlarm();
    }
    if (TeamAlignment == 2) {
      YellowAlarm();
    }
    if (TeamAlignment == 3) {
      GreenAlarm();
    }
  }
  if(TagFunction == 10) { // checking if we are sending Tear Gas
    if (TeamAlignment == 0) {
      RedGas();
    }
    if (TeamAlignment == 1) {
      BlueGas();
    }
    if (TeamAlignment == 2) {
      YellowGas();
    }
    if (TeamAlignment == 3) {
      GreenGas();
    }
  }
  if (Mode == 8) { // checking if in own the zone mode
    OwnTheZone();
  }
}
//******************************************************************************************************************************************************************************************
void PrintTag() {
  // prints each individual bit on serial monitor
  // typically not used but used for troubleshooting
  Serial.println("TagReceived!!");
  Serial.print("B0: "); Serial.println(B[0]);
  Serial.print("B1: "); Serial.println(B[1]);
  Serial.print("B2: "); Serial.println(B[2]);
  Serial.print("B3: "); Serial.println(B[3]);
  Serial.print("P0: "); Serial.println(P[0]);
  Serial.print("P1: "); Serial.println(P[1]);
  Serial.print("P2: "); Serial.println(P[2]);
  Serial.print("P3: "); Serial.println(P[3]);
  Serial.print("P4: "); Serial.println(P[4]);
  Serial.print("P5: "); Serial.println(P[5]);
  Serial.print("T0: "); Serial.println(T[0]);
  Serial.print("T1: "); Serial.println(T[1]);
  Serial.print("D0: "); Serial.println(D[0]);
  Serial.print("D1: "); Serial.println(D[1]);
  Serial.print("D2: "); Serial.println(D[2]);
  Serial.print("D3: "); Serial.println(D[3]);
  Serial.print("D4: "); Serial.println(D[4]);
  Serial.print("D5: "); Serial.println(D[5]);
  Serial.print("D6: "); Serial.println(D[6]);
  Serial.print("D7: "); Serial.println(D[7]);
  Serial.print("C0: "); Serial.println(C[0]);
  Serial.print("U0: "); Serial.println(U[0]);
  Serial.print("U1: "); Serial.println(U[1]);
  Serial.print("Z0: "); Serial.println(Z[0]);
  Serial.print("Z1: "); Serial.println(Z[1]);
}
//******************************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************************
// this procedure breaksdown each Weapon bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDDamage() {
      // determining indivudual protocol values for Weapon ID bits
      if (D[7] > 750) {
        DID[0] = 2;
      } else {
        DID[0] = 1;
      }
      if (D[6] > 750) {
        DID[1]=2;
      } else {
        DID[1]=0;
      }
      if (D[5] > 750) {
        DID[2]=4;
        } else {
        DID[2]=0;
      }
      if (D[4] > 750) {
        DID[3]=8;
      } else {
        DID[3]=0;
      }
      if (D[3] > 750) {
        DID[4]=16;
      } else {
        DID[4]=0;
      }
      if (D[2] > 750) {
        DID[5]=32;
      } else {
        DID[5]=0;
      }
      if (D[1] > 750) {
        DID[6]=64;
      } else {
        DID[6]=0;
      }
      if (D[0] > 750) {
        DID[7]=128;
      } else {
        DID[7]=0;
      }
      // ID Damage by summing assigned values above based upon protocol values (1-64)
      DamageID=DID[0]+DID[1]+DID[2]+DID[3]+DID[4]+DID[6]+DID[7];
      Serial.print("Damage ID = ");
      Serial.println(DamageID);
}
//******************************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************************
// this procedure breaksdown each bullet bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDShot() {
      // determining indivudual protocol values for Weapon ID bits
      if (B[3] > 750) {
        BID[0] = 2;
      } else {
        BID[0] = 1;
      }
      if (B[2] > 750) {
        BID[1]=2;
      } else {
        BID[1]=0;
      }
      if (B[1] > 750) {
        BID[2]=4;
        } else {
        BID[2]=0;
      }
      if (B[0] > 750) {
        BID[3]=8;
      } else {
        BID[3]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      ShotType=BID[0]+BID[1]+BID[2]+BID[3];
      Serial.print("Shot Type = ");
      Serial.println(ShotType);      
}
//******************************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************************
// this procedure breaksdown each player bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDplayer() {
      // determining indivudual protocol values for player ID bits
      // Also assign IR values for sending player ID with device originated tags
      if (P[5] > 750) {
        PID[0] = 2;
      } else {
        PID[0] = 1;
      }
      if (P[4] > 750) {
        PID[1]=2;
      } else {
        PID[1]=0;
      }
      if (P[3] > 750) {
        PID[2]=4;
        } else {
        PID[2]=0;
      }
      if (P[2] > 750) {
        PID[3]=8;
      } else {
        PID[3]=0;
      }
      if (P[1] > 750) {
        PID[4]=16;
      } else {
        PID[4]=0;
      }
      if (P[0] > 750) {
        PID[5]=32;
      } else {
        PID[5]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      PlayerID=PID[0]+PID[1]+PID[2]+PID[3]+PID[4]+PID[5];
      Serial.print("Player ID = ");
      Serial.println(PlayerID);   
}
//******************************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************************
void teamID() {
      // check if the IR is from Red team
      if (T[0] < 750 && T[1] < 750) {
        // sets the current team as red
        team = 1;
        Serial.print("team = Red = ");
        Serial.println(team);
        Posession = 0;
      }
      // check if the IR is from blue team 
      if (T[0] < 750 && T[1] > 750) {
        // sets the current team as blue
        team = 2;
        Serial.print("team = Blue = ");
        Serial.println(team);
        Posession = 1;
      }
      // check if the IR is from green team 
      if (T[0] > 750 && T[1] > 750) {
        // sets the current team as green
        team = 3;
        Serial.print("team = Green = ");
        Serial.println(team);
        Posession = 3;
      }
      if (T[0] > 750 && T[1] < 750) {
        // sets the current team as red
        team = 4;
        Serial.print("team = Yellow = ");
        Serial.println(team);
        Posession = 2;
      }
      if (Mode == 3) { // checks if acting as a slave
        if (PreviousPosession != Posession) { // checks if there was a turn over
          TURNOVER = true;          
        }
      }
}
//******************************************************************************************************************************************************************************************
// this procedure breaksdown each bullet bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the Power (0-4)
void IDPower() {
      if (U[0] < 750 && U[1] < 750) {
      Power = 0;
      Serial.print("Power = 0");
      Serial.println(Power);
      }
      if (U[0] < 750 && U[1] > 750) {
      Power = 1;
      Serial.print("Power = 1");
      Serial.println(Power);
      } 
      if (U[0] > 750 && U[1] > 750) {
      Power = 3;
      Serial.print("Power = 3");
      Serial.println(Power);
      }
      if (U[0] > 750 && U[1] < 750) {
      team = 3;
      Serial.print("Power = 2");
      Serial.println(Power);
      }
}
//******************************************************************************************************************************************************************************************
// this procedure breaksdown each bullet bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the Power (0-4)
void IsCritical() {
      if (C[0] < 750) {
      Critical = 0;
      } else {
        Critical = 1;
      }
}
//******************************************************************************************************************************************************************************************
// This procedure uses the preset IR_Sensor_Pin to determine if an ir received is BRX, if so it records the protocol received
void receiveBRXir() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 4; x++) B[x]=0;
  for (byte x = 0; x < 6; x++) P[x]=0;
  for (byte x = 0; x < 2; x++) T[x]=0;
  for (byte x = 0; x < 8; x++) D[x]=0;
  for (byte x = 0; x < 1; x++) C[x]=0;
  for (byte x = 0; x < 2; x++) U[x]=0;
  for (byte x = 0; x < 2; x++) Z[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 1500) { // checks that the incoming IR matchets BRX protocols
      digitalWrite(led, HIGH);
      // stores each pulse or bit, individually for analyzing the data
      B[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      B[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B2
      B[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B3
      B[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B4
      P[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1
      P[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2
      P[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3
      P[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4
      P[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5
      P[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6
      T[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      T[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      D[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      D[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      D[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      D[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      D[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D5
      D[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D6
      D[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D7
      D[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D8
      C[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // C1
      U[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?1
      U[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?2
      Z[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z1
      Z[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z2
      X[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      if (Z[1] > 250 && X[0] < 250 && B[0] < 1250) { // checks to make sure it is a BRX tag
        if (IRDEBUG) { // check if we are debugging ir protocols
          NEWTAG = true;
        }
        PrintTag();
        teamID();
        IDplayer();
        IDShot();
        IDDamage();
        IDPower();
        IsCritical();
        if (BYSHOTS) {
          AddPoints();
          digitalWrite(led, HIGH);
        }
        if (BYDAMAGE) {
          AddDamage();
        }
        if (Mode == 5) { // checks that we are in utility box mode
          TeamAlignment = team;
          SendTag();
        }
        }
      digitalWrite(led, LOW);
      }
}
//******************************************************************************************************************************************************************************************
void ClearScores() {
  Posession = 99;
  Red = 0;
  Blu = 0;
  Grn = 0;
  Ylw = 0;
  PostScores();
  GameTimer = 0;
  PLENTYOFTIME = true;
  UBID[0] = 99;
  UBID[1] = 99;
  UBID[2] = 99;
  UBID[3] = 99;
  UBID[4] = 99;
  UBID[5] = 99;
  UBID[6] = 99;
  UBID[7] = 99;
  UBID[8] = 99;
  UBID[9] = 99;
}
//******************************************************************************************************************************************************************************************
void AddDamage() {
  if (MaxScore > Red && MaxScore > Ylw && MaxScore > Blu && MaxScore > Grn) {
    if (Posession == 0) {
      Red = Red + DamageID;
      }
    if (Posession == 1) {
      Blu = Blu + DamageID;
    }
    if (Posession == 3) {
      Grn = Grn + DamageID;
    }
    if (Posession == 2) {
      Ylw = Ylw + DamageID;
    }
    POSTSCORES = true;
  }
}
//******************************************************************************************************************************************************************************************
void AddPoints() {
  if (MaxScore > Red && MaxScore > Ylw && MaxScore > Blu && MaxScore > Grn) {
    digitalWrite(led, HIGH);
    if (Posession == 0) {
      Red = Red+1;
    }
    if (Posession == 1) {
      Blu = Blu+1;
    }
    if (Posession == 3) {
      Grn = Grn+1;
    }
    if (Posession == 2) {
      Ylw = Ylw+1;
    }
    if (Mode == 2) {  // checking if we have other bases to account for scores
      int counter = 0;
      while (counter < 10) {
        if (UBID[counter] == 0) {
          Red++;
        }
        if (UBID[counter] == 1) {
          Blu++;
        }
        if (UBID[counter] == 2) {
          Ylw++;
        }
        if (UBID[counter] == 3) {
          Grn++;
        }
        counter++; 
      }
    }
    POSTSCORES = true;
    digitalWrite(led, LOW);
  }
}
//******************************************************************************************************************************************************************************************
void AddTime() {
  if (Posession != 99 || Mode == 5) { // checks that someone has actually captured the base or added points or we are in utility box mode
    GameTimer++; // adds one to the game timer
    Serial.println("Game counter Executed, Current Clock: " + String(GameTimer));
    Blynk.virtualWrite(V110, GameTimer); // sends updated game time to the blynk app
  }
  if (GameTimer > GameMaxTime) { // checks to see if the new game time is greater than the max allowable
    PLENTYOFTIME = false; // sets the trigger to halt the game by disabling all main loop functions
    Serial.println("Out of Time, Game Over");
  }
}
//******************************************************************************************************************************************************************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(Serial1.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    received = Serial1.readString(); // stores the incoming data to the pre set string
    Serial.print("Received: "); // printing data received
    Serial.println(received); // printing data received
    if(received.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)received.c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        tokenStrings[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        }
        // the data is now stored into individual strings under tokenStrings[]
        // print out the individual string arrays separately
        Serial.println("received LoRa communication");
        Serial.print("Token 0: "); // identifier for sender (+REC=900) this is our sender
        Serial.println(tokenStrings[0]);
        Serial.print("Token 1: "); // this is a value identifier for how many bytes are in the message
        Serial.println(tokenStrings[1]);
        Serial.print("Token 2: "); // this is the actual data recevied or message
        Serial.println(tokenStrings[2]);
        Serial.print("Token 3: "); // this is just a range indicator for transmission speed/distance
        Serial.println(tokenStrings[3]);
        Serial.print("Token 4: "); // same as token 3
        Serial.println(tokenStrings[4]);
        
        // check the data for a data match
        if (tokenStrings[2] == "R") {  //in this case our single received byte would always be at the 11th position
          // we need to check which base is switching to red
          if (tokenStrings[0] == "+REC=900") {
            UBID[0] = 0;
          }
          if (tokenStrings[0] == "+REC=901") {
            UBID[1] = 0;
          }
          if (tokenStrings[0] == "+REC=902") {
            UBID[2] = 0;
          }
          if (tokenStrings[0] == "+REC=903") {
            UBID[3] = 0;
          }
          if (tokenStrings[0] == "+REC=904") {
            UBID[4] = 0;
          }
          if (tokenStrings[0] == "+REC=905") {
            UBID[5] = 0;
          }
          if (tokenStrings[0] == "+REC=906") {
            UBID[6] = 0;
          }
          if (tokenStrings[0] == "+REC=907") {
            UBID[7] = 0;
          }
          if (tokenStrings[0] == "+REC=908") {
            UBID[8] = 0;
          }
          if (tokenStrings[0] == "+REC=909") {
            UBID[9] = 0;
          }
        }
        if (tokenStrings[2] == "B") {  //in this case our single received byte would always be at the 11th position
          // we need to check which base is switching to red
          if (tokenStrings[0] == "+REC=900") {
            UBID[0] = 1;
          }
          if (tokenStrings[0] == "+REC=901") {
            UBID[1] = 1;
          }
          if (tokenStrings[0] == "+REC=902") {
            UBID[2] = 1;
          }
          if (tokenStrings[0] == "+REC=903") {
            UBID[3] = 1;
          }
          if (tokenStrings[0] == "+REC=904") {
            UBID[4] = 1;
          }
          if (tokenStrings[0] == "+REC=905") {
            UBID[5] = 1;
          }
          if (tokenStrings[0] == "+REC=906") {
            UBID[6] = 1;
          }
          if (tokenStrings[0] == "+REC=907") {
            UBID[7] = 1;
          }
          if (tokenStrings[0] == "+REC=908") {
            UBID[8] = 1;
          }
          if (tokenStrings[0] == "+REC=909") {
            UBID[9] = 1;
          }
        }
        if (tokenStrings[2] == "Y") {  //in this case our single received byte would always be at the 11th position
          // we need to check which base is switching to red
          if (tokenStrings[0] == "+REC=900") {
            UBID[0] = 2;
          }
          if (tokenStrings[0] == "+REC=901") {
            UBID[1] = 2;
          }
          if (tokenStrings[0] == "+REC=902") {
            UBID[2] = 2;
          }
          if (tokenStrings[0] == "+REC=903") {
            UBID[3] = 2;
          }
          if (tokenStrings[0] == "+REC=904") {
            UBID[4] = 2;
          }
          if (tokenStrings[0] == "+REC=905") {
            UBID[5] = 2;
          }
          if (tokenStrings[0] == "+REC=906") {
            UBID[6] = 2;
          }
          if (tokenStrings[0] == "+REC=907") {
            UBID[7] = 2;
          }
          if (tokenStrings[0] == "+REC=908") {
            UBID[8] = 2;
          }
          if (tokenStrings[0] == "+REC=909") {
            UBID[9] = 2;
          }
        }
        if (tokenStrings[2] == "G") {  //in this case our single received byte would always be at the 11th position
          // we need to check which base is switching to red
          if (tokenStrings[0] == "+REC=900") {
            UBID[0] = 3;
          }
          if (tokenStrings[0] == "+REC=901") {
            UBID[1] = 3;
          }
          if (tokenStrings[0] == "+REC=902") {
            UBID[2] = 3;
          }
          if (tokenStrings[0] == "+REC=903") {
            UBID[3] = 3;
          }
          if (tokenStrings[0] == "+REC=904") {
            UBID[4] = 3;
          }
          if (tokenStrings[0] == "+REC=905") {
            UBID[5] = 3;
          }
          if (tokenStrings[0] == "+REC=906") {
            UBID[6] = 3;
          }
          if (tokenStrings[0] == "+REC=907") {
            UBID[7] = 3;
          }
          if (tokenStrings[0] == "+REC=908") {
            UBID[8] = 3;
          }
          if (tokenStrings[0] == "+REC=909") {
            UBID[9] = 3;
          }
        }
      }
    }
}
//******************************************************************************************************************************************************************************************
void TransmitStatus() {
  //delay(500);
  //String Sender = (String(received[5])+String(received[6])+String(received[7])); // if needed to send a response to the sender, this function builds the needed string
  //Serial.println("Transmitting Confirmation"); // printing to serial monitor
  //Serial1.print("AT+SEND="+Sender+",5,GotIt\r\n"); // used to send a confirmation to sender
  if (Mode == 3) {
    if (Posession == 0) {
      Serial1.print("AT+SEND=0,1,R\r\n"); // sending posession status to master
    }
    if (Posession == 1) {
      Serial1.print("AT+SEND=0,1,B\r\n"); // sending posession status to master
    }
    if (Posession == 2) {
      Serial1.print("AT+SEND=0,1,Y\r\n"); // sending posession status to master
    }
    if (Posession == 3) {
      Serial1.print("AT+SEND=0,1,G\r\n"); // sending posession status to master
    }
  }
  if (Mode = 2) {
    Serial1.print("AT+SEND=0,1,Z\r\n"); // sending reset command to slaves
  }
    
  //Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //Serial1.print("$PLAY,VA20,4,6,,,,,*");
  //Serial.println("Sent the '5' to device 901"); 
}
//******************************************************************************************************************************************************************************************
void BRXCommand() {
  Serial1.print(BRXCMD); // sending posession status to master
  Serial.print("Sent: " + String(BRXCMD) + ", Via LoRa");
}
//*********************************************************************************************
void syncscores() {
  if (playercounter < 164){
    BRXCMD = ("AT+SEND="+String(playercounter)+",4,1101\r\n");
  } else {
    playercounter = 100;
    ENABLESCORESYNC = false;
  }
}
//**********************************************************************************************************************************************
void PostScores() {
  digitalWrite(led, HIGH);
  Blynk.virtualWrite(V100, Red);
  Blynk.virtualWrite(V101, Blu);
  Blynk.virtualWrite(V102, Grn);
  Blynk.virtualWrite(V103, Ylw);
  digitalWrite(led, LOW);
}
//******************************************************************************************************************************************************************************************
void sendtaginfo() {
  Blynk.virtualWrite(V105, team);
  Blynk.virtualWrite(V108, Power);
  Blynk.virtualWrite(V109, Critical);
  Blynk.virtualWrite(V104, ShotType);
  Blynk.virtualWrite(V107, PlayerID); 
  Blynk.virtualWrite(V106, DamageID);
}
//******************************************************************************************************************************************************************************************
void transmissioncounter() {
  sendcounter++;
  Blynk.virtualWrite(V100, sendcounter);
  Serial.println("Added 1 to sendcounter and sent to blynk app");
  Serial.println("counter value = "+String(sendcounter));
}

void receivercounter() {
  receivecounter++;
  Blynk.virtualWrite(V101, receivecounter);
  Serial.println("Added 1 to receivercounter and sent to blynk app");
  Serial.println("counter value = "+String(receivecounter));
  digitalWrite(LED_BUILTIN, LOW); // turn onboard led off
}

void TransmitResponse() {
  // LETS RESPOND:
  delay(500);
  //String Sender = (String(received[5])+String(received[6])+String(received[7])); // if needed to send a response to the sender, this function builds the needed string
  Serial.println("Transmitting Confirmation"); // printing to serial monitor
  //Serial1.print("AT+SEND="+Sender+",5,GotIt\r\n"); // used to send a confirmation to sender
  Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //Serial1.print("$PLAY,VA20,4,6,,,,,*");
  Serial.println("Sent the '5' to device 0");
  transmissioncounter();
}

// Dirty loop with
void loop1(void *pvParameters) {
  int blahblah = 0;
  while (1) { // starts the forever loop
    // do nothing
    if(tokenStrings[2] == "5") {
    Serial.println(String(blahblah));
    blahblah++;
    }
    delay(1);
  }
}


// BLYNK LOOP. Note the 1ms delay, this is need so the watchdog doesn't get confused
void loop2(void *pvParameters) {
  while (1) { // starts the forever loop
    // put your main code here, to run repeatedly:
    Blynk.run();
    LoRaListen.run(); // run the lora listening object
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);   //default baudrate of module is 115200
  delay(100);             //wait for Lora device to be ready

  Serial.println("Waiting for connections...");
  
  Blynk.setDeviceName("JEDGE900");

  Blynk.begin(auth);

  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins

  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  
  Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  
  Serial1.print("AT+ADDRESS=900\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  
  Serial1.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond

  Serial1.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
   
  Serial1.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  
  Serial1.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  
  Serial1.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond

  pinMode(LED_BUILTIN, OUTPUT); // setting up the onboard LED to be used
  digitalWrite(LED_BUILTIN, LOW); // turn off onboard led
  digitalWrite(LED_BUILTIN, HIGH); // turn on onboard led
  delay(500);
  digitalWrite(LED_BUILTIN, LOW); // turn off onboard led

  LoRaListen.setInterval(1L, ReceiveTransmission); // Reading data from LoRa constantly

  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);

}

void loop() {
// emply loop
}
  
