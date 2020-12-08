/*
 * update 4/2/2020 annotations added and copied over objects for setting up games
 * update 4/3/2020 Cleaned up for more compatibility to serial comms programing
 * update 4/3/2020 Was able to plug in set setting from serial for friendly fire and outdoor/indoor mode
 * update 4/4/2020 was able to get team settings and manual input team settings working as well as gender settings
 * updated 4/4/2020 included weapon selection assignment and notification to select in game settings manual options
 * updated 4/6/2020 separated many variables to not share writing abilities from both cores (trouble shooting)
 * upfsyrf 4/7/2020 finished isolating variables between cores to eliminate both cores writing to same variables
 * updated 4/7/2020 fixed the resetting of the BLE function so device stays paired to brx, also integrated the delay start with the send game settings to start the game
 * updated 4/8/2020 fixed some menu callout audio selections, reduced the delay time to verify what is best and still functions to avoid disconnects
 * updated 4/8/2020 fixed weapon slot 1 loading issue, was set to weapon slot 0 so guns were overwriting.
 * updated 4/9/2020 fixed team selection item number and set freindly fire on when free for all selection is made for team selection.
 * updated 4/9/2020 fixed audio announcement for teams for free for all to say free for all
 * updated 4/9/2020 fixed audio announcement for domination to read control point versus select a game
 * updated 4/9/2020 fixed limited ammo, wasnt allowing limited ammo due to incrorrect if then statement setting
 * updated 4/9/2020 fixed the manual team selection option to enable players to pick their own teams
 * updated 4/10/2020 enabled LCD data sending to esp8266, updated data to be sent to get lives, weapon and other correct indicators sent to the LCD
 * updated 4/13/2020 worked on more LCD debuging issues for sending correct data to LCD ESP8266
 * updated 4/14/2020 changed power output for the BLE antenna to try to minimize disconnects, was successful
 * updated 4/15/2020 fixed unlimited ammo when unarmed, was looping non stop for reloading because no ammo on unarmed
 * updated 4/15/2020 added additional ammunition options, limited, unlimited magazines, and unlimited rounds as options - ulimited rounds more for kids
 * updated 4/19/2020 improved team selection process, incorporated "End Game" selection (requires app update) disabled buttons/trigger/reload from making noises when pressed upon connection to avoid annoying sounds from players
 * updated 4/19/2020 enabled player selection for weapon slots 0/1. tested, debugged and ready to go for todays changes.
 * updated 4/22/2020 enabled serial communications to send weapon selection to ESP8266 so that it can be displayed what weapon is what if lCD is installed
 * updated 4/22/2020 enabled game timer to terminate a game for a player
 * updated 4/27/2020 enabled serial send of game score data to esp8266
 * updated 4/28/2020 adjusted volume settings to modify volume not at game start but whenever
 * updated 5/5/2020 modified the delayed start counter to work better and not stop the program as well as incorporate auditable countdown
 * updated 5/5/2020 incorporated respawn delay timers as well as respawn stations for manual respawn
 * updated 5/6/2020 fixed count down audio for respawn and delay start timers, also fixed lives assignment, was adding 100 to the lives selected. Note: all 5/5/2020 updates tested functional
 * updated 5/6/2020 fixed game timer repeat end, added two minute warning, one minute warning and ten second count down to end of game
 * updated 5/6/2020 re-instated three team selection and added four team auto selections
 * updated 5/7/2020 disabled player manual selections when triggered by blynk for a new option to be enabled or if game starts (cant give them players too much credit can we?)
 * updated 5/8/2020 fixed a bug with the count down game timer announcements
 * updated 5/22/2020 integrated custom weapon audio
 * updated 5/25/2020 disabled auto lockout of buttons upon esp32 pairing, enabled blynk enabled lockout of buttons instead by V18 or 1801 serial command This way we can control esp32 bluetooth activation to be enabled instead of automatic
 * updated 5/25/2020 added in deap sleep enabling if esp8266 sends a 1901 command
 * updated 8/11/2020 changed reporting score processes and timing to instant upon esp8266 request
 * updated 8/14/2020 fixed error for sending player scores, was missing player [0] and had a player[64] same for team[0] and team[6]
 * updated 8/16/2020 added an IR Debug mode that when a tag is recieved, it enables the data to be sent to the esp8266 and forwarded on to the app
 * updated 8/18/2020 added OTA updating capability to device, can be a bit finicky so be patient i guess.
 * updated 10/30/2020 reformatted code for easier legibility and clearl define all functions
 * updated 10/30/2020 added in manual respawns from nades/bases, also added in other IR based tag support, but it might be better to change the IR types later to not cause damage, or add in a healt boost with the feature
 * Updated 11/16/2020 fixed bugs with manual respawn. all good now
 * Updated Thanksgiving removed sending coms and LCD functions, fixed multi core loops for better operation, added LoRa Functions Removed unused interactions with incoming data
 * 
 * 
 * by Jay Burden
 *
 * keep device in range of laser tag BLE enabled compatible device for it to work
 * This code requires an esp8266 to be paired via serial (reference esp8266 code)
 *      use pins 16 & 17 to connect to D8 & D7, respectively, of ESP8266, or D6 and D5 in my build
 * optionally power from laser tag battery to the GRND and the VCC pins or power
 *      via usb or standalone battery
 * this devices uses the serial communication to set tagger and game configuration settings
 * Note the sections labeled *****IMPORTANT***** as it requires customization on your part
 *
 *Serial data recieved from ESP8266 is intended to trigger actions to perform upon the laser
 *tag rifle
 *
 *Serial data sent to the ESP8266 is intended to be displayed on the LCD or for reporting
 *to a server.
 */

//****************************************************************
// libraries to include:
#include "BLEDevice.h"
#include <HardwareSerial.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define SERIAL1_RXPIN 16 // TO LORA TX
#define SERIAL1_TXPIN 17 // TO LORA RX
//****************************************************************

//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//*********** YOU NEED TO CHANGE INFO IN HERE FOR EACH GUN!!!!!!***********
#define BLE_SERVER_SERVICE_NAME "NWPLAYER6" // CHANGE ME!!!! (case sensitive)
// this is important it is how we filter
// out unwanted guns or other devices that use UART BLE characteristics you need to change 
// this to the name of the gun ble server

int GunID = 6; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
String LoRaID = "AT+ADDRESS=900\r\n";
// This is the WIFI info for OTA Update Mode
const char* ssid = "burtek";
const char* password = "Sunpower15";
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************

//****************************************************************
// The remote service we wish to connect to, all guns should be the same as
// this is an uart comms set up
static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"); 
// The characteristic of the remote service we are interested in.
// these uuids are used to send and recieve data
static BLEUUID    charRXUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    charTXUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// variables used for ble set up
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteRXCharacteristic;
static BLERemoteCharacteristic* pRemoteTXCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEClient*  pClient;

// variables used to provide notifications of pairing device 
// as well as connection status and recieving data from gun
char notifyData[100];
int notifyDataIndex = 0;
String tokenStrings[100];
char *ptr = NULL;
String received; // string used to record data coming in
String LoRatokenStrings[5]; // used to break out the incoming data for checks/balances

// these are variables im using to create settings for the game mode and
// gun settings
int settingsallowed = 0; // trigger variable used to walk through steps for configuring gun(s) for serial core
int settingsallowed1 = 0; // trigger variable used to walk through steps for configuring gun(s) for BLE core
int SetSlotA=2; // this is for weapon slot 0, default is AMR
int SLOTA=100; // used when weapon selection is manual
int SetSlotB=1; // this is for weapon slot 1, default is unarmed
int SLOTB=100; // used when weapon selection is manual
int SetLives=32000; // used for configuring lives
int SetSlotC=0; // this is for weapon slot 6 Respawns Etc.
int SetSlotD = 0; // this is used for perk weapon slot 7 medic etc.
int SetSlotE = 0; // this is used for Melee weapons
int SetTeam=0; // used to configure team player settings, default is 0
long SetTime=2000000000; // used for in game timer functions on esp32 (future
int SetODMode=0; // used to set indoor and outdoor modes (default is on)
int SetGNDR=0; // used to change player to male 0/female 1, male is default 
int SetRSPNMode; // used to set auto or manual respawns from bases/ir (future)
long RespawnTimer = 0; // used to delay until respawn enabled
long RespawnTimerMax = 0; // max timer setting for incremental respawn timer (Ramp 90)
int SetObj=32000; // used to program objectives
int SetFF=1; // set game to friendly fire on/off (default is on)
int SetVol=60; // set tagger volume adjustment, default is 100
int CurrentWeapSlot; // used for indicating what weapon slot is being used, primarily for unlimited ammo
int ReloadType; // used for unlimited ammo... maybe 10 is for unlimited
int weaponcounter = 1; // used for swaping weapons out in gun game mode

int Deaths = 0; // death counter
int Team=0; // team selection used when allowed for custom configuration
int MaxKills = 32000; // setting limit on kill counts
int Objectives = 32000; // objective goals
int CompletedObjectives=0; // earned objectives by player
int PlayerLives = 32000; // setting max player lives
int MaxTeamLives = 32000; // setting maximum team lives
long GameTimer = 2000000000; // setting maximum game time
long GameStartTime=0; // used to set the start time when a match begins
int PlayerKillCount[64] = {0}; // so its players 0-63 as the player id.
int KillCount = 0; // used for Lora Kill Tracking based on incomming kill confirmations
int TeamKillCount[6] = {0}; // teams 0-6, Red-0, blue-1, yellow-2, green-3, purple-4, cyan-5
long DelayStart = 0; // set delay count down to 0 seconds for default
int GameMode=1; // for setting up general settings
int Special=0; // special settings
int AudioPlayCounter=0; // used to make sure audio is played only once (redundant check)
int UNLIMITEDAMMO = 1; // used to trigger ammo auto replenish if enabled
bool OutofAmmoA = false; // trigger for auto respawn of ammo weapon slot 0
bool OutofAmmoB = false; // trigger for auto respawn of ammo weapon slot 1
String AudioSelection; // used to play stored audio files with tagger FOR SERIAL CORE
String AudioSelection1; // used to play stored audio files with tagger FOR BLE CORE
String SendLoraMessage; // used for lora command to be sent to players

int lastTaggedPlayer = -1;  // used to capture player id who last shot gun, for kill count attribution
int lastTaggedTeam = -1;  // used to captures last player team who shot gun, for kill count attribution

// used to send to ESP8266 for LCD display
int SpecialWeapon = 0;

bool ENABLEBLE = true; // used to enable or disable BLE device
bool VOLUMEADJUST=false; // trigger for audio adjustment
bool RESPAWN = false; // trigger to enable auto respawn when killed in game
bool MANUALRESPAWN = false; // trigger to enable manual respawn from base stations when killed
bool PENDINGRESPAWNIR = false; // trigger to check for IR respawn signals
bool GAMEOVER = false; // used to trigger game over and boot a gun out of play mode
bool AUDIO = false; // used to trigger an audio on tagger FOR SERIAL CORE
bool AUDIO1 = false; // used to trigger an audio on tagger FOR BLE CORE
bool GAMESTART = false; // used to trigger game start
bool TurnOffAudio=false; // used to trigger audio off from serial core
bool GETTEAM=false; // used when configuring customized settings
bool STATUSCHANGE=false; // used to loop through selectable customized options
bool GETSLOT0=false; // used for configuring manual weapon selection
bool GETSLOT1=false; // used for configuring manual weapon selection
bool INGAME=false; // status check for game timer and other later for running certain checks if gun is in game.
bool COUNTDOWN1=false; // used for triggering a specic countdown
bool COUNTDOWN2=false; // used for triggering a specific countdown
bool COUNTDOWN3=false; // used for triggering a specific countdown
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool INITIALIZEOTA = false; // enables the object to disable BLE and enable WiFi
bool SPECIALWEAPON = false;
bool HASFLAG = false; // used for capture the flag
bool SWAPBRX = false; // used for a funny commercial to make fun of battle company
bool INITIATESWAPBRX = false; // used to initiate the weapon swap
bool LISTENTOLORA = true; // enable radio listening
bool ENABLELORASEND = false; // enable lora transmission

long startScan = 0; // part of BLE enabling

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED

bool WEAP = false; // not used anymore but was used to auto load gun settings on esp boot

//**************************************************************
//**************************************************************
// this bad boy captures any ble data from gun and analyzes it based upon the
// protocol predecessor, a lot going on and very important as we are able to 
// use the ble data from gun to decode brx protocol with the gun, get data from 
// guns health status and ammo reserve etc.

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);

  memcpy(notifyData + notifyDataIndex, pData, length);
  notifyDataIndex = notifyDataIndex + length;
  if (notifyData[notifyDataIndex - 1] == '*') { // complete receviing
    notifyData[notifyDataIndex] = '\0';
    notifyDataIndex = 0; // reset index
    //Lets tokenize by ","
    String receData = notifyData;
    byte index = 0;
    ptr = strtok((char*)receData.c_str(), ",");  // takes a list of delimiters
    while (ptr != NULL)
    {
      tokenStrings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    Serial.println("We have found " + String(index ) + " tokens");
    
    //*******************************************************
    // first signal we get from gun is this one and it tells the esp
    // that gun is happy with the connection
    if (tokenStrings[0] == "$#CONNECT") { // checks to see if the connection notification was received from BRX
      VOLUMEADJUST=true; // runs the default audio setting trigger in the main loop for BLE 
      AudioSelection1="VA20"; // sets the auio selection for "connection established" and tells the brx to play it in the audio object from BLE loop
      AUDIO1=true; // ensures audio object runs
      } // THIS IS TO ENABLE GUN CONTROL BY AND LOCK OUT PLAYER BUTTONS ONCE ESP32 IS PAIRED, this is enabled when audio is equal to VA20 in the Audio object
    // this analyzes every single time a trigger is pulled
    // or a button is pressed or reload handle pulled. pretty cool
    // i dont do much with it yet here but will be used later to allow
    // players to select weapon pickups etc or options when starting a game
    // ideally with LCD installed this will be very usefull! can use it with
    // limited gun choices, like the basic smg, tar33 etc as we can use gun 
    // audio to help players know what they are picking but not implemented yet
    /* 
     *  Mapping of the BLE buttons pressed
     *  Trigger pulled: $BUT,0,1,*
     *  Tirgger Released: $BUT,0,0,*
     *  Alt fire pressed:  $BUT,1,1,*
     *  Alt fire released:  $BUT,1,0,*
     *  Reload pulled:  $BUT,2,1,*
     *  Reload released:  $BUT,2,0,*
     *  select button pressed:  $BUT,3,1,*
     *  select button released:  $BUT,3,0,*
     *  left button pressed:  $BUT,4,1,*
     *  left button released:  $BUT,4,0,*
     *  right button pressed:  $BUT,5,1,*
     *  right button released:  $BUT,5,0,*
     */
    if (tokenStrings[0] == "$BUT") {
      if (tokenStrings[1] == "0") {
        if (tokenStrings[2] == "1") {
          Serial.println("Trigger pulled"); // as indicated this is the trigger
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Trigger Released"); // goes without sayin... you let go of the trigger
          // upon release of a trigger, team settings can be changed if the proper allowance is in place
          if (GETTEAM){ // used for configuring manual team selection
            if (Team==5) {
              Team=0; 
              STATUSCHANGE=true; 
              AudioSelection1="VA13"; 
              AUDIO1=true; Serial.println("team changed from 5 to 0");
              }          
            if (Team==4) {
              Team=5; 
              AudioSelection1="VA2Y"; 
              AUDIO1=true; 
              Serial.println("team changed from 4 to 5");
              } // foxtrot team
            if (Team==3) {
              Team=4; 
              AudioSelection1="VA2G"; 
              AUDIO1=true; Serial.println("team changed from 3 to 4");
              } // echo team
            if (Team==2) {
              Team=3; 
              AudioSelection1="VA27"; 
              AUDIO1=true; 
              Serial.println("team changed from 2 to 3");
              } // delta team
            if (Team==1) {
              Team=2; 
              AudioSelection1="VA1R"; 
              AUDIO1=true; 
              Serial.println("team changed from 1 to 2");
              } // charlie team
            if (Team==0 && STATUSCHANGE==false) {
              Team=1; 
              AudioSelection1="VA1L"; 
              AUDIO1=true; Serial.println("team changed from 0 to 1");
              } // bravo team        
            STATUSCHANGE=false;
          }
          if (GETSLOT0){ // used for configuring manual team selection
            if (SLOTA==19) {
              SLOTA=1; 
              STATUSCHANGE=true; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon changed from 19 to 1");
              }          
            if (SLOTA==18) {
              SLOTA=19; 
              AudioSelection1="GN19"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 18 to 19");
              } // 
            if (SLOTA==17) {
              SLOTA=18; 
              AudioSelection1="GN18"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 17 to 18");
              } // 
            if (SLOTA==16) {
              SLOTA=17; 
              AudioSelection1="GN17"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 16 to 17");
              } // 
            if (SLOTA==15) {
              SLOTA=16; 
              AudioSelection1="GN16"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 15 to 16");
              } //        
            if (SLOTA==14) {
              SLOTA=15; 
              AudioSelection1="GN15"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 14 to 15");
              } // 
            if (SLOTA==13) {
              SLOTA=14; 
              AudioSelection1="GN14"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 13 to 14");
              } // 
            if (SLOTA==12) {
              SLOTA=13; 
              AudioSelection1="GN13"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 12 to 13");
              } // 
            if (SLOTA==11) {
              SLOTA=12; 
              AudioSelection1="GN12"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 11 to 12");
              } //         
            if (SLOTA==10) {
              SLOTA=11; 
              AudioSelection1="GN11"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 10 to 11");
              } // 
            if (SLOTA==9) {
              SLOTA=10; 
              AudioSelection1="GN10"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 9 to 10");
              } // 
            if (SLOTA==8) {
              SLOTA=9; 
              AudioSelection1="GN09"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 8 to 9");
              } // 
            if (SLOTA==7) {
              SLOTA=8; 
              AudioSelection1="GN08"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 7 to 8");
              } // 
            if (SLOTA==6) {
              SLOTA=7; 
              AudioSelection1="GN07"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 6 to 7");
              } // 
            if (SLOTA==5) {
              SLOTA=6; 
              AudioSelection1="GN06"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 5 to 6");
              } // 
            if (SLOTA==4) {
              SLOTA=5; 
              AudioSelection1="GN05"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 4 to 5");
              } //         
            if (SLOTA==3) {
              SLOTA=4; 
              AudioSelection1="GN04"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 3 to 4");
              } // 
            if (SLOTA==2) {
              SLOTA=3; 
              AudioSelection1="GN03"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 2 to 3");
              } // 
            if (SLOTA==1 && STATUSCHANGE==false) {
              SLOTA=2; 
              AudioSelection1="GN02"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 1 to 2");
              } // 
            if (SLOTA==100) {
              SLOTA=1; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 0 to 1");
              } //        
            STATUSCHANGE=false;
          }
          if (GETSLOT1){ // used for configuring manual team selection
            if (SLOTB==19) {
              SLOTB=1; 
              STATUSCHANGE=true; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon changed from 19 to 1");
              }          
            if (SLOTB==18) {
              SLOTB=19; 
              AudioSelection1="GN19";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 18 to 19");
              } // 
            if (SLOTB==17) {
              SLOTB=18; 
              AudioSelection1="GN18"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 17 to 18");
              } // 
            if (SLOTB==16) {
              SLOTB=17; 
              AudioSelection1="GN17";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 16 to 17");
              } // 
            if (SLOTB==15) {
              SLOTB=16; 
              AudioSelection1="GN16"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 15 to 16");
              } //        
            if (SLOTB==14) {
              SLOTB=15; 
              AudioSelection1="GN15"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 14 to 15");
              } // 
            if (SLOTB==13) {
              SLOTB=14; 
              AudioSelection1="GN14"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 13 to 14");
              } // 
            if (SLOTB==12) {
              SLOTB=13; 
              AudioSelection1="GN13"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 12 to 13");
              } // 
            if (SLOTB==11) {
              SLOTB=12; 
              AudioSelection1="GN12"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 11 to 12");
              } //         
            if (SLOTB==10) {
              SLOTB=11; 
              AudioSelection1="GN11"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 10 to 11");
              } // 
            if (SLOTB==9) {
              SLOTB=10; 
              AudioSelection1="GN10"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 9 to 10");
              } // 
            if (SLOTB==8) {
              SLOTB=9; 
              AudioSelection1="GN09";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 8 to 9");
              } // 
            if (SLOTB==7) {
              SLOTB=8; 
              AudioSelection1="GN08"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 7 to 8");
              } // 
            if (SLOTB==6) {
              SLOTB=7; 
              AudioSelection1="GN07"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 6 to 7");
              } // 
            if (SLOTB==5) {
              SLOTB=6; 
              AudioSelection1="GN06"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 5 to 6");
              } // 
            if (SLOTB==4) {
              SLOTB=5; 
              AudioSelection1="GN05"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 4 to 5");
              } //         
            if (SLOTB==3) {
              SLOTB=4; 
              AudioSelection1="GN04"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 3 to 4");
              } // 
            if (SLOTB==2) {
              SLOTB=3; 
              AudioSelection1="GN03"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 2 to 3");
              } // 
            if (SLOTB==1 && STATUSCHANGE==false) {
              SLOTB=2; 
              AudioSelection1="GN02"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 1 to 2");
              } // 
            if (SLOTB==100) {
              SLOTB=1; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 0 to 1");
              } //        
            STATUSCHANGE=false;
          }
        }
      }
      // alt fire button pressed and released section
      if (tokenStrings[1] == "1") {
        if (tokenStrings[2] == "1") {
          Serial.println("Alt fire pulled"); // yeah.. you pushed the red/yellow button
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Alt fire Released"); // now you released the button
          if (GETTEAM) {
            GETTEAM=false; 
            AudioSelection1="VAO"; 
            AUDIO1=true;
            }
          if (GETSLOT0) {
            GETSLOT0=false; 
            AudioSelection1="VAO"; 
            AUDIO1=true;
            }
          if (GETSLOT1) {
            GETSLOT1=false;
            AudioSelection1="VAO"; 
            AUDIO1=true;
          }
          if (SWAPBRX) {
            INITIATESWAPBRX = true; 
            AudioSelection1="VA76"; 
            AUDIO1=true;         
          }
        }
      }
      // charge or reload handle pulled and released section
      if (tokenStrings[1] == "2") {
        if (tokenStrings[2] == "1") {
          Serial.println("charge handle pulled"); // as indicated this is the handle
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Charge handle Released"); // goes without sayin... you let go of the handle
          // upon release of a handle, following steps occur
          //if(UNLIMITEDAMMO==2) { // checking if unlimited ammo is on or not
            //if (CurrentWeapSlot == 0) {
              //OutofAmmoA=true; // here is the variable for weapon slot 0
              //Serial.println("Weapon Slot 0 is out of ammo, enabling weapon reload for weapon slot 0");
            //}
            //if (CurrentWeapSlot == 1) {
               //OutofAmmoB=true; // trigger variable for weapon slot 1
               //Serial.println("Weapon Slot 1 is out of ammo, enabling weapon reload for weapon slot 1");
            //}
          //}
          }
      }
      // we can do more for left right select but yeah... maybe another time
    }
    
    // here is where we use the incoming ir to the guns sensors to our benefit in many ways
    // we can use it to id the player landing a tag and every aspect of that tag
    // but why stop there? we can use fabricated tags to set variables on the esp32 for programming game modes!
    if (tokenStrings[0] == "$HIR") {
      /*space 1 is the direction the tag came from or what sensor was hit, in above example sensor 4
        space 2 is for he type of tag recieved/shot. this is 0-15 for bullet type
        space 3 is the player id of the person sending the tag
        space 4 is the team id, 
        space 5 is the damage of the tag
        space 6 is "is critical" if critical a damage multiplier would apply, rare.
        space 7 is "power", not sure what that does.
      */
      //been tagged
      // run analysis on tag to determine if it carry's special instructions:
      TagPerks();
      lastTaggedPlayer = tokenStrings[3].toInt();
      lastTaggedTeam = tokenStrings[4].toInt();
      Serial.println("Just tagged by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
    }
    
    // this is a cool pop up... lots of info provided and broken out below.
    // this pops up everytime you pull the trigger and deplete your ammo.hmmm you
    // can guess what im using that for... see more below
// ****** note i still need to assign the ammo to the lcd outputs********
    if (tokenStrings[0] == "$ALCD") {
      /* ammunition status update occured 
       *  space 0 is ALCD indicator
       *  space 1 is rounds remaining in clip
       *  space 2 is accuracy of weapon
       *  space 3 is slot of weapon
       *  space 4 is remaining ammo outside clip
       *  space 5 is the overheating feature if aplicable
       *  
       *  more can be done with this, like using the ammo info to post to lcd
       */  
    }
    
    // this guy is recieved from brx every time your health changes... usually after
    // getting shot.. from the HIR indicator above. but this can tell us our health status
    // and we can send it to an LCD or use it to know when were dead and award points to the 
    // player who loast tagged the gun... hence the nomenclature "lasttaggedPlayer" etc
    // add in some kill counts and we can deplete lives and use it to enable or disable respawn functions
    if (tokenStrings[0] == "$HP") {
      /*health status update occured
       * can be used for updates on health as well as death occurance
       */
      if ((tokenStrings[1] == "0") && (tokenStrings[2] == "0") && (tokenStrings[3] == "0")) { // player is dead
        PlayerDied();
      }
    }
  } else {
    //hold data and keep receving
  }
}

//******************************************************************************************************
// BLE client stuff, cant say i understan all of it but yeah... had some help with this
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      doConnect = true;
      doScan = true; // enables scaning for designated BLE server (tagger)
      // WEAP = false; used to trigger sending game start commands to tagger
      Serial.println("onDisconnect");

    }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  Serial.println(" - Created client");
  // Connect to the remove BLE Server.
  if (pClient->connect(myDevice)) { // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteRXCharacteristic = pRemoteService->getCharacteristic(charRXUUID);
    if (pRemoteRXCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charRXUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our RX characteristic");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteTXCharacteristic = pRemoteService->getCharacteristic(charTXUUID);
    if (pRemoteTXCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charTXUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our TX characteristic");
    // Read the value of the characteristic.
    if (pRemoteRXCharacteristic->canRead()) {
      std::string value = pRemoteRXCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    if (pRemoteTXCharacteristic->canRead()) {
      std::string value = pRemoteTXCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    if (pRemoteTXCharacteristic->canNotify())
      pRemoteTXCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    return true;
  }
  return false;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      // We have found a device, let us now see if it contains the service we are looking for.
      //if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      if (advertisedDevice.getName() == BLE_SERVER_SERVICE_NAME) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = false;
      } // Found our server
      else {
        doScan = true;
      }
    } // onResult
}; // MyAdvertisedDeviceCallbacks

// ****************************************************************************************
/*Analyzing incoming tag to determine if a perk action is needed
        space 2 is for he type of tag recieved/shot. almost all are 0 and im thinking other types are medic etc.
        space 3 is the player id, in this case it was player 0 hitting player 1.
        space 4 is the team id, this was team 0 or red team
        space 5 is the damage dealt to the player, this took 13 hit points off the player 1
        space 6 is "is critical" if critical a damage multiplier would apply, rare.
        space 7 is "power", not sure what that does.*/
void TagPerks() {
  Serial.println("IR Direction: "+String(tokenStrings[1]));
  Serial.println("Bullet Type: "+String(tokenStrings[2]));
  Serial.println("Player ID: "+String(tokenStrings[3]));
  Serial.println("Team: "+String(tokenStrings[4]));
  Serial.println("Damage: "+String(tokenStrings[5]));
  Serial.println("Is Critical: "+String(tokenStrings[6]));
  Serial.println("Power: "+String(tokenStrings[7]));
  //  Checking for special tags
  if (tokenStrings[2] == "15" && tokenStrings[6] == "1" && tokenStrings[7] == "0") { // we just determined that it is a respawn tag
      Serial.println("Received a respawn tag");
      if (tokenStrings[4].toInt() == SetTeam) {
        Serial.println("Respawn Tag Team is a Match");
        if (PENDINGRESPAWNIR) { // checks if we are awaiting a respawn signal
          RESPAWN = true; // triggers a respawn
          PENDINGRESPAWNIR = false; // closing the process of checking for a respawining tag and enables all other normal in game functions
        }
      } else {
        Serial.println("Respawn Tag Team does not match");        
      }
    }
  if (tokenStrings[6] == "1" && tokenStrings[2] == "0") { // we determined that it is a custom tag for game interaction
    Serial.println("received a game interaction tag");
    // now that we know weve got a special tag, what to do with it...
    // respawning tag
    if (tokenStrings[7] == "3") { // we just determined that it is a respawn tag
      Serial.println("Received a respawn tag");
      if (tokenStrings[4].toInt() == SetTeam) {
        Serial.println("Respawn Tag Team is a Match");
        if (PENDINGRESPAWNIR) { // checks if we are awaiting a respawn signal
          RESPAWN = true; // triggers a respawn
          PENDINGRESPAWNIR = false; // closing the process of checking for a respawining tag and enables all other normal in game functions
        }
      } else {
        Serial.println("Respawn Tag Team does not match");        
      }
    }
    // objective tag
    if (tokenStrings[7] == "1") { // just determined that this is an objective tag - is critical, bullet 0, power 1, damage 1
      Serial.println("Received an Objective Tag");
      
        if (GameMode == 2) { // we're playing capture the flag
          if (tokenStrings[4].toInt() == SetTeam) {
            Serial.println("Objective Tag Team is a Match");
            AudioSelection1="VA2K"; // setting audio play to notify we captured the flag
            AUDIO1 = true; // enabling play back of audio
            HASFLAG = true; // set condition that we have flag to true
            SpecialWeapon = 99; // loads a new weapon that will deposit flag into base
            SPECIALWEAPON = true; // enables program to load the new weapon
          } else {
            Serial.println("Objective tag team does not match");
          }
        } else {
          CompletedObjectives++; // added one point to player objectives
          AudioSelection1="VAR"; // set an announcement "good job team"
          AUDIO1=true; // enabling BLE Audio Announcement Send
        }
    }
  }
}
//******************************************************************************************
void InitializeOTAUpdater() {
BLEDevice::deinit("");
Serial.println("deinitialized BLE Device");
WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ENABLEOTAUPDATE = true;
  INITIALIZEOTA = false;
}
// sets and sends game settings based upon the stored settings
void SetFFOutdoor() {
  // token one of the following command is free for all, 0 is off and 1 is on
  if(SetODMode == 0 && SetFF == 1) {
    sendString("$GSET,1,0,1,0,1,0,50,1,*");
    }
  if(SetODMode == 1 && SetFF == 1) {
    sendString("$GSET,1,1,1,0,1,0,50,1,*");
    }
  if(SetODMode == 1 && SetFF == 0) {
    sendString("$GSET,0,1,1,0,1,0,50,1,*");
    }
  if(SetODMode == 0 && SetFF == 0) {
    sendString("$GSET,0,0,1,0,1,0,50,1,*");
    }
}
//*****************************************************************************************
void SwapBRX() {
  if (weaponcounter < 19) {
    weaponcounter++;
  } else {
    weaponcounter = 2;
  }
  Serial.println("Weapon Counter = " + String(weaponcounter));
  SetSlotA = weaponcounter;
  weaponsettingsA();
  if(SetSlotA < 10) {
    AudioSelection = ("GN0" + String(SetSlotA));
  }
  if (SetSlotA > 9) {
    AudioSelection = ("GN" + String(SetSlotA));
  }
  AUDIO=true;  
}

//******************************************************************************************
// sets and sends gun type to slot 0 based upon stored settings
void weaponsettingsA() {
  if (SLOTA != 100) {
    SetSlotA=SLOTA; 
    SLOTA=100;
    Serial.println("set SetSlotA as SLOTA and reset SLOTA as 100");
  }
  if (UNLIMITEDAMMO==3){
    if(SetSlotA == 1) {
      Serial.println("Weapon 0 set to Unarmed"); 
      sendString("$WEAP,0,*");
    } // cleared out weapon 0
    if(SetSlotA == 2) {
      Serial.println("Weapon 0 set to AMR"); 
      sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,0,1400,10,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
    if(SetSlotA == 3) {
      Serial.println("Weapon 0 set to Assault Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,0,1400,10,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
    if(SetSlotA == 4) {
      Serial.println("Weapon 0 set to Bolt Rifle"); 
      sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,0,2000,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
    if(SetSlotA == 5) {
      Serial.println("Weapon 0 set to BurstRifle"); 
      sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,0,1700,10,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
    if(SetSlotA == 6) {
      Serial.println("Weapon 0 set to ChargeRifle"); 
      sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,0,2500,10,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
    if(SetSlotA == 7) {
      Serial.println("Weapon 0 set to Energy Launcher");
      sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,0,1400,10,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
    if(SetSlotA == 8) {
      Serial.println("Weapon 0 set to Energy Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,0,2400,10,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
    if(SetSlotA == 9) {
      Serial.println("Weapon 0 set to Force Rifle");
      sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,0,1700,10,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
    if(SetSlotA == 10) {
      Serial.println("Weapon 0 set to Ion Sniper"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,0,2000,10,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
    if(SetSlotA == 11) {
      Serial.println("Weapon 0 set to Laser Cannon"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,0,2000,10,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
    if(SetSlotA == 12) {
      Serial.println("Weapon 0 set to Plasma Sniper");
      sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,0,2000,10,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
    if(SetSlotA == 13) {
      Serial.println("Weapon 0 set to Rail Gun");
      sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,0,2400,10,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
    if(SetSlotA == 14) {
      Serial.println("Weapon 0 set to Rocket Launcher");
      sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,0,1200,10,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
    if(SetSlotA == 15) {
      Serial.println("Weapon 0 set to Shotgun");
      sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,0,400,10,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
    if(SetSlotA == 16) {
      Serial.println("Weapon 0 set to SMG");
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,0,2500,10,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
    if(SetSlotA == 17) {
      Serial.println("Weapon 0 set to Sniper Rifle");
      sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,0,1700,10,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
    if(SetSlotA == 18) {
      Serial.println("Weapon 0 set to Stinger");
      sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,0,1700,10,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
    if(SetSlotA == 19) {
      Serial.println("Weapon 0 set to Suppressor"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,0,2000,10,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  }
  if (UNLIMITEDAMMO==2) {
    if(SetSlotA == 1) {
      Serial.println("Weapon 0 set to Unarmed");
      sendString("$WEAP,0,*");
    } // cleared out weapon 0
    if(SetSlotA == 2) {
      Serial.println("Weapon 0 set to AMR"); 
      sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,32768,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,9999999,75,,*");
    }
    if(SetSlotA == 3) {
      Serial.println("Weapon 0 set to Assault Rifle");
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*");
    }
    if(SetSlotA == 4) {
      Serial.println("Weapon 0 set to Bolt Rifle");
      sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,32768,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,9999999,75,,*");
    }
    if(SetSlotA == 5) {
      Serial.println("Weapon 0 set to BurstRifle"); 
      sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,32768,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,9999999,75,,*");
    }
    if(SetSlotA == 6) {
      Serial.println("Weapon 0 set to ChargeRifle"); 
      sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*");
    }
    if(SetSlotA == 7) {
      Serial.println("Weapon 0 set to Energy Launcher");
      sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,32768,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,9999999,75,,*");
    }
    if(SetSlotA == 8) {
      Serial.println("Weapon 0 set to Energy Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,32768,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,9999999,75,,*");
    }
    if(SetSlotA == 9) {
      Serial.println("Weapon 0 set to Force Rifle"); 
      sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,32768,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,9999999,75,,*");
    }
    if(SetSlotA == 10) {
      Serial.println("Weapon 0 set to Ion Sniper"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*");
    }
    if(SetSlotA == 11) {
      Serial.println("Weapon 0 set to Laser Cannon"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,32768,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,9999999,75,,*");
    }
    if(SetSlotA == 12) {
      Serial.println("Weapon 0 set to Plasma Sniper"); 
      sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,32768,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,9999999,75,40,*");
    }
    if(SetSlotA == 13) {
      Serial.println("Weapon 0 set to Rail Gun"); 
      sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,32768,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,9999999,75,,*");
    }
    if(SetSlotA == 14) {
      Serial.println("Weapon 0 set to Rocket Launcher"); 
      sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,75,30,*");
    }
    if(SetSlotA == 15) {
      Serial.println("Weapon 0 set to Shotgun"); 
      sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,32768,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,9999999,75,30,*");
    }
    if(SetSlotA == 16) {
      Serial.println("Weapon 0 set to SMG"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,32768,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,9999999,75,,*");
    }
    if(SetSlotA == 17) {
      Serial.println("Weapon 0 set to Sniper Rifle");
      sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,32768,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,9999999,75,,*");
    }
    if(SetSlotA == 18) {
      Serial.println("Weapon 0 set to Stinger");
      sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,32768,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,9999999,75,,*");
    }
    if(SetSlotA == 19) {
      Serial.println("Weapon 0 set to Suppressor"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,32768,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,9999999,75,,*");
    }
  }
  if (UNLIMITEDAMMO==1) {
      if(SetSlotA == 1) {
        Serial.println("Weapon 0 set to Unarmed");
        sendString("$WEAP,0,*");
        } // cleared out weapon 0
      if(SetSlotA == 2) {
        Serial.println("Weapon 0 set to AMR");
        sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
        }
      if(SetSlotA == 3) {
        Serial.println("Weapon 0 set to Assault Rifle"); 
        sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
        }
      if(SetSlotA == 4) {
        Serial.println("Weapon 0 set to Bolt Rifle"); 
        sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
        }
      if(SetSlotA == 5) {
        Serial.println("Weapon 0 set to BurstRifle"); 
        sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
        }
      if(SetSlotA == 6) {
        Serial.println("Weapon 0 set to ChargeRifle");
        sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
        }
      if(SetSlotA == 7) {
        Serial.println("Weapon 0 set to Energy Launcher"); 
        sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
        }
      if(SetSlotA == 8) {
        Serial.println("Weapon 0 set to Energy Rifle"); 
        sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
        }
      if(SetSlotA == 9) {
        Serial.println("Weapon 0 set to Force Rifle"); 
        sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
        }
      if(SetSlotA == 10) {
        Serial.println("Weapon 0 set to Ion Sniper");
        sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
        }
      if(SetSlotA == 11) {
        Serial.println("Weapon 0 set to Laser Cannon"); 
        sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
        }
      if(SetSlotA == 12) {
        Serial.println("Weapon 0 set to Plasma Sniper");
        sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
        }
      if(SetSlotA == 13) {
        Serial.println("Weapon 0 set to Rail Gun");
        sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
        }
      if(SetSlotA == 14) {
        Serial.println("Weapon 0 set to Rocket Launcher"); 
        sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
        }
      if(SetSlotA == 15) {
        Serial.println("Weapon 0 set to Shotgun"); 
        sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
        }
      if(SetSlotA == 16) {
        Serial.println("Weapon 0 set to SMG"); 
        sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
        }
      if(SetSlotA == 17) {
        Serial.println("Weapon 0 set to Sniper Rifle"); 
        sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
        }
      if(SetSlotA == 18) {
        Serial.println("Weapon 0 set to Stinger");
        sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
        }
      if(SetSlotA == 19) {
        Serial.println("Weapon 0 set to Suppressor"); 
        sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
        }
  }
}
//*****************************************************************************************
// In Game Weapon Upgrade
void LoadSpecialWeapon() {
  if(SpecialWeapon == 1) {
    Serial.println("Weapon 0 set to Unarmed"); 
    sendString("$WEAP,0,*");
    } // cleared out weapon 0
  if(SpecialWeapon == 2) {
    Serial.println("Weapon 0 set to AMR"); 
    sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
  if(SpecialWeapon == 3) {
    Serial.println("Weapon 0 set to Assault Rifle");
    sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
  if(SpecialWeapon == 4) {
    Serial.println("Weapon 0 set to Bolt Rifle"); 
    sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
  if(SpecialWeapon == 5) {
    Serial.println("Weapon 0 set to BurstRifle");
    sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
  if(SpecialWeapon == 6) {
    Serial.println("Weapon 0 set to ChargeRifle");
    sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
  if(SpecialWeapon == 7) {
    Serial.println("Weapon 0 set to Energy Launcher"); 
    sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 8) {
    Serial.println("Weapon 0 set to Energy Rifle");
    sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
  if(SpecialWeapon == 9) {
    Serial.println("Weapon 0 set to Force Rifle");
    sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
  if(SpecialWeapon == 10) {
    Serial.println("Weapon 0 set to Ion Sniper"); 
    sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
  if(SpecialWeapon == 11) {
    Serial.println("Weapon 0 set to Laser Cannon"); 
    sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
  if(SpecialWeapon == 12) {
    Serial.println("Weapon 0 set to Plasma Sniper"); 
    sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
  if(SpecialWeapon == 13) {
    Serial.println("Weapon 0 set to Rail Gun"); 
    sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 14) {
    Serial.println("Weapon 0 set to Rocket Launcher"); 
    sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
  if(SpecialWeapon == 15) {
    Serial.println("Weapon 0 set to Shotgun"); 
    sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
  if(SpecialWeapon == 16) {
    Serial.println("Weapon 0 set to SMG"); 
    sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
  if(SpecialWeapon == 17) {
    Serial.println("Weapon 0 set to Sniper Rifle"); 
    sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
  if(SpecialWeapon == 18) {
    Serial.println("Weapon 0 set to Stinger"); 
    sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
  if(SpecialWeapon == 19) {
    Serial.println("Weapon 0 set to Suppressor"); 
    sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  if(SpecialWeapon == 99) {
    Serial.println("Flag Capture, Gun becomes the flag"); 
    sendString("$WEAP,0,*"); 
    sendString("$WEAP,1,*");
    sendString("$WEAP,1,*"); 
    sendString("$WEAP,3,,100,2,0,0,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,32768,75,,*");
    } 
}

//******************************************************************************************

// sets and sends gun for slot 0 based upon stored settings
void weaponsettingsB() {
  if (SLOTB != 100) {
    SetSlotB=SLOTB; 
    SLOTB=100;
  }
  if (UNLIMITEDAMMO==3){
    if(SetSlotB == 1) {
      Serial.println("Weapon 1 set to Unarmed"); 
      sendString("$WEAP,1,*");
    } // cleared out weapon 0
    if(SetSlotB == 2) {
      Serial.println("Weapon 1 set to AMR"); 
      sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,0,1400,10,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
    if(SetSlotB == 3) {
      Serial.println("Weapon 1 set to Assault Rifle"); 
      sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,0,1400,10,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
    if(SetSlotB == 4) {
      Serial.println("Weapon 1 set to Bolt Rifle"); 
      sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,0,2000,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
    if(SetSlotB == 5) {
      Serial.println("Weapon 1 set to BurstRifle");
      sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,0,1700,10,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
    if(SetSlotB == 6) {
      Serial.println("Weapon 1 set to ChargeRifle"); 
      sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,0,2500,10,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
    if(SetSlotB == 7) {
      Serial.println("Weapon 1 set to Energy Launcher"); 
      sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,0,1400,10,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
    if(SetSlotB == 8) {
      Serial.println("Weapon 1 set to Energy Rifle"); 
      sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,0,2400,10,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
    if(SetSlotB == 9) {
      Serial.println("Weapon 1 set to Force Rifle"); 
      sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,0,1700,10,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
    if(SetSlotB == 10) {
      Serial.println("Weapon 1 set to Ion Sniper"); 
      sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,0,2000,10,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
    if(SetSlotB == 11) {
      Serial.println("Weapon 1 set to Laser Cannon"); 
      sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,0,2000,10,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
    if(SetSlotB == 12) {
      Serial.println("Weapon 1 set to Plasma Sniper");
      sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,0,2000,10,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
    if(SetSlotB == 13) {
      Serial.println("Weapon 1 set to Rail Gun"); 
      sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,0,2400,10,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
    if(SetSlotB == 14) {
      Serial.println("Weapon 1 set to Rocket Launcher");
      sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,0,1200,10,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
    if(SetSlotB == 15) {
      Serial.println("Weapon 1 set to Shotgun");
      sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,0,400,10,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
    if(SetSlotB == 16) {
      Serial.println("Weapon 1 set to SMG"); 
      sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,0,2500,10,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
    if(SetSlotB == 17) {
      Serial.println("Weapon 1 set to Sniper Rifle"); 
      sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,0,1700,10,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
    if(SetSlotB == 18) {
      Serial.println("Weapon 1 set to Stinger"); 
      sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,0,1700,10,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
    if(SetSlotB == 19) {
      Serial.println("Weapon 1 set to Suppressor"); 
      sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,0,2000,10,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  }  
  if (UNLIMITEDAMMO==2) {
      if(SetSlotB == 1) {
        Serial.println("Weapon 1 set to Unarmed"); 
        sendString("$WEAP,1,*");
        } // cleared out weapon 0
      if(SetSlotB == 2) {
        Serial.println("Weapon 1 set to AMR"); 
        sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,32768,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,9999999,75,,*");
        }
      if(SetSlotB == 3) {
        Serial.println("Weapon 1 set to Assault Rifle");
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*");
        }
      if(SetSlotB == 4) {
        Serial.println("Weapon 1 set to Bolt Rifle"); 
        sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,32768,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,9999999,75,,*");
        }
      if(SetSlotB == 5) {
        Serial.println("Weapon 1 set to BurstRifle"); 
        sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,32768,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,9999999,75,,*");
        }
      if(SetSlotB == 6) {
        Serial.println("Weapon 1 set to ChargeRifle"); 
        sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*");
        }
      if(SetSlotB == 7) {
        Serial.println("Weapon 1 set to Energy Launcher"); 
        sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,32768,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,9999999,75,,*");
        }
      if(SetSlotB == 8) {
        Serial.println("Weapon 1 set to Energy Rifle");
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,32768,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,9999999,75,,*");
        }
      if(SetSlotB == 9) {
        Serial.println("Weapon 1 set to Force Rifle");
        sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,32768,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,9999999,75,,*");
        }
      if(SetSlotB == 10) {
        Serial.println("Weapon 1 set to Ion Sniper"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*");
        }
      if(SetSlotB == 11) {
        Serial.println("Weapon 1 set to Laser Cannon");
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,32768,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,9999999,75,,*");
        }
      if(SetSlotB == 12) {
        Serial.println("Weapon 1 set to Plasma Sniper");
        sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,32768,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,9999999,75,40,*");
        }
      if(SetSlotB == 13) {
        Serial.println("Weapon 1 set to Rail Gun");
        sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,32768,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,9999999,75,,*");
        }
      if(SetSlotB == 14) {
        Serial.println("Weapon 1 set to Rocket Launcher"); 
        sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,75,30,*");
        }
      if(SetSlotB == 15) {
        Serial.println("Weapon 1 set to Shotgun"); 
        sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,32768,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,9999999,75,30,*");
        }
      if(SetSlotB == 16) {
        Serial.println("Weapon 1 set to SMG"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,32768,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,9999999,75,,*");
        }
      if(SetSlotB == 17) {
        Serial.println("Weapon 1 set to Sniper Rifle"); 
        sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,32768,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,9999999,75,,*");
        }
      if(SetSlotB == 18) {
        Serial.println("Weapon 1 set to Stinger");
        sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,32768,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,9999999,75,,*");
        }
      if(SetSlotB == 19) {
        Serial.println("Weapon 1 set to Suppressor"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,32768,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,9999999,75,,*");
        }
    }
    if (UNLIMITEDAMMO==1) {
      if(SetSlotB == 1) {
        Serial.println("Weapon 1 set to Unarmed");
        sendString("$WEAP,1,*");
        } // cleared out weapon 0
      if(SetSlotB == 2) {
        Serial.println("Weapon 1 set to AMR"); 
        sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
        }
      if(SetSlotB == 3) {
        Serial.println("Weapon 1 set to Assault Rifle"); 
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
        }
      if(SetSlotB == 4) {
        Serial.println("Weapon 1 set to Bolt Rifle");
        sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
        }
      if(SetSlotB == 5) {
        Serial.println("Weapon 1 set to BurstRifle"); 
        sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
        }
      if(SetSlotB == 6) {
        Serial.println("Weapon 1 set to ChargeRifle");
        sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
        }
      if(SetSlotB == 7) {
        Serial.println("Weapon 1 set to Energy Launcher");
        sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
        }
      if(SetSlotB == 8) {
        Serial.println("Weapon 1 set to Energy Rifle"); 
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
        }
      if(SetSlotB == 9) {
        Serial.println("Weapon 1 set to Force Rifle"); 
        sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
        }
      if(SetSlotB == 10) {
        Serial.println("Weapon 1 set to Ion Sniper"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
        }
      if(SetSlotB == 11) {
        Serial.println("Weapon 1 set to Laser Cannon"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
        }
      if(SetSlotB == 12) {
        Serial.println("Weapon 1 set to Plasma Sniper"); 
        sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
        }
      if(SetSlotB == 13) {
        Serial.println("Weapon 1 set to Rail Gun"); 
        sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
        }
      if(SetSlotB == 14) {
        Serial.println("Weapon 1 set to Rocket Launcher"); 
        sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
        }
      if(SetSlotB == 15) {
        Serial.println("Weapon 1 set to Shotgun"); 
        sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
        }
      if(SetSlotB == 16) {
        Serial.println("Weapon 1 set to SMG"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
        }
      if(SetSlotB == 17) {
        Serial.println("Weapon 1 set to Sniper Rifle"); 
        sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
        }
      if(SetSlotB == 18) {
        Serial.println("Weapon 1 set to Stinger"); 
        sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
        }
      if(SetSlotB == 19) {
        Serial.println("Weapon 1 set to Suppressor"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
        }
    }
}

void weaponsettingsC() {
  if(SetSlotC == 0){
    Serial.println("Weapon 3 set to Unarmed"); 
    sendString("$WEAP,6,*"); // Set weapon 3 as unarmed
  }
  if(SetSlotC == 1){
    Serial.println("Weapon 3 set to Respawn"); 
    sendString("$WEAP,6,1,90,15,0,6,100,,,,,,,,1000,100,1,0,0,10,15,100,100,,0,0,,,,,,,,,,,,,,1,0,20,,*"); // Set weapon 3 as respawn
  }
  if(SetSlotD == 0){
    Serial.println("Weapon 5 set to Unarmed"); 
    sendString("$WEAP,7,*"); // Set weapon 5 as unarmed
  }
  if(SetSlotD == 4) {
    Serial.println("Weapon 5 set to Medic");
    sendString("$WEAP,7,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 2) {
    Serial.println("Weapon 5 set to Sheilds");
    sendString("$WEAP,7,1,90,2,1,70,0,,,,,,,,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 3) {
    Serial.println("Weapon 5 set to Armor");
    sendString("$WEAP,7,1,90,3,0,70,0,,,,,,,,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 1) {
    Serial.println("Weapon 5 set to Tear Gas");
    sendString("$WEAP,7,2,100,11,1,1,0,,,,,,1,80,1400,50,10,0,0,10,11,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,10,9999999,75,30,*");
  }
  if(SetSlotD == 7) {
    Serial.println("Weapon 5 set to Medic at Range");
    sendString("$WEAP,7,2,100,1,0,20,0,,,,,,20,80,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 6) {
    Serial.println("Weapon 5 set to Armor at Range");
    sendString("$WEAP,7,2,100,2,1,30,0,,,,,,40,80,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 5) {
    Serial.println("Weapon 5 set to Sheilds at range");
    sendString("$WEAP,7,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotE == 1) {
    Serial.println("Weapon 4 set to Sheilds at range");
    sendString("$WEAP,7,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotE == 2) {
    Serial.println("Weapon 4 set to Sheilds at range");
    sendString("$WEAP,7,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotE == 3) {
    Serial.println("Weapon 4 set to Sheilds at range");
    sendString("$WEAP,7,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }  
}
//****************************************************************************************
//************** This sends Settings to Tagger *******************************************
//****************************************************************************************
// loads all the game configuration settings into the gun
void gameconfigurator() {
  Serial.println("Running Game Configurator based upon recieved inputs");
  Serial.println("Clearing settings from brx");
  sendString("$CLEAR,*");
  sendString("$START,*");
  SetFFOutdoor();
  playersettings();
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  // sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  /* SIR commands to configure the incoming hits recognition
   *  $SIR,0,0,,1,0,0,1,,*  Assault Rifle Energy Rifle  Ion Sniper  Laser Cannon  Plasma Sniper Shotgun SMG Stinger Suppressor
   *  $SIR,0,1,,36,0,0,1,,* Force Rifle Sniper Rifle         
   *  $SIR,0,3,,37,0,0,1,,* AMR Bolt Rifle  BurstRifle              
   *  $SIR,1,0,H29,10,0,0,1,,*  Respawn and Add HP                  
   *  $SIR,2,1,VA8C,11,0,0,1,,* Add Shields                      
   *  $SIR,3,0,VA16,13,0,0,1,,* Add Armor                
   *  $SIR,6,0,H02,1,0,90,1,40,*  Rail Gun                    
   *  $SIR,8,0,,38,0,0,1,,* ChargeRifle               
   *  $SIR,9,3,,24,10,0,,,* Energy Launcher               
   *  $SIR,10,0,X13,1,0,100,2,60,*  Rocket Launcher               
   *  $SIR,11,0,VA2,28,0,0,1,,* Tear Gas - not working yet                        
   *  $SIR,13,0,H50,1,0,0,1,,*  Energy Blade 
   *  $SIR,13,1,H57,1,0,0,1,,*  Rifle Bash                  
   *  $SIR,13,3,H49,1,0,100,0,60,*  WarHammer               
   *  $SIR,15,0,H29,10,0,0,1,,* Add HP Or Respawns Player
   */
  sendString("$SIR,0,0,,1,0,0,1,,*");
  sendString("$SIR,0,1,,36,0,0,1,,*");
  sendString("$SIR,0,3,,37,0,0,1,,*");
  sendString("$SIR,1,0,,10,0,0,1,,*");
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*");
  sendString("$SIR,3,0,VA16,13,0,0,1,,*");
  sendString("$SIR,6,0,H02,1,0,90,1,40,*");
  sendString("$SIR,8,0,,38,0,0,1,,*");
  sendString("$SIR,9,3,,24,10,0,,,*");
  sendString("$SIR,10,0,X13,1,0,100,2,60,*");
  sendString("$SIR,11,0,VA2,28,0,0,1,,*");
  sendString("$SIR,13,1,H57,1,0,0,1,,*");
  sendString("$SIR,13,0,H50,1,0,0,1,,*");
  sendString("$SIR,13,3,H49,1,0,100,0,60,*");
  sendString("$SIR,15,0,,14,0,0,1,,");
  // The $SIR functions above can be changed to incorporate more in game IR based functions (health boosts, armor, shields) or customized over BLE to support game functions/modes
  sendString("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
  if (SWAPBRX) {
    sendString("$BMAP,1,98,,,,,*"); // sets the alt fire weapon to alternate between weapon 0, 1, 6 & 7
  } else {
    sendString("$BMAP,1,100,0,1,6,7,*"); // sets the alt fire weapon to alternate between weapon 0, 1, 6 & 7
  }
  sendString("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
  sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5
  sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4
  sendString("$BMAP,5,3,,,,,*"); // Sets the right button as weapon 3, using for perks/respawns etc. 
  sendString("$BMAP,8,4,,,,,*"); // sets the gyro as weapon 4
  Serial.println("Finished Game Configuration set up");
}

//****************************************************************************************
//************************ This starts a game *******************************************
//****************************************************************************************
// this starts a game
void delaystart() {
  Serial.println("Starting Delayed Game Start");
  //sendString("$PLAY,VA84,4,5,,,,,*"); // plays a ten second countdown
  sendString("$HLED,,6,,,,,*"); // changes headset to end of game
  // this portion creates a hang up in the program to delay until the time is up
  long actualdelay = 0; // used to count the actual delay versus desired delay
  long delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
  long delaycounter = millis(); // this will be used to track current time in milliseconds and compared to the start of the delay
  int audibletrigger = 0; // used as a trigger once we get to 10 seconds left
  if (DelayStart > 10) {
  while (DelayStart > actualdelay) { // this creates a sub loop in the object to keep doing the following steps until this condition is met... actual delay is the same as planned delay
    delaycounter = millis(); // sets the delay clock to the current progam timer
    actualdelay = delaycounter - delaybeginning; // calculates how long weve been delaying the program/start
    if ((DelayStart-actualdelay) < 10000) {
      audibletrigger++;
      } // a check to start adding value to the audible trigger
    if (audibletrigger == 1) {
      sendString("$PLAY,VA83,4,6,,,,,*"); 
      Serial.println("Playing ten second countdown");
      } // this can only happen once so it doesnt keep looping in the program we only play it when trigger is equal to 1
  }
  }
  sendString("$PLAY,VA81,4,6,,,,,*"); // plays the .. nevermind
  sendString("$PLAYX,0,*");
  sendString("$SPAWN,,*");
  Serial.println("Delayed Start Complete, should be in game play mode now");
  GameStartTime=millis();
  GAMEOVER=false;
  INGAME=true;
  if (GameTimer > 120000) {
    COUNTDOWN1=true;
    Serial.println("enabled countdown timer 1");
    } else {
      COUNTDOWN3=true;
    Serial.println("enabled countdown timer 3");
      } // enables the appropriate countdown announcements
}

//******************************************************************************************

// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
// had some major help from Sri Lanka Guy!
void sendString(String value) {
  const char * c_string = value.c_str();
  uint8_t buf[21] = {0};
  int sentSize = 0;
  Serial.print("sending: ");
  Serial.println(value);
  if (value.length() > 20) {
    for (int i = 0; i < value.length() / 20; i++) {
      memcpy(buf, c_string + i * 20, 20);
      Serial.print((char*)buf);
      pRemoteRXCharacteristic->writeValue(buf, 20, true);
      sentSize += 20;
    }
    int remaining = value.length() - sentSize;
    memcpy(buf, c_string + sentSize, remaining);
    pRemoteRXCharacteristic->writeValue(buf, remaining, true);
    for (int i = 0; i < remaining; i++)
      Serial.print((char)buf[i]);
    Serial.println();
  }
  else {
    pRemoteRXCharacteristic->writeValue((uint8_t*)value.c_str(), value.length(), true);
  }
}


//******************************************************************************************
// sets and sends player settings to gun based upon configuration settings
void playersettings() {
  // token 2 is player id or gun id other tokens were interested in modification
  // are 2 for team ID, 3 for max HP, 4 for max Armor, 5 for Max Shields, 
  // 6 is critical damage bonus
  // We really are only messing with Gender and Team though
  // Gender is determined by the audio call outs listed, tokens 9 and on
  // male is default as 0, female is 1
  // health = 45; armor = 70; shield =70;
  if(SetGNDR == 0) {
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,V33,V3I,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
  else {
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,VB3,VBI,VBC,VBG,VBE,VB7,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
}
//******************************************************************************************

//******************************************************************************************

// ends game... thats all
void gameover() {
  sendString("$STOP,*"); // stops everything going on
  sendString("$CLEAR,*"); // clears out anything stored for game settings
  sendString("$PLAY,VS6,4,6,,,,,*"); // says game over
  GAMEOVER = false;
  INGAME=false;
  COUNTDOWN1=false;
  COUNTDOWN2=false;
  COUNTDOWN3=false;
}

//******************************************************************************************

// as the name says... respawn a player!
void respawnplayer() {
  sendString("$HLOOP,2,1200,*"); // flashes the headset lights in a loop
  // this portion creates a hang up in the program to delay until the time is up
  if (SetRSPNMode == 9) {
    gameconfigurator();
  }
  if (RespawnTimer < RespawnTimerMax) {
    RespawnTimer = 5000 * Deaths;
  }
  long actualdelay = 0; // used to count the actual delay versus desired delay
  long delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
  long delaycounter = millis(); // this will be used to track current time in milliseconds and compared to the start of the delay
  int audibletrigger = 0; // used as a trigger once we get to 10 seconds left
  if (RespawnTimer > 10) {
  while (RespawnTimer > actualdelay) { // this creates a sub loop in the object to keep doing the following steps until this condition is met... actual delay is the same as planned delay
    delaycounter = millis(); // sets the delay clock to the current progam timer
    actualdelay = delaycounter - delaybeginning; // calculates how long weve been delaying the program/start
    if ((RespawnTimer-actualdelay) < 3000) {
      audibletrigger++;
      } // a check to start adding value to the audible trigger
    if (audibletrigger == 1) {
      sendString("$PLAY,VA80,4,6,,,,,*"); 
      Serial.println("playing 3 second countdown");
      } // this can only happen once so it doesnt keep looping in the program we only play it when trigger is equal to 1
  }
  }
  Serial.println("Respawning Player");
  //sendString("$WEAP,0,*"); // cleared out weapon 0
  //sendString("$WEAP,1,*"); // cleared out weapon 1
  //sendString("$WEAP,4,*"); // cleared out melee weapon
  //sendString("$WEAP,3,*"); // cleared out melee weapon
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  sendString("$GLED,,,,5,,,*"); // changes headset to tagged out color
  sendString("$SPAWN,,*"); // respawns player back in game
  Serial.println("Player Respawned");
  RESPAWN = false;
}
//****************************************************************************************
// disclaimer... incomplete... 
// this function will be used when a player is eliminated and needs to respawn off of a base
// or player signal to respawn them... a lot to think about still on this and im using auto respawn 
// for now untill this is further thought out and developed
void ManualRespawnMode() {
  sendString("$STOP,*"); // this is essentially ending the game for player... need to rerun configurator or use a different command
  sendString("$SPAWN,,*");
  sendString("$HLOOP,2,1200,*");
  AudioSelection1 = "VA54";
  AUDIO1 = true;
  PENDINGRESPAWNIR = true;
  MANUALRESPAWN = false;
}
//****************************************************************************
void Audio() {
  if (AUDIO) {
    if(AudioPlayCounter == 0) {
      AudioPlayCounter++; 
      sendString("$PLAY,"+AudioSelection+",4,6,,,,,*");
      AudioSelection = "NULL"; // clears the audio so no odd repeats that i may have left open ended
      TurnOffAudio=true;
      }
    }
  if (AUDIO1) {
    sendString("$PLAY,"+AudioSelection1+",4,6,,,,,*");
    if (AudioSelection1=="VA20" && SWAPBRX == false) {
      sendString("$CLEAR,*");
      sendString("$START,*");
    }
    AudioSelection1 = "NULL"; // clears audio so no odd audio repeats that i left around in the code
    AUDIO1=false; 
    TurnOffAudio=false;
    }
  if (SWAPBRX) {
    GAMESTART = true;
  }
}
//******************************************************************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  int A = 0;
  int B = 1;
  int C = 2;
  int D = 3;
  //Serial.println("listening to lora"); // check used for testing to make sure this loop is running
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
        LoRatokenStrings[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        }
        // the data is now stored into individual strings under tokenStrings[]
        // print out the individual string arrays separately
        Serial.println("received LoRa communication");
        Serial.print("Token 0: "); // identifier for sender (+REC=900) this is our sender
        Serial.println(LoRatokenStrings[0]);
        Serial.print("Token 1: "); // this is a value identifier for how many bytes are in the message
        Serial.println(LoRatokenStrings[1]);
        Serial.print("Token 2: "); // this is the actual data recevied or message
        Serial.println(LoRatokenStrings[2]);
        Serial.print("Token 3: "); // this is just a range indicator for transmission speed/distance
        Serial.println(LoRatokenStrings[3]);
        Serial.print("Token 4: "); // same as token 3
        Serial.println(LoRatokenStrings[4]);
        int BRXCommand = LoRatokenStrings[2].toInt();
        Serial.print("LoRa Command Received: " +String(BRXCommand));
      if(BRXCommand==1) {
        settingsallowed=1; 
        AudioSelection="VA5F";
        SetSlotA=100;
        Serial.println("Weapon Slot 0 set to Manual");
        }
      if(BRXCommand > 1 && BRXCommand < 50) {
        SetSlotA=BRXCommand-1;
        Serial.println("Weapon Slot 0 set"); 
        if(SetSlotA < 10) {
          AudioSelection = ("GN0" + String(SetSlotA));
        }
        if (SetSlotA > 9) {
          AudioSelection = ("GN" + String(SetSlotA));
        }
        }  
      // setting weapon slot 1
      if(BRXCommand==101) {
        settingsallowed=2; 
        AudioSelection="VA5F"; 
        SetSlotB=100; 
        Serial.println("Weapon Slot 1 set to Manual");
        }
      if(BRXCommand > 101 && BRXCommand < 150) {
        SetSlotB=BRXCommand-101; 
        Serial.println("Weapon Slot 1 set"); 
        if(SetSlotB < 10) {
          AudioSelection = ("GN0" + String(SetSlotB));
        }
        if (SetSlotB > 9) {
          AudioSelection = ("GN" + String(SetSlotB));
        }
      }
      if(BRXCommand == 200) {
        SetSlotC = 0; 
        Serial.println("Weapon Slot 6 set"); 
        AudioSelection="GN01";        
      }
      if(BRXCommand == 201) {
        SetSlotC = 1; 
        Serial.println("Weapon Slot 6 set"); 
        AudioSelection="VA9H";
      }
      if(BRXCommand == 300) {
        SetSlotD = 0; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="GN01";
      }
      if(BRXCommand == 301) {
        SetSlotD = 1; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="VA6G";
      }
      if(BRXCommand == 302) {
        SetSlotD = 2; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="VA5L";
      }
      if(BRXCommand == 303) {
        SetSlotD = 3; 
        Serial.println("Weapon Slot 7 set");
        AudioSelection="VA1G"; 
      }
      if(BRXCommand == 304) {
        SetSlotD = 4; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="VA4D";
      }
      if(BRXCommand == 305) {
        SetSlotD = 5; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="VA5L";
      }
      if(BRXCommand == 306) {
        SetSlotD = 6; 
        Serial.println("Weapon Slot 7 set"); 
        AudioSelection="VA1G";
      }
      if(BRXCommand == 307) {
        SetSlotD = 7; 
        Serial.println("Weapon Slot 7 set");
        AudioSelection="VA4D"; 
      }
      // setting player lives
      if(BRXCommand == 401) {
        SetLives=1;
        Serial.println("Player Lives set to " + String(PlayerLives)); 
        AudioSelection="VA01";
      }
      if(BRXCommand==402) {
        PlayerLives=3;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA03";
      }
      if(BRXCommand==403) {
        PlayerLives=10;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA0A";
      }
      if(BRXCommand==404) {
        PlayerLives=15;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA0F";
      }
      if(BRXCommand==405) {
        PlayerLives=25;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA0P";
      }
      if(BRXCommand==406) {
        PlayerLives=50;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA6V";
      }
      if(BRXCommand==407) {
        PlayerLives=32000;
        Serial.println("Player Lives set to " + String(PlayerLives));
        AudioSelection="VA6V";
      }
      // setting game time
      if(BRXCommand==501) {
        SetTime=60000; 
        Serial.println("Game time set to 1 minute");
        AudioSelection="VA0V";
        }
      if(BRXCommand==502) {
        SetTime=300000;
        Serial.println("Game time set to 5 minute");
        AudioSelection="VA2S";
        }
      if(BRXCommand==503) {
        SetTime=600000; 
        Serial.println("Game time set to 10 minute"); 
        AudioSelection="VA6H";
        }
      if(BRXCommand==504) {
        SetTime=900000;
        Serial.println("Game time set to 15 minute"); 
        AudioSelection="VA2P";
        }
      if(BRXCommand==505) {
        SetTime=1200000; 
        Serial.println("Game time set to 20 minute");
        AudioSelection="VA6Q";
        }
      if(BRXCommand==506) {
        SetTime=1500000;
        Serial.println("Game time set to 25 minute"); 
        AudioSelection="VA6P";
        }
      if(BRXCommand==507) {
        SetTime=1800000; 
        Serial.println("Game time set to 30 minute");
        AudioSelection="VA0Q";
        }
      if(BRXCommand==508) {
        SetTime=2000000000; 
        Serial.println("Game time set to Unlimited");
        AudioSelection="VA6V";
        }
      // set outdoor/indoor settings
      if(BRXCommand==602) {
        SetODMode=0;
        Serial.println("Outdoor Mode On"); 
        AudioSelection="VA4W";
        }
      if(BRXCommand==601) {
        SetODMode=1; 
        Serial.println("Indoor Mode On"); 
        AudioSelection="VA3W";
        }
      if(BRXCommand==603) {
        SetODMode=1;
        Serial.println("Stealth Mode On");
        AudioSelection="VA60";
        }
      // set team settings
      if(BRXCommand==701) {
        Serial.println("Free For All"); 
        SetTeam=0;
        SetFF=1;
        AudioSelection="VA30";
        }
      if(BRXCommand==702) {
        Serial.println("Teams Mode Two Teams (odds/evens)");
        if (GunID % 2) {
          SetTeam=0; 
          AudioSelection="VA13";
          } else {
            SetTeam=1; 
            AudioSelection="VA1L";
            }
        }
      if(BRXCommand==703) {
        Serial.println("Teams Mode Three Teams (every third player)");
        while (A < 64) {
          if (GunID == A) {
            SetTeam=0;
          }
          if (GunID == B) {
            SetTeam=1;
          }
          if (GunID == C) {
            SetTeam=2;
          }
          A = A + 3;
          B = B + 3;
          C = C + 3;
        }
        A = 0;
        B = 1;
        C = 2;
        AudioSelection="VA1L";
        }
      if(BRXCommand==704) {
        Serial.println("Teams Mode Four Teams (every fourth player)");
        while (A < 64) {
          if (GunID == A) {
            SetTeam=0;
          }
          if (GunID == B) {
            SetTeam=1;
          }
          if (GunID == C) {
            SetTeam=2;
          }
          if (GunID == D) {
            SetTeam=3;
          }
          A = A + 4;
          B = B + 4;
          C = C + 4;
          D = D + 4;
        }
        A = 0;
        B = 1;
        C = 2;
        D = 3;
        AudioSelection="VA1L";
        }
      if(BRXCommand==705) {
        Serial.println("Teams Mode Player's Choice"); 
        settingsallowed=3; 
        SetTeam=100; 
        AudioSelection="VA5E";
        } // this one allows for manual input of settings... each gun will need to select a team now
      // setting Melee Weapon
      if(BRXCommand == 801) {
        SetSlotE = 1; 
        Serial.println("Weapon Slot 4 set");
        AudioSelection="VA30"; 
      }
      if(BRXCommand == 802) {
        SetSlotE = 2; 
        Serial.println("Weapon Slot 4 set");
        AudioSelection="VA30"; 
      }
      if(BRXCommand == 803) {
        SetSlotE = 3; 
        Serial.println("Weapon Slot 4 set");
        AudioSelection="VA30"; 
      }
      // Setting Respawn Settings
      if(BRXCommand==901) {
        SetRSPNMode=1;
        RespawnTimer=10; 
        Serial.println("Respawn Set to Immediate"); 
        AudioSelection="VA54";
        }
      if(BRXCommand==902) {
        SetRSPNMode=2; 
        RespawnTimer=15000;
        Serial.println("Respawn Set to 15 seconds"); 
        AudioSelection="VA2Q";
        }
      if(BRXCommand==903) {
        SetRSPNMode=3;
        RespawnTimer=30000; 
        Serial.println("Respawn Set to 30 seconds"); 
        AudioSelection="VA0R";
        }
      if(BRXCommand==904) {
        SetRSPNMode=4; 
        RespawnTimer=45000; 
        Serial.println("Respawn Set to 45 seconds"); 
        AudioSelection="VA0T";
        }
      if(BRXCommand==905) {
        SetRSPNMode=5; 
        RespawnTimer=60000;
        Serial.println("Respawn Set to 60 seconds");
        AudioSelection="VA0V";
        }
      if(BRXCommand==906) {
        SetRSPNMode=6;
        RespawnTimer=90000;
        Serial.println("Respawn Set to 90 seconds"); 
        AudioSelection="VA0X";
        }
      if(BRXCommand==907) {
        SetRSPNMode=7; 
        RespawnTimer=120000;
        Serial.println("Respawn Set to 120 seconds");
        AudioSelection="VA0S";
        }
      if(BRXCommand==908) {
        SetRSPNMode=8; 
        RespawnTimer=150000;
        Serial.println("Respawn Set to 150 seconds");
        AudioSelection="VA0W";
        }
      if(BRXCommand==909) {
        SetRSPNMode=9;
        RespawnTimer=10; 
        Serial.println("Respawn Set to Manual/Respawn Station");
        AudioSelection="VA9H";
        }
      // set start delay
      if(BRXCommand==1001) {
        DelayStart=10;
        Serial.println("Delay Start Set to Immediate"); 
        AudioSelection="VA4T";
        }
      if(BRXCommand==1002) {
        DelayStart=15000;
        Serial.println("Delay Start Set to 15 seconds"); 
        AudioSelection="VA2Q";
        }
      if(BRXCommand==1003) {
        DelayStart=30000; 
        Serial.println("Delay Start Set to 30 seconds");
        AudioSelection="VA0R";
        }
      if(BRXCommand==1004) {
        DelayStart=45000; 
        Serial.println("Delay Start Set to 45 seconds");
        AudioSelection="VA0T";
        }
      if(BRXCommand==1005) {
        DelayStart=60000;
        Serial.println("Delay Start Set to 60 seconds"); 
        AudioSelection="VA0V";
        }
      if(BRXCommand==1006) {
        DelayStart=90000;
        Serial.println("Delay Start Set to 90 seconds");
        AudioSelection="VA0X";
        }
      if(BRXCommand==1007) {
        DelayStart=300000; 
        Serial.println("Delay Start Set to 5 minutes"); 
        AudioSelection="VA2S";
        }
      if(BRXCommand==1008) {
        DelayStart=600000; 
        Serial.println("Delay Start Set to 10 minutes"); 
        AudioSelection="VA6H";
        }
      if(BRXCommand==1009) {
        DelayStart=900000;
        Serial.println("Delay Start Set to 15 minutes"); 
        AudioSelection="VA2P";
        }
      // Sync Score Request Recieved
      if(BRXCommand==1101) {
        String Controller;
        Serial.println("Request Recieved to Sync Scoring");
        AudioSelection="VA91";
        if (tokenStrings[0] == "+REC=900") {
            Controller = "AT+SEND=900,";
          }
          if (tokenStrings[0] == "+REC=901") {
            Controller = "AT+SEND=901,";
          }
          if (tokenStrings[0] == "+REC=902") {
            Controller = "AT+SEND=902,";
          }
          if (tokenStrings[0] == "+REC=903") {
            Controller = "AT+SEND=903,";
          }
          if (tokenStrings[0] == "+REC=904") {
            Controller = "AT+SEND=900,";
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
        SendLoraMessage = "AT+SEND=" + String(killerplayer) + ",4,KILL\r\n";
        }
      // setting gender
      if(BRXCommand==1200) {
        SetGNDR=0; 
        Serial.println("Gender set to Male");
        AudioSelection="V3I";
        }
      if(BRXCommand==1201) {
        SetGNDR=1;
        Serial.println("Gender set to Female");
        AudioSelection="VBI";
        }
      // setting Ammunitions mode
      if(BRXCommand==1303) {
        UNLIMITEDAMMO=3; 
        Serial.println("Ammo set to unlimited rounds"); 
        AudioSelection="VA6V";
        }
      if(BRXCommand==1302) {
        UNLIMITEDAMMO=2;
        Serial.println("Ammo set to unlimited magazies"); 
        AudioSelection="VA6V";
        }
      if(BRXCommand==1301) {
        UNLIMITEDAMMO=1; 
        Serial.println("Ammo set to limited"); 
        AudioSelection="VA14";
        }
      // setting Freindly Fire mode
      if(BRXCommand==1401) {
        SetFF=1;
        Serial.println("Friendly Fire On"); 
        AudioSelection="VA32";
        }
      if(BRXCommand==1400) {
        SetFF=0; 
        Serial.println("Friendly Fire Off");
        AudioSelection="VA31";
        }
      // Setting Tagger volume
      if(BRXCommand == 1501) {
        SetVol = 20;
        Serial.println("Volume set to 20%");
        AudioSelection="VA01";
        VOLUMEADJUST=true;
      }
      if(BRXCommand == 1502) {
        SetVol = 40;
        Serial.println("Volume set to 40%");
        AudioSelection="VA02";
        VOLUMEADJUST=true;
      }
      if(BRXCommand == 1503) {
        SetVol = 60;
        Serial.println("Volume set to 60");
        AudioSelection="VA03";
        VOLUMEADJUST=true;
      }
      if(BRXCommand == 1504) {
        SetVol = 80;
        Serial.println("Volume set to 80%");
        AudioSelection="VA04";
        VOLUMEADJUST=true;
      }
      if(BRXCommand == 1505) {
        SetVol = 100;
        Serial.println("Volume set to 100");
        AudioSelection="VA05";
        VOLUMEADJUST=true;
      }
      // enabling game start
      Serial.println("Checking for game start");
      if (BRXCommand==99) {
        Serial.println("Received Game Start Command");
        Serial.println("Shutting down Lora receiver");
        LISTENTOLORA = false;
        Serial.println("Enabling game start object");
        GAMESTART=true; 
        Serial.println("Setting Audio to Null");
        AudioSelection="NULL";
        Serial.println("starting game");
        if (SetTeam == 100) {
          SetTeam=Team;
        }
        }
      if (BRXCommand==98) {
        LISTENTOLORA = true;
        GAMEOVER=true; 
        Serial.println("ending game");
        AudioSelection="NULL";
        }
      // enable audio notification for changes
      if(1600 > BRXCommand && BRXCommand > 0) {
        Serial.println("Enabling Audio Object to force BRX Callout");
        AUDIO=true;
      }
      // In Game Pick Ups
      if (BRXCommand==1701) {
        Serial.println("Random Weapon Pickup");
        SpecialWeapon = random(2, 19); // randomly assigning a weapon
        SPECIALWEAPON = true; // enables program to load the new weapon
        AudioSelection="VA76";
        }        
      if (BRXCommand == 151) {
        LISTENTOLORA = false;
        ENABLEBLE = false; 
        INITIALIZEOTA = true; 
        Serial.println("Disabling OTA Updates"); 
      }
      if (tokenStrings[2] == "KILL") {
        AudioSelection="VN9";
        KillCount++;
        Serial.println("Kill Count: " + String(KillCount));
        AUDIO=true;
      }
    }
}
}
//******************************************************************************************************************************************************************************************
void BlinkOnboardLED() {
  //loop to blink without delay
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    ledState = not(ledState);
    // set the LED with the ledState of the variable:
    digitalWrite(led,  ledState);
  }
}
void BLESetup(){
BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.

  /**
 * notes from possible ways to increase output power
 * 
 * bledevice::setPower(Powerlevel);
 * @brief Set the transmission power.
 * The power level can be one of:
 * * ESP_PWR_LVL_N14
 * * ESP_PWR_LVL_N11
 * * ESP_PWR_LVL_N8
 * * ESP_PWR_LVL_N5
 * * ESP_PWR_LVL_N2
 * * ESP_PWR_LVL_P1
 * * ESP_PWR_LVL_P4
 * * ESP_PWR_LVL_P7
 * @param [in] powerLevel.
 * esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P9);
 * esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
 * esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_P9); 
 */

  esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P7); // updated version 4/14/2020
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P7); // updated version 4/14/2020
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_P7); // updated version 4/14/2020
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  //BLEDevice::setPower(ESP_PWR_LVL_N14); // old version
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10, true);

  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
}
//****************************************************************************************
void PlayerDied() {
  if (HASFLAG) { HASFLAG = false; AudioSelection1 = "VA2I"; AUDIO1 = true;}
  int killerplayer = lastTaggedPlayer + 100;
  SendLoraMessage = "AT+SEND=" + String(killerplayer) + ",4,KILL\r\n";
  ENABLELORASEND = true;
  Serial.println("Sending via LoRa: " + SendLoraMessage);
  PlayerKillCount[lastTaggedPlayer]++; // adding a point to the last player who killed us
  TeamKillCount[lastTaggedTeam]++; // adding a point to the team who caused the last kill
  PlayerLives--; // taking our preset lives and subtracting one life then talking about it on the monitor
  Deaths++;
  AudioSelection1="KBP"+String(lastTaggedPlayer);
  AUDIO1=true;
  Serial.println("Lives Remaining = " + String(PlayerLives));
  Serial.println("Killed by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
  Serial.println("Team: " + String(lastTaggedTeam) + "Score: " + String(TeamKillCount[lastTaggedTeam]));
  Serial.println("Player: " + String(lastTaggedPlayer) + " Score: " + String(PlayerKillCount[lastTaggedPlayer]));
  Serial.println("Death Count: " + String(Deaths));
  if (PlayerLives > 0 && SetRSPNMode < 9) { // doing a check if we still have lives left after dying
    RESPAWN = true;
    Serial.println("Auto respawn enabled");
  }
  if (PlayerLives > 0 && SetRSPNMode == 9) { // doing a check if we still have lives left after dying
    MANUALRESPAWN = true;
    Serial.println("Manual respawn enabled");
  }
  if (PlayerLives == 0) {
    GAMEOVER=true; 
    AudioSelection1="VA46"; 
    AUDIO1=true; 
    Serial.println("lives depleted");
  }
}
//****************************************************************************************
void ControlBRX() {
  // the main loop for BLE activity is here, it is devided in three sections....
  // sections are for when connected, when not connected and to connect again
//***********************************************************************************
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      doConnect = false; // stop trying to make the connection.
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
  }
//*************************************************************************************
  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    //pRemoteCharacteristic->writeValue((uint8_t*)newValue.c_str(), newValue.length(),true);
    // here we put in processes to run based upon conditions to make a game functions, only runs if not in game
    if (AUDIO) {
      Audio();
    } else {TurnOffAudio=false;}
    if (AUDIO1) {
      Audio();
    }
    if (INGAME == false) {
    if (settingsallowed1==3) {
      Serial.println("Team Settings requested"); 
      delay(250);
      GETTEAM=true;
      GETSLOT0=false;
      GETSLOT1=false; 
      settingsallowed1=0;
      }
    if (settingsallowed1==1) {
      Serial.println("Weapon Slot 0 Requested"); 
      delay(250);
      GETSLOT0=true; 
      GETSLOT1=false;
      GETTEAM=false; 
      settingsallowed1=0;
      }
    if (settingsallowed1==2) {
      Serial.println("Weapon Slot 1 Requested"); 
      delay(250); 
      GETSLOT1=true;
      GETSLOT0=false;
      GETTEAM=false; 
      settingsallowed1=0;
      }
    if (settingsallowed>0) {
      Serial.println("manual settings requested"); 
      settingsallowed1=settingsallowed;
      } // this is triggered if a manual option is required for game settings    
    if (VOLUMEADJUST) {
      VOLUMEADJUST=false;
      sendString("$VOL,"+String(SetVol)+",0,*"); // sets max volume on gun 0-100 feet distance
    }    
    if (GAMESTART) {
      // need to turn off the trigger audible selections if a player didnt press alt fire to confirm
      Serial.println("game start function beginning...");
      GETSLOT0=false; 
      GETSLOT1=false;  
      GETTEAM=false;
      Serial.println("just reset values for manual options and locked them out if left open");
      gameconfigurator(); // send all game settings, weapons etc. 
      delaystart(); // enable the start based upon delay selected
      GAMESTART = false; // disables the trigger so this doesnt loop/
      PlayerLives=SetLives; // sets configured lives from settings received
      GameTimer=SetTime; // sets timer from input of esp8266
      Deaths = 0;
      LISTENTOLORA = true;
    }
    }
    // In game checks and balances to execute actions for in game functions
    if (INGAME) {
      long ActualGameTime = millis() - GameStartTime;
      long GameTimeRemaining = GameTimer - ActualGameTime;
      if (ActualGameTime > GameTimer) {
        GAMEOVER=true; 
        Serial.println("game time expired");
        }
      if (OutofAmmoA) {
        weaponsettingsA();      
        Serial.println("Weapon Slot 0 has been reloaded, disabling reload");
        OutofAmmoA=false;
      }
      if (OutofAmmoB) {
        weaponsettingsB();
        Serial.println("Weapon Slot 1 has been reloaded, disabling reload");
        OutofAmmoB=false;
      }
      if (RESPAWN) { // checks if auto respawn was triggered to respawn a player
        respawnplayer(); // respawns player
      }
      if (MANUALRESPAWN) { // checks if manual respawn was triggered to respawn a player
        ManualRespawnMode();
      }
      if (GAMEOVER) { // checks if something triggered game over (out of lives, objective met)
        gameover(); // runs object to kick player out of game
        LISTENTOLORA = true;
      }
      if (COUNTDOWN1) {
        if (GameTimeRemaining < 120001) {
          AudioSelection1="VSD";
          AUDIO1=true; 
          COUNTDOWN1=false; 
          COUNTDOWN2=true;
          Serial.println("2 minutes remaining");
          } // two minute warning
      }
      if (COUNTDOWN2) {
        if (GameTimeRemaining < 60001) {
          AudioSelection1="VSA"; 
          AUDIO1=true;
          COUNTDOWN2=false; 
          COUNTDOWN3=true; 
          Serial.println("1 minute remaining");
          } // one minute warning
      }
      if (COUNTDOWN3) {
        if (GameTimeRemaining < 10001) {
          AudioSelection1="VA83"; 
          AUDIO1=true; 
          COUNTDOWN3=false;
          Serial.println("ten seconds remaining");
          } // ten second count down
      }
      if (SPECIALWEAPON) {
        SPECIALWEAPON = false;
        LoadSpecialWeapon();
      }
      if (INITIATESWAPBRX) {
        INITIATESWAPBRX = false; // disables swap trigger
        SwapBRX();
      }
    }  
//************************************************************************************
  } else if (doScan) {
    if (millis() - startScan > 5000) { // scan for 5 seconds
      Serial.println("Scanning again");
      BLEDevice::init("");
      BLEDevice::getScan()->start(10, true); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
      startScan = millis();
    }
  }
}
//***************************************************************************************
void SendViaLora() {
  Serial1.print(SendLoraMessage); // sending posession status to master
  Serial.print("Sent: " + String(SendLoraMessage) + ", Via LoRa");
 }
//***************************************************************************************
void InitializeLora() {
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  
  //**************************************************************
  //************ CHANGE ME START *********************************
  //**************************************************************
  Serial1.print(LoRaID);   //needs to be unique
  //**************************************************************
  //************ CHANGE ME END ***********************************
  //**************************************************************
  
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
}
//**********************************************************************************************************************************************



//******************************************************************************************
//*********************** THIS IS A LOOP FOR SECOND CORE ***********************************
//******************************************************************************************
// serial communications that is pinned to second core in set up
void loop1(void *pvParameters){
  InitializeLora(); // runs lora set up once
  while (1) { // starts an infinite loop
    if (settingsallowed1>0) {
      settingsallowed=0;
      } // checking that a trigger was set from other core, if so, disabling it on this core
    if (TurnOffAudio) {
      AUDIO=false;
      AudioPlayCounter=0;
    }
    if (LISTENTOLORA) {
      ReceiveTransmission(); // listens to LoRa
    }
    if (ENABLELORASEND) {
      ENABLELORASEND = false;
      SendViaLora();
    }
    delay(1);
  }
}
//******************************************************************************************
//*********** THIS IS A LOOP FOR FIRST CORE *************************************************************************
//******************************************************************************************
void loop2(void *pvParameters) {
  while (1) { // starts infinite loop
    if (INITIALIZEOTA) {
      InitializeOTAUpdater();
    }
    while (ENABLEOTAUPDATE) {
      ArduinoOTA.handle();
      BlinkOnboardLED();
      }
    while (ENABLEBLE) {
      ControlBRX();
    }
    delay(1);
  }
}
//******************************************************************************************
//***************** THIS IS THE SET UP ******************************************************************
//******************************************************************************************
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  // delay(5000); not needed anymore
  BLESetup();
  pinMode(led, OUTPUT);
  digitalWrite(led,  ledState);
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
} // End of setup.

//******************************************************************************************
//********** EMPY LOOP *************************************************************************
//******************************************************************************************
// This is the Arduino main loop function for the BLE Core.
void loop() {
  // EMPTY LOOP
} // End of loop
