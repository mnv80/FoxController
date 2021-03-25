//-----------------------------------------------------------------------------
// Fox Hunt Controller
// For ATMEL chips
// Version 4 - move morse routines into a library
// Version 3 - replaces RTC with DTMF
//
// Use:
//  1. Download and install the CW and DTMF libraries into your IDE
//  2. Edit the FOX_MSG to identify the specific fox in which this is used.
//  3. Edit the FOX_ID to the station id to use for this transmission.
//  4. Optionally edit any other macros in the Fox Configuration Values section.
//  5. Compile and upload to your hardware as usual.  
//  6. Connect to your radio, turn it on and set the frequency.  
//  7. From another radio send a DTMF 1 in order to activate the fox.
//  8. From another radio send a DTMF 0 in order to deactivate the fox.
//  9. Adjust the squelch and volume if needed to get reliable response to DTMF tones.
// 10. While active the controller will repeat 200 times:
//        Send a series of tones 
//        Send FOX_MSG and FOX_ID (in CW)
//        Pause for IDLE_TIME
//-----------------------------------------------------------------------------
#include <CW.h>
#include <DTMF.h>

//-----------------------------------------------------------------------------
// Fox Configuration Values
//-----------------------------------------------------------------------------
#define PTT_PIN             10
#define TX_PIN               6
#define RX_PIN              A0
#define LED_PIN             13
#define WPM                 20
#define CW_FREQ            600
#define TUNE_FREQ          500
#define TUNE_LIMIT          20
#define FOX_MSG             "MSG - Change Me"
#define FOX_ID              "ID - Change Me"
#define IDLE_TIME           20

//-----------------------------------------------------------------------------
// DTMF Decode Thresholds
// Larger values mean longer press required to detect, but fewer false positives
//-----------------------------------------------------------------------------
#define DECODE_THRESHOLD  3000.0

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
CW           cw = CW(PTT_PIN, TX_PIN, LED_PIN, WPM, CW_FREQ);
DTMF       dtmf = DTMF(8928.57);
int  cycleLimit = 0;  // when fox stops transmitting

//-----------------------------------------------------------------------------
// Transmission Routines
//-----------------------------------------------------------------------------

// 900ms of tone + 100ms of silence, repeated TUNE_LIMIT times
void sendFoxTone() {
   cw.pttOn();
   for (int i=0; i<TUNE_LIMIT; i++) {
      cw.tune( TUNE_FREQ, 900 );
      delay(100);
   }
   cw.pttOff();
}

// send FOX_MSG with PTT control
void sendFoxMessage() {
   cw.pttOn();
   cw.send(FOX_MSG);
   cw.pttOff();
}

// send FOX_ID with PTT control
void sendFoxID() {
   cw.pttOn();
   cw.send("DE ");
   cw.send(FOX_ID);
   cw.pttOff();
}

//-----------------------------------------------------------------------------
// DTMF Processing
//-----------------------------------------------------------------------------
int OKtoGo() {
  if (cycleLimit <= 0) {
    return 0;
  } else {
    cycleLimit--;
    return 1;
  }
}

void checkDTMF(int ms) {
   unsigned long timeOut = millis() + (unsigned long) ms;
   char c;

   while (timeOut > millis()) {
     dtmf.sample(RX_PIN);
     //dtmf.dumpSample();
     
     dtmf.detect(511);
     //dtmf.dumpMagnitudes();
     
     c = dtmf.decode(DECODE_THRESHOLD);  
     //Serial.println(c, HEX);
     
     if (c == 0) continue;  // DTMF is idle
     if (c == 1) continue;  // DTMF button still down
     
     //otherwise we have a new button press
     processDTMF(c);
     return;
   }
}

void waitForDTMFIdle() {
   while (1) {
     dtmf.sample(RX_PIN);
     dtmf.detect(511);
     if (dtmf.decode(DECODE_THRESHOLD) == 0) 
        return;
   }
}

void processDTMF(char c) {
  char *responseMessage = " OK ";  

  switch (c) {
    case '1':
      cycleLimit = 200; //run for a while 
      break;
      
    case '0':
      cycleLimit = 0;  //stop now
      break;
      
    default:
      responseMessage = " ? ";
  } 
  
  waitForDTMFIdle();
  cw.pttOn();
  cw.send(responseMessage);
  cw.pttOff();
}

//-----------------------------------------------------------------------------
// Setup
//-----------------------------------------------------------------------------
void setup() {    
   //Serial.begin(9600);
   
   pinMode(RX_PIN, INPUT);     
   delay(1000);  // initial delay after powering on
}

//-----------------------------------------------------------------------------
// Loop
//-----------------------------------------------------------------------------
void loop() {

   // if we are active - send the burst
   if (OKtoGo()) {
      sendFoxTone();
      delay(500);
      sendFoxMessage();
      delay(500);
      sendFoxID();
   }   

   // look for DTMF during the idle time
   checkDTMF(IDLE_TIME * 1000);
}
