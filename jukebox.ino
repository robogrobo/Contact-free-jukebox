/*
  jukebox

  this is the firmware for the contact-free jukebox.
  that can play audio tracks depending on rfid tags.
  
  attached to the arduino is an adafruit waveshield
  and an id-12 rfid reader.
  
  created @ 8.1.12 by dominik grob (@ruedi)
  
  credits to charlie's rfid teddy bear
  
  website: scalotta.tumblr.com
*/

#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"

#include <SoftwareSerial.h>

//  sdcard settings
SdReader card;
FatVolume vol;
FatReader root;
FatReader f;

WaveHC wave;

//  rfid settings
SoftwareSerial RFIDSerial(8, 6); 

int RFIDResetPin = 7;
char RFIDtag[14];
int RFIDindex = 0;
boolean RFIDreading = false;

//  define tag id and tracks
#define NUMTAGS 8
char audiotags[NUMTAGS][14] = {"0100C4B7B2C0",
                               "0100C4A04C29",
                               "0100C49F6339",
                               "0100C48689CA",
                               "0100C4868AC9",
                               "4800B48C4C3C",
                               "4800B4CD90A1",
                               "4800B4838BF4"};
// make sure soundfile names are not longer then 8 chars (without filetype)
char audiofiles[NUMTAGS][14] = {"elefant.wav",
                                "pinguin.wav",
                                "nikki.wav",
                                "wildsau.wav",
                                "esel.wav",
                                "papagei.wav",
                                "krokodil.wav",
                                "nacht.wav"};

void setup() {
  Serial.begin(9600);
  putstring_nl("debug");
  putstring_nl("----------------");
  
  putstring("Free RAM: ");
  Serial.println(freeRam());
  
  //  set output pins for DAC control
  //  pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  // pin13 LED
  pinMode(13, OUTPUT);
   
  if (!card.init()) {
    putstring_nl("Card init. failed!");
    sdErrorCheck();
    while(1);
  }
  
  // enable optimize read
  card.partialBlockRead(true);
 
  // fat partition?
  uint8_t part;
  for (part = 0; part < 5; part++) {
    if (vol.init(card, part)) 
      break;
  }
  if (part == 5) {
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();
    while(1);
  }
  
  // show infos
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);
  
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!");
    while(1);
  }
  
  putstring_nl("> sdcard ready");
  
  // rfid setup
  pinMode(RFIDResetPin, OUTPUT);
  digitalWrite(RFIDResetPin, HIGH);
  RFIDSerial.begin(9600);
  
  putstring_nl("> rfid ready");
  
  // play startup chime
  delay(500);  // avoid loudspeaker click noise
  playcomplete("chime.wav");
}

void loop() {
  RFIDindex = 0;

  //  rfid data?
  while(RFIDSerial.available()) {
    int readByte = RFIDSerial.read();

    if(readByte == 2) RFIDreading = true;
    if(readByte == 3) RFIDreading = false;

    if(RFIDreading && readByte != 2 && readByte != 10 && readByte != 13){
      RFIDtag[RFIDindex] = readByte;
      RFIDindex++;
    }
  }

  //  check tag and play track if tag id found
  checkTag(RFIDtag);
  //  prepare for next read
  clearTag(RFIDtag);  
  resetReader(); 
}

void playcomplete(char *name) {
  playfile(name);
  while (wave.isplaying) {
    // playing
  }
}

void playfile(char *name) {
  if (wave.isplaying) {
    wave.stop();
  }
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.println(name); return;
  }
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  wave.play();
}

void resetReader() {
  digitalWrite(RFIDResetPin, LOW);
  digitalWrite(RFIDResetPin, HIGH);
  delay(150);
}

void clearTag(char one[]) {
  for(int i = 0; i < strlen(one); i++){
    one[i] = 0;
  }
}

void checkTag(char tag[]) {
  if(strlen(tag) == 0) return;
  
  boolean matching = true;

  //  compare tag id
  for(int a = 0; a < NUMTAGS; a++) {
    matching = true;
    for(int c = 0; c < 12; c++) {
      if(tag[c] != audiotags[a][c]) {
        matching = false;
        break;
      }
    }
    
    //  in case of a match play the track
    if(matching) {
      putstring("playing: "); Serial.println(audiofiles[a]);
      digitalWrite(RFIDResetPin, LOW);
      delay(500);  // avoid loudspeaker click noise
      playcomplete(audiofiles[a]);
      break;
    }
  }
}


// ---------------------------
// debug functions
// ---------------------------
int freeRam(void) {
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}
