/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


 /* Instructions:

    - to run this application you need an ESP32 with PSRAM installed and an SD-CARD slot (ie TTGO VGA32 v1.4 or FabGL Development Board with WROVER)
    - open this with Arduino and make sure PSRAM is DISABLED
    - partition scheme must be: Huge App
    - compile and upload the sketch

 */

 /* maybe useful:
 * Optional SD Card connections:
 *   MISO => GPIO 16  (2 for PICO-D4)
 *   MOSI => GPIO 17  (12 for PICO-D4)
 *   CLK  => GPIO 14
 *   CS   => GPIO 13
 *
 * To change above assignment fill other paramaters of FileBrowser::mountSDCard().
 */

#include "inputbox.h"
//#include "fabui.h"

#pragma message "This sketch requires Tools->Partition Scheme = Huge APP"


#include <memory>

#include "esp32-hal-psram.h"
extern "C" {
#include "esp_spiram.h"
}
#include "esp_sntp.h"

#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "fabgl.h"

#include "mconf.h"
#include "machine.h"

#include <iostream>
#include <string>

FileBrowser fb("/");

//#define SD_MOUNT_PATH "/SD"

#define SPIFFS_MOUNT_PATH  "/flash"
#define FORMAT_ON_FAIL     true

#define WIFI_DELAY        500 // Delay paramter for connection.
#define MAX_CONNECT_TIME  10000 // Wait this much until device gets IP. 

using std::unique_ptr;
using fabgl::StringList;
using fabgl::imin;
using fabgl::imax;

Preferences   preferences;
InputBox      ibox;
Machine     * machine;

// noinit! Used to maintain datetime between reboots
__NOINIT_ATTR static timeval savedTimeValue;

static bool wifiConnected = false;
static bool downloadOK    = false;

bool hasSD = true; // tem microSD?
bool audioMode = false; // output from Monitor activided
char path[30] = "/";

#include <SPI.h>

//SD Card
//#define SD_CS 22
#define SD_CS 13
#define SPI_MOSI 12
#define SPI_MISO 2
#define SPI_SCK 14

// for the SD use
#include "SD.h"




// For App chatterbox
#include <DNSServer.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include "HTMLPAGE.h"

#define FORMAT_SPIFFS_IF_FAILED true
#define RECORD_SEP "\x1E"

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer server(80);

const char* filename2 = "/posts.txt";


// for the app JPGView():
#include <TJpg_Decoder.h>
//#include <FS.h>
//#include "SPIFFS.h" // ESP32 only
fabgl::VGA16Controller VGAController;
//fabgl::VGAController VGAController; // dont show the complete image .jpg
fabgl::Canvas cv(&VGAController);


// For App Web Radios
#include <Arduino.h>
#include "Audio.h"
//#include <Preferences.h> 
//#include <WiFi.h>


  uint8_t max_volume   = 21;
  uint8_t cur_volume   = 10;  
  


// For Audio Player https://how2electronics.com/esp32-music-audio-mp3-player/
//int file_index = 1000;
//String file_list[20];
//int file_num = 0;
//String filename = " ";

/* void open_new_song(Audio audio, String filename)
{
  //Audio audio(true, 1, 0);
  //music_info.name = filename.substring(1, filename.indexOf("."));
  File file = SD.open(filename);
  //file.name() = filename;
  audio.connecttoFS(SD, file.name());
  //music_info.runtime = audio.getAudioCurrentTime();
  //music_info.length = audio.getAudioFileDuration();
  //music_info.volume = audio.getVolume();
  //music_info.status = 1;
  Serial.println("**********start a new sound************");

  /* while(1) {
  //audio.loop();
  if (audio.isRunning()) {
    Serial.println("cheguei aqui");
    audio.loop();
  }
  else
  {
    Serial.println("cheguei aqui2");
    break;
    //delay(1000);
    /*if (audio.isRunning()) {
     audio.connecttohost(urls[UrlSelector].c_str());
    }*/
  //}
  
  //audio.loop();

  /*if (PS2Controller::keyboard()->scancodeAvailable()) {
    int scode = PS2Controller::keyboard()->getNextScancode();
  if (scode == 22) { // 1 WebRadios
    WebRadios();
  }
  else if (scode == 30) { // 2 //ChatterBox bate papo horizontal
    ChatterBox();
  }
  //else if (scode == 38) { // 3 Mp3 Player

  //} 
  } 

    
}*/

/* void AudioMenu() {
  Serial.println("audioMenu iniciado");
  ibox.end();       // No more display needed
  FileBrowser::unmountSDCard(); // lets mount another path
  Audio audio(true, 1, 0);
  //Audio audio(true, 1, 0);
  //audio.setBufsize(15000, 0);
  audio.setVolume(12);
  pinMode(13, OUTPUT);      
  digitalWrite(13, HIGH);
  SPI.begin(14,2,12);
  SPI.setFrequency(1000000);
  SD.begin(13);
  //File root = SD.open("/"); // OK
  //if(!root){
  //  Serial.println("Failed to open directory");
  //  return;
  //}
  //audio.setVolume(12);
  //audio.connecttoFS(SD, "/audio.mp3");
  //if (!FileBrowser::mountSDCard(false, "/VGA32Audio/", 8)) Serial.println("Não montou");
  //if (!FileBrowser::mountSDCard(false, "/VGA32Audio", 8)) Serial.println("Não montou");
  //else {
  //audio.connecttoFS(SD, "/VGA32Audio/welcome.mp3");
  open_new_song(audio, "/VGA32Audio/welcome.mp3");
  
  open_new_song(audio, "/VGA32Audio/press.mp3");
  //open_new_song("/VGA32Audio/one.mp3");
  //open_new_song("/VGA32Audio/for.mp3");
  //open_new_song("/VGA32Audio/internet.mp3");
  //open_new_song("/VGA32Audio/radio.mp3");

} */

void audio_eof_mp3(const char *info) { //end of file
    Serial.print("eof_mp3     ");
    Serial.println(info);
    esp_restart();
    //if (audio.isRunning())    Serial.println("audio.isRunning");
    //playNextFile(); // play next file in directory if playing a multi-track episode
    
    /* file_index++;
    if (file_index >= file_num)
    {
        file_index = 0;
    }
    if (file_index < 999) open_new_song(file_list[file_index]); */

} 


// For app Web Radios
void audio_info(const char *info){
    Serial.print("audio_info: "); Serial.println(info);
}
void audio_showstation(const char *info){
    //write_stationName(String(info));
}
void audio_showstreamtitle(const char *info){
    String sinfo=String(info);
    sinfo.replace("|", "\n");
    //write_streamTitle(sinfo);
}

void PlayAudio(const char* filename){

  /* if(psramInit()){
          Serial.println("\nThe PSRAM is correctly initialized");
  }
  else{
          Serial.println("\nPSRAM does not work");
  } */


  
  Audio audio(true, 1, 0);
  //Serial.println("Increasing buffer size. ");
  //audio.setBufsize(15000, 0);
  //audio.setBufsize(1000, 0); A stack overflow in task IDLE0 has been detected.
  //audio.setBufsize(2000, 0); Não funcionou
  //audio.setBufsize(50000, 0); //Não funcionou 
  //audio.setBufsize(5000, 0); //inputBufferSize: 3399 bytes Não funcionou 

  
    //Serial.printf("Listing directory: %s\n", dirname);
    //int i = 0;
 
    //File root = fs.open(path);

  audio.forceMono(true); // necessary to have enough memory to play
  
  audio.setVolume(6); // 0...21
  //open_new_song(audio, file_list[file_index]);
  //open_new_song(audio, filename);
  //print_song_time();

  //filename = file_list[file_index];
      //file.name() = filename;
      //audio.connecttoFS(SD, file.name());
   //   audio.connecttoFS(SD, filename);


  //audio.connecttoFS(SD, filen3);
  audio.connecttoFS(SD, filename);

  while (1) {
    audio.loop();

   /* if (audio.isRunning()){
      Serial.println("audio.isRunning");
      audio.loop();
    }
    else {
      audio.stopSong();
      //file_index++;
    /* if (file_index >= file_num)
    {
        file_index = 0;
    }
    if (file_index < 999) open_new_song(audio, file_list[file_index]);
    }  */

    //File root = fs.open(path);
    /* root = fs.open(path);
    if (!root) {
        Serial.println("Failed to open directory");
        //return i;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        //return i;
    }
 
    File file = root.openNextFile();
    file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
        }
        else
        {
            filename = file.name();
            if (filename.endsWith(".wav"))
            {
              open_new_song(audio, filename);
            }
            else if (filename.endsWith(".mp3"))
            {
              open_new_song(audio, filename);
            }
        }
        file = root.openNextFile();
    } */
    

    // take a keypress
    /* int scode = PS2Controller::keyboard()->getNextScancode();
    Serial.println(scode);
    if (PS2Controller::keyboard()->scancodeAvailable()) {
      int scode = PS2Controller::keyboard()->getNextScancode();
      Serial.println(scode);
      if ((scode == 114) and (cur_volume>2)) {
        cur_volume = cur_volume - 2;
        audio.setVolume(cur_volume);
      }
      else if ((scode == 117) and (cur_volume < max_volume)) {
        cur_volume = cur_volume + 2;
        audio.setVolume(cur_volume);
      }
      else if (scode == 102) {  // get out backspace
        esp_restart();
      } 
                                
    //xprintf("%02X ", scode);
    //if (scode == 0xF0 || scode == 0xE0) ++clen;
    //--clen;
    //if (clen == 0) {
    //  clen = 1;
    //  xprintf("\r\n");
    //}
    }  */
    
  }
}




// try to connected using saved parameters
bool tryToConnect()
{
  Serial.println("tryToConnect Start");
  WiFi.setSleep(false);
  bool connected = WiFi.status() == WL_CONNECTED;
  if (!connected) {
    char SSID[32] = "";
    char psw[32]  = "";
    if (preferences.getString("SSID", SSID, sizeof(SSID)) && preferences.getString("WiFiPsw", psw, sizeof(psw))) {
      ibox.progressBox("", "Abort", true, 200, [&](fabgl::ProgressForm * form) {
        WiFi.begin(SSID, psw);
        for (int i = 0; i < 32 && WiFi.status() != WL_CONNECTED; ++i) {
          if (!form->update(i * 100 / 32, "Connecting to %s...", SSID))
            break;
          delay(500);
          if (i == 16)
            WiFi.reconnect();
        }
        connected = (WiFi.status() == WL_CONNECTED);
      });
      // show to user the connection state
      if (!connected) {
        WiFi.disconnect();
        ibox.message("", "WiFi Connection failed!");
      }
    }
  }
  return connected;
}


bool checkWiFi()
{
  Serial.println("checkWiFi Start");
  wifiConnected = tryToConnect();
  if (!wifiConnected) {

    // configure WiFi?
    if (ibox.message("WiFi Configuration", "Configure WiFi?", "No", "Yes") == InputResult::Enter) {

      // repeat until connected or until user cancels
      do {

        // yes, scan for networks showing a progress dialog box
        int networksCount = 0;
        ibox.progressBox("", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
          form->update(0, "Scanning WiFi networks...");
          networksCount = WiFi.scanNetworks();
        });

        // are there available WiFi?
        if (networksCount > 0) {

          // yes, show a selectable list
          StringList list;
          for (int i = 0; i < networksCount; ++i)
            list.appendFmt("%s (%d dBm)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
          int s = ibox.menu("WiFi Configuration", "Please select a WiFi network", &list);

          // user selected something?
          if (s > -1) {
            // yes, ask for WiFi password
            char psw[32] = "";
            if (ibox.textInput("WiFi Configuration", "Insert WiFi password", psw, 31, "Cancel", "OK", true) == InputResult::Enter) {
              // user pressed OK, connect to WiFi...
              preferences.putString("SSID", WiFi.SSID(s).c_str());
              preferences.putString("WiFiPsw", psw);
              wifiConnected = tryToConnect();
              // show to user the connection state
              if (wifiConnected)
                ibox.message("", "Connection succeeded!");
            } else
              break;
          } else
            break;
        } else {
          // there is no WiFi
          ibox.message("", "No WiFi network found!");
          break;
        }
        WiFi.scanDelete();

      } while (!wifiConnected);

    }

  }

  return wifiConnected;
}


// handle soft restart
void shutdownHandler()
{
  // save current datetime into Preferences
  gettimeofday(&savedTimeValue, nullptr);
}


void updateDateTime()
{
  // Set timezone
  setenv("TZ", "GMT0BST,M3.5.0/01,M10.5.0/02", 1);
  tzset();

//Example time zones
//const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";     // UK
//const char* Timezone = "MET-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";       // Central Europe
//const char* Timezone = "EST-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA  
//const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
//const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
//const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
//const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
//const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia

  // get datetime from savedTimeValue? (noinit section)
  if (esp_reset_reason() == ESP_RST_SW) {
    // adjust time taking account elapsed time since ESP32 started
    savedTimeValue.tv_usec += (int) esp_timer_get_time();
    savedTimeValue.tv_sec  += savedTimeValue.tv_usec / 1000000;
    savedTimeValue.tv_usec %= 1000000;
    settimeofday(&savedTimeValue, nullptr);
    return;
  }

  if (checkWiFi()) {

    // we need time right now
    ibox.progressBox("", nullptr, true, 200, [&](fabgl::ProgressForm * form) {
      sntp_setoperatingmode(SNTP_OPMODE_POLL);
      sntp_setservername(0, (char*)"pool.ntp.org");
      sntp_init();
      for (int i = 0; i < 12 && sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED; ++i) {
        form->update(i * 100 / 12, "Getting date-time from SNTP...");
        delay(500);
      }
      sntp_stop();
      ibox.setAutoOK(2);
      ibox.message("", "Date and Time updated. Restarting...");
      esp_restart();
    });

  } else {

    // set default time
    auto tm = (struct tm){ .tm_sec  = 0, .tm_min  = 0, .tm_hour = 8, .tm_mday = 14, .tm_mon  = 7, .tm_year = 84 };
    auto now = (timeval){ .tv_sec = mktime(&tm) };
    settimeofday(&now, nullptr);

  }
}


// download specified filename from URL
bool downloadURL(char const * URL, FILE * file)
{
  Serial.println("downloadURL start");
  downloadOK = false;

  char const * filename = strrchr(URL, '/') + 1;

  ibox.progressBox("", "Abort", true, 380, [&](fabgl::ProgressForm * form) {
    form->update(0, "Preparing to download %s", filename);
    HTTPClient http;
    http.begin(URL);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      if (file) {
        int tlen = http.getSize();
        int len = tlen;
        auto buf = (uint8_t*) SOC_EXTRAM_DATA_LOW; // use PSRAM as buffer
        WiFiClient * stream = http.getStreamPtr();
        int dsize = 0;
        while (http.connected() && (len > 0 || len == -1)) {
          size_t size = stream->available();
          if (size) {
            int c = stream->readBytes(buf, size);
            auto wr = fwrite(buf, 1, c, file);
            if (wr != c) {
              dsize = 0;
              break;  // writing failure!
            }
            dsize += c;
            if (len > 0)
              len -= c;
            if (!form->update((int64_t)dsize * 100 / tlen, "Downloading %s (%.2f / %.2f MB)", filename, (double)dsize / 1048576.0, tlen / 1048576.0))
              break;
          }
        }
        downloadOK = (len == 0 || (len == -1 && dsize > 0));
      }
    }
    http.end();
  });

  return downloadOK;
}


// return filename if successfully downloaded or already exist
char const * getDisk(char const * url)
{
  Serial.println("getDisk start");
  //FileBrowser fb(SD_MOUNT_PATH);
  FileBrowser fb("/SD");
  
  char const * filename = nullptr;
  //Serial.println(url);
  if (url) {
    if (strncmp("://", url + 4, 3) == 0) { // only http not https
      //Serial.println("Sim");
      // this is actually an URL
      filename = strrchr(url, '/') + 1;
      //Serial.println(filename);
      if (filename && !fb.exists(filename, false)) {
        //Serial.println("Sim tamb.");
        // disk doesn't exist, try to download
        if (!checkWiFi())
          return nullptr;
        auto file = fb.openFile(filename, "wb");
        bool success = downloadURL(url, file);
        fclose(file);
        if (!success) {
          fb.remove(filename);
          return nullptr;
        }
      }
    } else {
      // this is just a file
      if (fb.filePathExists(url)) {
        //Serial.println("Sim tamb. isso");
        filename = url;
      }
    }
  }
  Serial.println("getDisk end");
  return filename;
}


// user pressed SYSREQ (ALT + PRINTSCREEN)
void sysReqCallback()
{
  machine->graphicsAdapter()->enableVideo(false);
  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);

  int s = ibox.menu("", "Select a command", "Restart (Boot Menu);Continue;Mount Disk");
  switch (s) {

    // Restart
    case 0:
      esp_restart();
      break;

    // Mount Disk
    case 2:
    {
      int s = ibox.menu("", "Select Drive", "Floppy A (fd0);Floppy B (fd1)");
      if (s > -1) {
        constexpr int MAXNAMELEN = 256;
        unique_ptr<char[]> dir(new char[MAXNAMELEN + 1] { '/', 'S', 'D', 0 } );
        unique_ptr<char[]> filename(new char[MAXNAMELEN + 1] { 0 } );
        if (machine->diskFilename(s))
          strcpy(filename.get(), machine->diskFilename(s));
        if (ibox.fileSelector("Select Disk Image", "Image Filename", dir.get(), MAXNAMELEN, filename.get(), MAXNAMELEN) == InputResult::Enter) {
          machine->setDriveImage(s, filename.get());
        }
      }
      break;
    }

    // Continue
    default:
      break;
  }

  ibox.end();
  PS2Controller::keyboard()->enableVirtualKeys(false, false); // don't use virtual keys
  machine->graphicsAdapter()->enableVideo(true);
}





// For App chatterbox:
void sendPage(){
  Serial.println("GET /");
  server.send(200,"text/html",HTMLPAGE);  
}

void sendMessages(){
  Serial.println("GET /posts");
  File file = SPIFFS.open(filename2, FILE_READ);
  if(!file){
      Serial.println("- failed to open file for reading");
  }

  server.streamFile(file,"text/plain");
  
  file.close();
}

void receiveMessage(){
  Serial.println("POST /post");
  int argCount = server.args();
  if (argCount == 0){
    Serial.println("zero args?");
  }
  
  File file = SPIFFS.open(filename2, FILE_APPEND);
  if(!file){
      Serial.println("- failed to open file for writing");
  }
  if(argCount == 1){
    String line = server.arg(0);
    line.replace(String(RECORD_SEP),String(""));
    file.print(line);
    file.print(RECORD_SEP);
    Serial.print("New message: ");
    Serial.println(line);
  }
  file.close();
  
  server.send(200,"text.plain","");
}



// for the app JPGView():
void writePixel(int16_t x, int16_t y, uint16_t rgb565) 
{
  uint8_t r = ((rgb565 >> 11) & 0x1F) * 255 / 31;   // red   0 .. 255
  uint8_t g = ((rgb565 >> 5) & 0x3F) * 255 / 63.0;  // green 0 .. 255
  uint8_t b = (rgb565 & 0x1F) * 255 / 31.0;         // blue  0 .. 255
  cv.setPixel(x, y, RGB888(r, g, b));
}

// This next function will be called during decoding of the jpeg file to
// render each block to the Canvas.
bool vga_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if ( y >= cv.getHeight() ) return 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, bitmap[j * w + i]);
    }
  }

  // Return 1 to decode next block
  return 1;
}

// show image jpg
// code by https://github.com/fdivitto/FabGL/discussions/160#discussioncomment-1423074
void ShowJPG(char * filename)
{

  //Initialise SPIFFS:
  if(!SPIFFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }  
    
  Serial.print("filename: ");
  Serial.println(filename);
  fabgl::BitmappedDisplayController::queueSize = 1024; // deixou mais rápido, com mais 10 = extremamente lento (e não resolveu d mostrar tudo)
  //cv.waitCompletion(false);
  //cv.endUpdate();
  //cv.reset(); // pane com o de baixo
  //cv.resetPaintOptions(); // pane
  ibox.end();
  VGAController.begin();
  //VGAController.setResolution(VGA_640x480_60Hz, 512, 300);
  //VGAController.setResolution(VGA_512x300_60Hz);
  //VGAController.setResolution(VGA_400x300_60Hz);
  //VGAController.setResolution(VGA_512x384_60Hz, 512, 300);
  VGAController.setResolution(VGA_512x384_60Hz); // my monitor resolution is 1024 x 600
  VGAController.setProcessPrimitivesOnBlank(true);
  cv.setBrushColor(RGB888(0, 0, 0));
  cv.clear();
  //Serial.print("ho");
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  //Serial.print("directory: ");
  //Serial.println(directory); //'directory' was not declared in this scope
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(vga_output);
  // Time recorded for test purposes
  uint32_t t = millis();
  //Serial.print("hiy3");
  // Get the width and height in pixels of the jpeg if you wish
  uint16_t w = 0, h = 0;
  //strcpy(filename, filename2);
  //auto file = fb.openFile(filename, "wb");
  //strcpy(file, filename);
  //filename = std::to_string(file); //'to_string' is not a member of 'std'
  //filename = to_string(file);   'to_string' was not declared in this scope
  //TJpgDec.drawSdJpg(0, 0, filename);
  //fclose(file);
  //TJpgDec.getSdJpgSize(&w, &h, filename2); // Note name preceded with "/"

  //Resize if possible: (down scale...)
  
    TJpgDec.getFsJpgSize(&w, &h, String("/")+filename);
  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
  /* if (w < 320 and h < 240)
    {
    TJpgDec.setJpgScale(2);
    }
  else 
    TJpgDec.setJpgScale(1); */
    
  /* Resize if possible: (640x480)
  if (w > 5120 or h > 3840) 
    TJpgDec.setJpgScale(8);
  else if (w > 2560 or h > 1920)
    TJpgDec.setJpgScale(4);
  else if (w > 640 or h > 480)
    TJpgDec.setJpgScale(2);
  else 
    TJpgDec.setJpgScale(1);  */

  //Resize if possible: (512x300) // (for my monitor...)
  //if (w > 4096 or h > 2400) 
  if (w > 4301 or h > 2520) // crop 5%
    TJpgDec.setJpgScale(8);
  //if (w > 2048 or h > 1200)
  if (w > 2150 or h > 1260) //crop 5%
    //TJpgDec.setJpgScale(8);
    TJpgDec.setJpgScale(4);
  //else if (w > 2048 or h > 1200)
  //else if (w > 1024 or h > 600)
  else if (w > 1075 or h > 630) //crop 5%
    //TJpgDec.setJpgScale(4);
    TJpgDec.setJpgScale(2);
  //else if (w > 1024 or h > 600)
  //else if (w > 512 or h > 300)
  //else if (w > 538 or h > 315) //crop 5%
  //  TJpgDec.setJpgScale(2);
  else 
    TJpgDec.setJpgScale(1); 

  // Draw the image, top left at 0,0
  //TJpgDec.drawFsJpg(0, 0, filename); //file not found
  //TJpgDec.drawFsJpg(0, 0, "/panda.jpg"); // Ok
  //filename = "/" + filename; invalid operands of types 'const char [2]' and 'char*' to binary 'operator+'
  //filename = "/" += filename; invalid operands of types 'const char [2]' and 'char*' to binary 'operator+'
  //filename = '/' + filename;  dont work
  Serial.print("filename: ");
  Serial.println(filename);
  TJpgDec.drawFsJpg(0, 0, String("/")+filename); 
  //TJpgDec.drawSdJpg(0, 0, filename);  //file not found
  //TJpgDec.drawSdJpg(0, 0, "/moto.jpg"); //file not found
  //TJpgDec.drawSdJpg(0, 0, "SD/moto.jpg"); //file not found
  //TJpgDec.drawSdJpg(0, 0, "/SD/moto.jpg"); //file not found
  //TJpgDec.drawSdJpg(0, 0, "moto.jpg"); //file not found
  //TJpgDec.drawSdJpg(0, 0, "/panda.jpg".c_str());  // Jpeg file not found request for member 'c_str' in '"/panda.jpg"', which is of non-class type 'const char [11]'
  //TJpgDec.drawSdJpg(0, 0, "/moto.jpg"); //Jpeg file not found
  //TJpgDec.drawSdJpg(0, 0, "/moto.jpg"); // Jpeg file not found
  
  // How much time did rendering take
  t = millis() - t;
  Serial.print(t); Serial.println(" ms");

  // take a keypress
  int scode = PS2Controller::keyboard()->getNextScancode();

  // now user can choice another app:
  esp_restart();
   /*while (1){
    t = millis();
  } */
}



// For app File Download
// download from URL. Filename is the last part of the URL
void download(char const * URL) {
  tryToConnect();
  char const * slashPos = strrchr(URL, '/');
  if (slashPos) {
    char filename[slashPos - URL + 1];
    strcpy(filename, slashPos + 1);

    HTTPClient http;
    http.begin(URL);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {

      //int fullpathLen = fileBrowser->content().getFullPath(filename);
      int fullpathLen = 1;
      char fullpath[fullpathLen];
      //fileBrowser->content().getFullPath(filename, fullpath, fullpathLen);
      FILE * f = fopen(fullpath, "wb");

      int len = http.getSize();
      uint8_t buf[128] = { 0 };
      WiFiClient * stream = http.getStreamPtr();
      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available();
        if (size) {
          int c = stream->readBytes(buf, fabgl::imin(sizeof(buf), (int) size));
          fwrite(buf, c, 1, f);
          if (len > 0)
            len -= c;
        }
      }

      fclose(f);

      //updateBrowser();
    }
  }
} 






// App PCEmulator code
void PCEmulator()
{

  // try to start SD
  /* if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8))   // @TODO: reduce to 4?
    ibox.message("Error!", "This app requires a SD-CARD!", nullptr, nullptr); */

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
      //ibox.message("", "microSD-CARD not found!", nullptr, nullptr); 
      hasSD = false;
        Serial.println("SD not Found. ");
      }
   else {
        hasSD = true;
        Serial.println("SD Found. ");
   }

  // try to turn off the wifi
  WiFi.mode(WIFI_OFF);
  
  // we need PSRAM for this app, but we will handle it manually, so please DO NOT enable PSRAM on your development env

  #ifdef BOARD_HAS_PSRAM
  ibox.message("Warning!", "Please disable PSRAM to improve performance!");
  #endif

  // note: we use just 2MB of PSRAM so the infamous PSRAM bug should not happen. But to avoid gcc compiler hack (-mfix-esp32-psram-cache-issue)
  // we enable PSRAM at runtime, otherwise the hack slows down CPU too much (PSRAM_HACK is no more required).
  if (esp_spiram_init() != ESP_OK)
    ibox.message("Error!", "This app requires a board with PSRAM!", nullptr, nullptr);

  #ifndef BOARD_HAS_PSRAM
  esp_spiram_init_cache();
  #endif
  

  // machine configurations
  MachineConf mconf;

  // show a list of machine configurations

  ibox.setAutoOK(6);
  int idx = preferences.getInt("dconf", 0);

  for (bool showDialog = true; showDialog; ) {

    loadMachineConfiguration(&mconf);

    StringList dconfs;
    for (auto conf = mconf.getFirstItem(); conf; conf = conf->next)
      dconfs.append(conf->desc);
    dconfs.select(idx, true);

    ibox.setupButton(0, "Browse Files");
    ibox.setupButton(1, "Options", "Edit;New;Remove", 52);
    auto r = ibox.select("Machine Configurations", "Please select a machine configuration", &dconfs, nullptr, "Run");

    idx = dconfs.getFirstSelected();

    switch (r) {
      case InputResult::ButtonExt0:
        // Browse Files
        ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
        break;
      case InputResult::ButtonExt1:
        // Options
        switch (ibox.selectedSubItem()) {
          // Edit
          case 0:
            editConfigDialog(&ibox, &mconf, idx);
            break;
          // New
          case 1:
            newConfigDialog(&ibox, &mconf, idx);
            break;
          // Remove
          case 2:
            delConfigDialog(&ibox, &mconf, idx);
            break;
        };
        break;
      case InputResult::Enter:
        // Run
        showDialog = false;
        break;
      default:
        break;
    }

    // next selection will not have timeout
    ibox.setAutoOK(0);
  }

  idx = imax(idx, 0);
  preferences.putInt("dconf", idx);

  // setup selected configuration
  auto conf = mconf.getItem(idx);

  char const * diskFilename[DISKCOUNT];
  downloadOK = true;
  for (int i = 0; i < DISKCOUNT && downloadOK; ++i)
    diskFilename[i] = getDisk(conf->disk[i]);

  if (!downloadOK || (!diskFilename[0] && !diskFilename[2])) {
    // unable to get boot disks
    ibox.message("Error!", "Unable to get system disks!");
    esp_restart();
  }

  if (wifiConnected) {
    // disk downloaded from the Internet, need to reboot to fully disable wifi
    ibox.setAutoOK(2);
    ibox.message("", "Disks downloaded. Restarting...");
    esp_restart();
  }

  ibox.end();

  machine = new Machine;

  machine->setBaseDirectory(SD_MOUNT_PATH);
  for (int i = 0; i < DISKCOUNT; ++i)
    machine->setDriveImage(i, diskFilename[i], conf->cylinders[i], conf->heads[i], conf->sectors[i]);

  machine->setBootDrive(conf->bootDrive);

  /*
  printf("MALLOC_CAP_32BIT : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
  printf("MALLOC_CAP_8BIT  : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  printf("MALLOC_CAP_DMA   : %d bytes (largest %d bytes)\r\n\n", heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

  heap_caps_dump_all();
  */

  machine->setSysReqCallback(sysReqCallback);

  machine->run();

// loop for PCEmulator
while (1)
{
#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
  static uint32_t tcpu = 0, s1 = 0, count = 0;
  tcpu = machine->ticksCounter();
  s_vgapalctrlcycles = 0;
  s1 = fabgl::getCycleCount();
  delay(1000);
  printf("%d\tCPU: %d", count, machine->ticksCounter() - tcpu);
  printf("   Graph: %lld / %d   (%d%%)\n", s_vgapalctrlcycles, fabgl::getCycleCount() - s1, (int)((double)s_vgapalctrlcycles/240000000*100));
  ++count;
#else

  vTaskDelete(NULL);

#endif

  }
}





void ChatterBox()
{
  // the app use another way so:
  //SPIFFS.end();
  
  //Initialise SPIFFS:
    if(!SPIFFS.begin()){
      Serial.println("SPIFFS Mount Failed");
      return;
    }
  // initialize file (not totally sure this is necessary but YOLO)
  File file = SPIFFS.open(filename2, FILE_READ);
  if(!file){
      file.close();
      File file_write = SPIFFS.open(filename2, FILE_WRITE);
      if(!file_write){
          Serial.println("- failed to create file?!?");
      }
      else{
        file_write.print("");
        file_write.close();
      }
  }else{
    file.close();
  }
  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("ChatterBox");
  delay(500); // experienced crashes without this delay!
  tryToConnect();
  //WiFi.persistent(false); // maybe a solution for https://github.com/fenwick67/esp32-chatterbox/issues/1
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  // init http server
  server.on("/", HTTP_GET, sendPage);
  server.on("/index.html", HTTP_GET, sendPage);
  server.on("/posts", HTTP_GET, sendMessages);
  server.on("/post", HTTP_POST, receiveMessage);
  server.begin();
  //Serial.println("Chegou aqui!!");
  // No display:
  // initializes input box
  //InputBox ib;
  //ib.begin(VGA_640x480_60Hz, 500, 400, 4);
  //ib.setBackgroundColor(RGB888(0, 0, 0));
  //ib.onPaint = [&](Canvas * canvas) { drawInfo(canvas); };
  //ib.setAutoOK(0);
  //dnsServer.processNextRequest();
  //server.handleClient();
  //IPAddress IP = WiFi.softAPIP();
  //char capIP[16] = "";
  //strcpy(capIP, String(apIP));
  //char IP2[16] = "";
  //strcpy(IP2, String(IP));
  ibox.message("Status","ChatterBox Started! Click 'Enter'.",WiFi.softAPIP().toString().c_str(),WiFi.localIP().toString().c_str()); 
  //ib.messageFmt("", nullptr," ", "ChatterBox Started!");


  // ChatterBox Loop:
  
  while (1)
  {
  //Serial.println("Chegou aqui");
  dnsServer.processNextRequest();
  server.handleClient();
  }
}




// App JPG View
void JPGView()
{
  FileBrowser::mountSPIFFS(FORMAT_ON_FAIL, SPIFFS_MOUNT_PATH);

  // File Select
  if (SPIFFS_MOUNT_PATH) {
    char filename[16] = "/";
    //String filename;
    char directory[32];
    strcpy(directory, SPIFFS_MOUNT_PATH);
    if (ibox.fileSelector("File Select", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter) {
      Serial.print("directory: ");
      Serial.println(directory);
      Serial.print("filename: ");
      Serial.println(filename);

      // the lib use another way so:
      SPIFFS.end();
      
      ShowJPG(filename);
    }
  }
}



// App WebBrowser, no, download file now.
void DownloadFile() {
  // download file button
  //auto downloadBtn = new uiButton(frame, "Download", Point(260, 50), Size(90, 20));
  //downloadBtn->onClick = [=]() {
  //char URL[128] = "http://";
  // 'https' don't work.
  char URL[228] = "http://d32ogoqmya1dw8.cloudfront.net/files/teaching_computation/workshop_2018/activities/plain_text_version_declaration_inde.txt";
  //char URL[228] = "https://www.w3.org/services/html2txt?url=https%3A%2F%2Fg1.globo.com%2F&noinlinerefs=on&nonums=on";
  auto r = ibox.textInput("Download File", "URL", URL, sizeof(URL), "Cancel", "OK",false);
  switch (r) {
    case InputResult::ButtonExt1:
    Serial.println(URL);
    getDisk(URL);
    break;
    case InputResult::Enter:
    Serial.println(URL);
    char const * filename = getDisk(URL);
    Serial.println(filename);
    //readFile(SD, filename);
    break;
  }
      //updateFreeSpaceLabel();
   
}



void WifiSafe() { // app Wi-Fi Seguro
  Serial.println("App Wi-fi seguro iniciado");
  int senhaDesconhecida = 1;
  byte RedeSelecionada = 0;
  String RedeAlvo = " ";
  //String redes[] { " ", " ", " ", " "};
  //char SSID[32] = "";
  //char psw[32]  = "";
  //while (senhaDesconhecida == 1) {

  // try to start SD
  /* if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8))   // @TODO: reduce to 4?
    ibox.message("Error!", "This app requires a SD-CARD!", nullptr, nullptr); */

    // Encontrar as redes de Wi-Fi
    //WiFi.disconnect(); //disconecta da rede em que está (se está em alguma)
    //scanAndSort2(); //função que scaneia e classifica as redes com senhas
    //delay(WIFI_DELAY);

    //Escolha da rede
    int n = WiFi.scanNetworks(); // escaneia e analiza as redes disponíveis
    Serial.print(n);
    if (n == 0) {
      Serial.println("no networks found");
      ibox.message("Error!", "No networks found!");
    } 
    /* else {
      Serial.print(n);
      for (int j = 0; j < 4; ++j) { // Para cada vaga de rede....
        for (int i = 0; i < n; ++i) { // Para cada rede encontrada....
          if ((WiFi.encryptionType(i) != 7) and (redes[j] != WiFi.SSID(i)) and (redes[j-1] != WiFi.SSID(i)) and (redes[j-2] != WiFi.SSID(i)) and (redes[j-3] != WiFi.SSID(i))) {
            redes[j] = WiFi.SSID(i);
          }
          // WiFi.encryptionType(i) != 7 = sem senha
        }
        Serial.println(String(redes[j]));
      }
    } */
    StringList list;
    for (int i = 0; i < n; ++i)
      list.appendFmt("%s (%d dBm)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    int s = ibox.menu("Choose a network", "Please select a WiFi network", &list);
    //int s = ibox.menu("Choose a network", ".", &redes);
    //if (s == 0) PCEmulator();
    String redes[n];
    RedeAlvo = WiFi.SSID(s).c_str();
    Serial.println("Rede Alvo: " + RedeAlvo);

    // Start the Search:
    if (RedeAlvo != " ") {

      // se senha = nome da rede
      
      //WiFi.begin(RedeAlvo, RedeAlvo);
      //WiFi.begin(char(RedeAlvo), char(RedeAlvo));
      //ibox.message(String(RedeAlvo),String(RedeAlvo));
      //ibox.message(RedeAlvo.c_str(),RedeAlvo.c_str(),nullptr);
      //form->update(i * 100 / 12, "Getting date-time from SNTP...");
      ibox.progressBox("trying to connect", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
        form->update(0, RedeAlvo.c_str()); 
        WiFi.begin(RedeAlvo.c_str(), RedeAlvo.c_str());
      });
      unsigned short try_cnt = 0;
      while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
        delay(500); //WIFI_DELAY
        Serial.print(".");
        try_cnt++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println(RedeAlvo);
        Serial.println("Connection Successful!");
        Serial.println("Your device IP address is ");
        Serial.println(WiFi.localIP());
        //display.drawString(0, 0, "Conectado por WiFi!");
        //linha4 = "Senha Correta!";
        ibox.message("password found", RedeAlvo.c_str(), nullptr, "Ok");
        senhaDesconhecida = 0;
        //break;
        /*if (digitalRead(esquerdo) == HIGH) {
          //Serial.println("esquerdo apertado");
          //delay(170); // espera para cada apertada ser vista individualmente 
          tela = 0; // voltar pelo menu
          break;
        }*/
      }

      // se senha = nome da rede, todas letras minusculas
      String TudoMinusculas = RedeAlvo;
      TudoMinusculas.toLowerCase();
      if ((RedeAlvo != TudoMinusculas) and (senhaDesconhecida == 1)) {
        Serial.println("RedeAlvo != (TudoMinusculas.toLowerCase()");
        ibox.progressBox("trying to connect", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
          form->update(0, TudoMinusculas.c_str()); 
          WiFi.begin(RedeAlvo.c_str(), TudoMinusculas.c_str());
        });
        //ibox.message(RedeAlvo.c_str(),TudoMinusculas.c_str(),nullptr,nullptr);
        try_cnt = 0;
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
          delay(WIFI_DELAY);
          Serial.print(".");
          try_cnt++;
        }
        if(WiFi.status() == WL_CONNECTED) {
          Serial.println("");
          Serial.println(RedeAlvo);
          Serial.println("Connection Successful!");
          Serial.println("Your device IP address is ");
          Serial.println(WiFi.localIP());
          //display.drawString(0, 0, "Conectado por WiFi!");
          ibox.message("password found", TudoMinusculas.c_str(), nullptr, "Ok");
          senhaDesconhecida = 0;
          //break;
          /* if (digitalRead(esquerdo) == HIGH) {
            //Serial.println("esquerdo apertado");
            //delay(170); // espera para cada apertada ser vista individualmente 
            tela = 0; // voltar pelo menu
            
          } */
        }
      }

      // se senha = nome da rede, todas letras minusculas e sem espaços
      String SemEspacos = TudoMinusculas;
      SemEspacos.replace(" ", "");
      Serial.println(String(SemEspacos));
      if ((SemEspacos != TudoMinusculas) and (senhaDesconhecida == 1)) {
        ibox.progressBox("trying to connect: no space", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
          form->update(0, SemEspacos.c_str()); 
          WiFi.begin(RedeAlvo.c_str(), SemEspacos.c_str());
        });
        
        
        //ibox.message(RedeAlvo.c_str(),SemEspacos.c_str(),nullptr,nullptr);
        try_cnt = 0;
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
          delay(WIFI_DELAY);
          Serial.print(".");
          try_cnt++;
        }
        if(WiFi.status() == WL_CONNECTED) {
          Serial.println("");
          Serial.println(RedeAlvo);
          Serial.println("Connection Successful!");
          Serial.println("Your device IP address is ");
          Serial.println(WiFi.localIP());
          //display.drawString(0, 0, "Conectado por WiFi!");
          //linha4 = "Senha Correta!";
          ibox.message("password found", SemEspacos.c_str(), nullptr, "Ok");
          senhaDesconhecida = 0;
          //break;
          /*if (digitalRead(esquerdo) == HIGH) {
            //Serial.println("esquerdo apertado");
            //delay(170); // espera para cada apertada ser vista individualmente 
            tela = 0; // voltar pelo menu
            break;
          } */
        }
      }
      // caso mais raro, ficou de fora
      // se senha = nome da rede + 123
    
    //Pegando uma senha do arquivo
    
    //Initialise SPIFFS:
    if(!SPIFFS.begin()){
      Serial.println("SPIFFS Mount Failed");
      return;
    }  
    File f = SPIFFS.open("/Senhas.dat", "r"); // r = abrir para leitura
    String line = "senha secretaentra sua senha";
    //char psw[32]  = "";
    //while(senhaDesconhecida == 1) {
    while(f.available()) {
    
      WiFi.softAPdisconnect();
      delay(500);
      //delay(1000);
      //File f = SPIFFS.open("/Senhas.dat", "r"); // r = abrir para leitura
      /* if (digitalRead(esquerdo) == HIGH) {
        //Serial.println("esquerdo apertado");
        //delay(170); // espera para cada apertada ser vista individualmente 
        tela = 0; // voltar pelo menu
      } */
      //Serial.println("Fim do loop3");
      line = f.readStringUntil('\n');
      //Serial.println("Fim do loop2");
      //delay(500);
      //psw = f.readStringUntil('\n'); //incompatible types in assignment of 'String' to 'char [32]'
      //psw = (f.readStringUntil('\n')).c_str(); //incompatible types in assignment of 'const char*' to 'char [32]'
      //strcpy(psw, String(f.readStringUntil('\n'))); //cannot convert 'String' to 'const char*' for argument '2' to 'char* strcpy(char*, const char*)'
      //psw = String(f.readStringUntil('\n')); //incompatible types in assignment of 'String' to 'char [32]'
      //psw = getString(f.readStringUntil('\n')); //'getString' was not declared in this scope
      // strcpy(psw, line); //cannot convert 'String' to 'const char*' for argument '2' to 'char* strcpy(char*, const char*)'
      //ibox.message(RedeAlvo.c_str(),line.c_str());
      Serial.println(line);
      ibox.progressBox("trying to connect", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
        form->update(0, line.c_str()); 
        //WiFi.begin(RedeAlvo.c_str(), line.c_str());
        WiFi.begin(RedeAlvo.c_str(), line.c_str());
      });
        //WiFi.begin(RedeAlvo.c_str(), line.c_str());
        //delay(500);
        try_cnt = 0;
        
        /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
          delay(WIFI_DELAY);
          Serial.print(".");
          try_cnt++;
        }
        if(WiFi.status() == WL_CONNECTED) {
          Serial.println("");
          Serial.println("Connection Successful!");
          Serial.println("Your device IP address is ");
          Serial.println(WiFi.localIP());
          ibox.message("password found", line.c_str(), nullptr, "Ok");
          senhaDesconhecida = 0;
          f.close();
          break;
        }
        /* else {
          ibox.message("password not found", nullptr, "Ok");
          //f.close();
          Serial.println("Connection FAILED");
        } */
      //Serial.println("Fim do loop");
      }
    f.close();
    
    }
    // if the next app use another way:
    SPIFFS.end();
}



// App Web Radios
void WebRadios()
{
  Serial.println("WebRadio Player Start! ");

  //Serial.println(ESP.getFreeHeap());

  /*if(psramInit()){
          Serial.println("\nThe PSRAM is correctly initialized");
  }else{
          Serial.println("\nPSRAM does not work");
  } */

  //max_stations= sizeof(stations)/sizeof(stations[0]); log_i("max stations %i", max_stations);

  /* pref.begin("WebRadio", false);  // instance of preferences for defaults (station, volume ...)
  if(pref.getShort("volume", 1000) == 1000){ // if that: pref was never been initialized
      pref.putShort("volume", 7);
      pref.putShort("station", 0);
  }
  else{ // get the stored values
      cur_station = pref.getShort("station");
      cur_volume = pref.getShort("volume");
      //cur_volume = 7;
  } */

  // connect to wifi
  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();

  //heap_caps_dump_all(); reset the system

  /*WiFi.begin("CarmemCurute","19842806");
  while (WiFi.status() != WL_CONNECTED){
      delay(2000);
      Serial.print(".");
  } */
  
  tryToConnect();
  
  // No more display needed
  ibox.end();
  
  //Serial.println("Increasing buffer size. ");
  Audio audio(true, 1, 0);
  audio.setBufsize(15000, 0);

  //ESP.getFreeHeap()
  cur_volume = 6;
  audio.setVolume(cur_volume); // 0...21
  //cur_station = 0;
  
  audio.connecttohost("https://prod-52-91-107-216.wostreaming.net/goodkarma-wknramaac-ibc2"); // ESPN 850 AM news talk sports USA

  //loop
  while (1) {
    audio.loop();
    
    // take a keypress
    //int scode = PS2Controller::keyboard()->getNextScancode();
    //Serial.println(scode);

    if (PS2Controller::keyboard()->scancodeAvailable()) {
      int scode = PS2Controller::keyboard()->getNextScancode();
      if ((scode == 114) and (cur_volume>2)) {
        cur_volume = cur_volume - 2;
        audio.setVolume(cur_volume);
      }
      else if ((scode == 117) and (cur_volume < max_volume)) {
        cur_volume = cur_volume + 2;
        audio.setVolume(cur_volume);
      }
      else if (scode == 22) { // 1
        //audio.connecttospeech("Gaúcha, Brasil.", "pt"); 
        audio.connecttohost("https://1132747t.ha.azioncdn.net/primary/gaucha_rbs.sdp/chunklist_47ea98af-3f33-4a88-9628-a617e3e40466.m3u8"); // News Porto Alegre - RS OK 25/3
      }
      else if (scode == 30) { // 2
        //audio.connecttospeech("92 FM, Brasil.", "pt");
        //audio.connecttomarytts("92 FM, Brasil.", "pt","bits1");
        audio.connecttohost("https://aovivord92fm.rbsdirect.com.br/memorystreams/HLS/92fm/92fm-32000.m3u8"); // pop news talk brazilian  Server:Brazil OK 25/3
      }
      else if (scode == 38) { // 3
        audio.connecttospeech("Business 87.5 FM, Russia.", "en");
        audio.connecttohost("http://bfm.hostingradio.ru:8004/fm?1679613025750"); // Business 87.5 FM Moscow / Russia OK 25/3 (slow in brasil)
      }
      else if (scode == 37) { // 4
        audio.connecttospeech("Radio 10, Argentina.", "en");
        audio.connecttohost("https://radio10.stweb.tv/radio10/live/chunks.m3u8"); // Radio 10 pop news talk latin OK 25/3
      }
      else if (scode == 46) { // 5
        audio.connecttohost("https://streaming.fabrik.fm/voc/echocast/audio/low/index.m3u8"); // Voice of the Cape news talk South Africa Server:South Africa OK 25/3
      }
      else if (scode == 54) { // 6
        audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3"); //  0 N - 70s on Radio Deutsch Server:Germany OK 25/3
      }
      else if (scode == 61) { // 7
        audio.connecttohost("https://stream2.cnmns.net/paradisefm"); //Paradise FM pop news talk oldies Server:Australia OK 25/3
      }
      else if (scode == 62) { // 8
        //audio.connecttohost("http://61.89.201.27:8000/radikishi.mp3"); // Radio Kishiwada Japan pop news talk classic ethnic Server:Japan failed!
        audio.connecttohost("https://stream-icy.bauermedia.pt/comercial.aac"); // Radio Comercial Portugal pop news folk Server:UK OK 25/3
      }
      else if (scode == 70) { // 9
        audio.connecttohost("https://rhema-radio.streamguys1.com/rhema-star.mp3"); // Star FM talk christian  Server:NZ OK 25/3 (slow in brasil)
      }
      else if (scode == 69) { // 0
        audio.connecttohost("http://10.42.0.1/audio.m3u"); // open a file call 'audio.m3u in a Linux computer router wifi and webserver started. (opening an access point in Linux)
      }
      //else if (scode == 69) { // 0
      //  audio.connecttohost("http://10.42.0.1/audio.m3u"); // open a file call 'audio.m3u in a Linux computer router wifi and webserver started. 
      //}
      /* else if (scode == 116){  // the list of urls spend much memory
        if (cur_station<max_stations-1) cur_station++;
        else cur_station = 0;
        audio.connecttohost(stations[cur_station].c_str());
      }
      else if (scode == 107){
        if (cur_station>0) cur_station--;
        else cur_station = max_stations-1;
        audio.connecttohost(stations[cur_station].c_str());
      } */
      else if (scode == 102) {  // get out backspace
        esp_restart();
      }
                                
    //xprintf("%02X ", scode);
    //if (scode == 0xF0 || scode == 0xE0) ++clen;
    //--clen;
    //if (clen == 0) {
    //  clen = 1;
    //  xprintf("\r\n");
    //}
    Serial.println(scode);
  }

    
  }

}



void AudioPlayer(){
  Serial.println("Audio Player started");

  /* Serial.print( "fAlarm " );
  Serial.print(uxTaskGetStackHighWaterMark( NULL ));
  Serial.println();
  Serial.flush(); */

  //ibox.textInput("Audio Player", "Path", path, sizeof(path), "Cancel", "OK",false);

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
    //ibox.message("", "microSD-CARD not found!", nullptr, nullptr); 
    hasSD = false;
    Serial.println("SD not Found. ");
  }
  else {
    hasSD = true;
    Serial.println("SD Found. ");
  }

  char filename[50] = "/";
  //String filename;
  char directory[50];
  strcpy(directory, "/");
  if (ibox.fileSelector("Audio Player", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter) {
  //if (ibox.fileSelector("File Select", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename)) == InputResult::Enter) {
    //directory += filename;   - invalid use of non-lvalue array
    Serial.print("directory: ");
    Serial.println(directory);
    //filename = "/" + filename;  //invalid use of non-lvalue array
    //'/' += filename; invalid use of non-lvalue array
    //filename[16] = "/" + filename; invalid operands of types 'const char [2]' and 'char [16]' to binary 'operator+'
    //filename[16] = "/" += filename; invalid use of non-lvalue array
    Serial.print("filename: ");
    Serial.println(filename);
  }
    //String filen;
    //String diren;
    //String filen = directory + filename; //invalid operands of types 'char [32]' and 'char [50]' to binary 'operator+'
    //strcpy(diren,directory);
    //strcpy(filen,filename); //cannot convert 'String' to 'char*' for argument '1' to 'char* strcpy(char*, const char*)'
    //filen = diren + filen;
    //directory =+ filename; incompatible types in assignment of 'char*' to 'char [32]'
    //String diren = toString(directory); //'toString' was not declared in this scope
    //String filen = toString(filename);
    //filen = filen + diren;
    char filen[strlen(directory)+strlen(filename)+1] = "";
    strcat(filen,directory);
    Serial.print("filen: ");
    Serial.println(filen);
    strcat(filen,"/");
    strcat(filen,filename);
    Serial.print("filen: ");
    Serial.println(filen);
    //strcpy (filen,filen[5].c_str());
    //filen.erase (0,5); //request for member 'erase' in 'filen', which is of non-class type 'char

    String filen2;
    filen2 += filen;
    Serial.print("filen2: ");
    Serial.println(filen2);
    //filen2.erase(0,5); //'class String' has no member named 'erase'
    //std::string filen2 = filen2.substr(5); //conflicting declaration 'std::__cxx11::string filen2'
    filen2.remove(0, 3);  
    Serial.print("filen2: ");
    Serial.println(filen2);
    
  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 

  // No more display needed
  ibox.end();

  SPIFFS.end();

  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  // try to turn off the wifi
  WiFi.mode(WIFI_OFF);

/*
 * Optional SD Card connections:
 *   MISO => GPIO 16  (2 for PICO-D4)
 *   MOSI => GPIO 17  (12 for PICO-D4)
 *   CLK  => GPIO 14
 *   CS   => GPIO 13 */
  //SD(SPI)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  //Serial.println("pin3");
  SPI.setFrequency(1000000);
  //Serial.println("pin4");
  if (!SD.begin(SD_CS, SPI)) {
      Serial.println("Card Mount Failed");
      //break;
  }
  else Serial.println("Card Mount Ok!");  

  // get the path

  /* auto r = ibox.textInput("MP3 Player", "Path", path, sizeof(path), "Cancel", "OK",false);
  switch (r) {
    case InputResult::ButtonExt1:
      Serial.println(path);
      //getDisk(URL);
      //break;
    case InputResult::Enter:
      Serial.println(path);
      //char const * filename = getDisk(URL);
      //Serial.println(filename);
      //readFile(SD, filename);
      //break;
  } */


  
  //Read SD
  //file_num = get_music_list(SD, path, 0, file_list);
  //Serial.print("Music file count:");
  //Serial.println(file_num);
  //Serial.println("All music:");
  //for (int i = 0; i < file_num; i++)
  //{
  //    Serial.println(file_list[i]);
  //}

  char filen3[filen2.length() + 1];
  //filen3 = filen2.c_str(); //incompatible types in assignment of 'const char*' to 'char [50]'
  strcpy(filen3,filen2.c_str()); 
  PlayAudio(filen3);

  
} // End of app MP3 Player




void setup()
{
  Serial.begin(115200); delay(500); printf("\n\n\nStart\n\n");// DEBUG ONLY

  
  Serial.println(ESP.getFreeHeap());
  //heap_caps_get_free_size();

  disableCore0WDT();
  delay(100); // experienced crashes without this delay!
  disableCore1WDT();

  preferences.begin("PCEmulator", false);

  // uncomment to clear preferences
  //preferences.clear();
  
  // save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);
  ibox.setBackgroundColor(RGB888(0, 0, 0));

  //ibox.onPaint = [&](Canvas * canvas) { drawInfo(canvas); };

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
    //ibox.message("", "microSD-CARD not found!", nullptr, nullptr); 
    hasSD = false;
    Serial.println("SD not Found. ");
  }
  else {
    hasSD = true;
    Serial.println("SD Found. ");
  }

  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 

  // uncomment to format SD!
  //FileBrowser::format(fabgl::DriveType::SDCard, 0);

  esp_register_shutdown_handler(shutdownHandler);

  //updateDateTime();

  //heap_caps_get_free_size();

  // Monitor or Audio?
  fabgl::StringList list;
  list.append("Using Audio");
  list.append("Using Monitor");
  list.select(0, true);
  int s = ibox.menu("Applications", "choose the mode", &list);
  ibox.setAutoOK(5);
  if (s == 0) {
    audioMode = true;
    Serial.println("audioMode ativado");
  }
  else if (s == 1) {
    audioMode = false;
    Serial.println("audioMode não ativo");
  }

}



#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
namespace fabgl {
  extern volatile uint64_t s_vgapalctrlcycles;
}
using fabgl::s_vgapalctrlcycles;
#endif



void loop()
{

      
  // Apps Menu

  if (audioMode == false) {
    fabgl::StringList list;
    list.append("PCEmulator"); //0
    list.append("ChatterBox");
    list.append("File Browser");
    list.append("JPG View");
    list.append("Web Radios");
    list.append("Audio Player");
    list.append("Wifi Safe");

    list.select(0, true);
    //int s = ibox.menu("Applications list", "Click on an item", &list);
    int s = ibox.menu("Applications", "choose the app", &list);
    if (s == 0) {
      //if (hasSD == false) ibox.message("Error!", "microSD-CARD not found!", nullptr, nullptr); 
      PCEmulator();
    }
    if (s == 1) {
      //Initialise SPIFFS:
      if(!SPIFFS.begin()){
        Serial.println("SPIFFS Mount Failed");
        return;
      }  
      ChatterBox();
    }
    if (s == 2) {
      //rootWindow()->frameStyle().backgroundColor = RGB888(0, 0, 64);
      //auto frame = new uiFrame(rootWindow(), "FileBrowser Example", Point(15, 10), Size(375, 275));
      //frame->frameProps().hasCloseButton = false;
      //fb = new uiFileBrowser(frame, Point(10, 25), Size(140, 180));
      //fb->setDirectory("/");
      if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
      //ibox.message("", "microSD-CARD not found!", nullptr, nullptr); 
      hasSD = false;
        Serial.println("SD not Found. ");
      }
      else {
        hasSD = true;
        Serial.println("SD Found. ");
      }
      FileBrowser::mountSPIFFS(FORMAT_ON_FAIL, SPIFFS_MOUNT_PATH);
      ibox.folderBrowser("Browse Files", "/");
      //ibox.folderBrowser("Browse Files", SPIFFS_MOUNT_PATH);
      //ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
    }
    if (s == 3) JPGView(); // updateDateTime();
    if (s == 4) {
      WebRadios();
    }
    if (s == 5) {
      //News();
      AudioPlayer();
    }
    if (s == 6) WifiSafe();
    //if (s == 7) 
    //if (s == 8) 
    //if (s == 9) 
  }
  
  else if (audioMode == true) { // Audio mode on
    
    // take a keypress
    Serial.println("take a keypress");
    int scode = PS2Controller::keyboard()->getNextScancode();
    Serial.println(scode);
    
    if (scode == 22) { // 1 WebRádios
      WebRadios();
    }
    else if (scode == 30) { // 2 Chat server
      ChatterBox();
    }
    else if (scode == 38) { // 3 MP3 Player
      //if (hasSD == true) { // Tem cartão SD
        AudioPlayer();
      //}
      //else {
      //  Serial.println("SD Mount Failed");
      //}
    }

     /* audio.connecttoFS(SD, "/press.mp3");
      audio.connecttoFS(SD, "/one.mp3");
      audio.connecttoFS(SD, "/for.mp3");
      audio.connecttoFS(SD, "/internet.mp3");
      audio.connecttoFS(SD, "/radio.mp3"); */
    //}
    /* File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      String dir = file.name();
      if (dir == "/VGA32Audio") {
        Serial.println("/VGA32Audio exists.");

      audio.connecttoFS(SD, "/welcome.mp3");
      
      audio.connecttoFS(SD, "/press.mp3");
      audio.connecttoFS(SD, "/one.mp3");
      audio.connecttoFS(SD, "/for.mp3");
      audio.connecttoFS(SD, "/internet.mp3");
      audio.connecttoFS(SD, "/radio.mp3");
      break;
      /*  File file2 = file.openNextFile();
        while(file2){
          String filename = file2.name();
          if (filename.endsWith(".mp3")) {
            Serial.print("tocar:");
            Serial.print(filename);
            audio.connecttoFS(SD,file2.name());
          }
          file2 = file.openNextFile();
        } 
      }
    }
  file = root.openNextFile();
  } */
      
      
     /* audio.connecttoFS(SD, "/audio.mp3");
      audio.connecttoFS(SD, "/welcome.mp3");
      
      

      audio.connecttoFS(SD, "/VGA32Audio/press.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/two.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/for.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/chat.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/server.mp3");

      audio.connecttoFS(SD, "/VGA32Audio/press.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/three.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/for.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/mp3.mp3");
      audio.connecttoFS(SD, "/VGA32Audio/player.mp3"); */
      
    }
    
    /* // take a keypress
    int scode = PS2Controller::keyboard()->getNextScancode();
    Serial.println(scode);
    if (scode == 22) { // 1
      // WebRadios
      WebRadios();
    }
    else if (scode == 30) { // 2
      //ChatterBox bate papo horizontal
      ChatterBox();
    } */

    /* else if (scode == 38) { // 3 MP3 Player

    }
    else if (scode == 37) { // 4

    }
    else if (scode == 46) { // 5

    }
    else if (scode == 54) { // 6

    }
    else if (scode == 61) { // 7

    }
    else if (scode == 62) { // 8
      //

    }
    else if (scode == 70) { // 9

    }
    else if (scode == 69) { // 0

    } */
  delay(500);
}

//O sketch usa 1776202 bytes (56%) de espaço de armazenamento para programas. O máximo são 3145728 bytes.
//Variáveis globais usam 57464 bytes (17%) de memória dinâmica, deixando 270216 bytes para variáveis locais. O máximo são 327680 bytes.
