 /*
    PC Emulator Plus Created by Adrian Rupp, 2022-2024.

  PCEmulator Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
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

SoundGenerator     m_soundGen;
//SoundGenerator           m_soundGen(16000, GPIO_NUM_25, SoundGenMethod::SigmaDelta);
//SoundGenerator           m_soundGen(16000, GPIO_NUM_25, SoundGenMethod::DAC);

//#define SD_MOUNT_PATH "/SD"

#define SPIFFS_MOUNT_PATH  "/flash"
#define FORMAT_ON_FAIL     true

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
//char path[30] = "/";
String path = "/";

#include <SPI.h>

//SD Card
//#define SD_CS 22
#define SD_CS 13
#define SPI_MOSI 12
#define SPI_MISO 2
#define SPI_SCK 14

// for the SD use
#include "SD.h"
File root;

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
//fabgl::VGA16Controller VGAController;
//fabgl::VGA2Controller VGAController;
fabgl::VGAController VGAController;
fabgl::Canvas cv(&VGAController);

// For App Web Radios
#include <Arduino.h>
#include "Audio.h"
//#include <Preferences.h> 
//#include <WiFi.h>
long randNumber; // any RND number

  uint8_t max_volume   = 21;
  uint8_t cur_volume   = 10;  



void audio_eof_mp3(const char *info) { //end of file
//    Serial.print("eof_mp3     ");
//    Serial.println(info);
    //esp_restart();
  playNextFile(); // play next file in directory if playing a multi-track episode
      //audio.stopSong();
    
    //File file = root.openNextFile();
    //Serial.println("oi");
    //String temp = file.name();
    //String tempr = root.name();
    //Serial.println("root");
    //Serial.println(tempr);
    //Serial.println("temp");
    //Serial.println(temp);
    //PlayAudio(temp.c_str());
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

  
  //Audio audio(true, 1, 0);
  //Audio audio;
  Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN); // Is it also possible without an external DAC?If the 8-bit sound is enough, you can do that.
  
  //Serial.println("Increasing buffer size. ");
  //audio.setBufsize(15000, 0);
  //audio.setBufsize(1000, 0); A stack overflow in task IDLE0 has been detected.
  //audio.setBufsize(2000, 0); Não funcionou
  //audio.setBufsize(50000, 0); //Não funcionou 
  //audio.setBufsize(5000, 0); //inputBufferSize: 3399 bytes Não funcionou 

  
    //Serial.printf("Listing directory: %s\n", dirname);
    //int i = 0;
 
    //File root = fs.open(path);

  //audio.forceMono(true); // necessary to have enough memory to play
  
  audio.setVolume(9); // 0...21
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
    //heap_caps_check_integrity_all()
    audio.loop();
    if (audio.isRunning()){
      //Serial.println("audio.isRunning");
      audio.loop();
    }
    else {
      audio.stopSong();
    }

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




// For Audio Player:
void playNextFile() {
  //String path = root.name();
  //String lastfile = path.substring(path.lastIndexOf('/'));
  //String path2 = path.substring(0,path.lastIndexOf('/'));
  Serial.println("path");
  Serial.println(path);
  Serial.println("root");
  Serial.println(root.name());
  //File root1 = SD.open(path);
  File file = root.openNextFile();
  //String filename = file.name();
  //Serial.println("file");
  //Serial.println(filename);
  while (file) {
    String filename = file.name();
    if (filename.endsWith(".mp3") or filename.endsWith(".wav") or filename.endsWith(".flac") or filename.endsWith(".wma")) {
      //Serial.println("achei");
      randNumber = random(4);
      if (randNumber < 1) {
        if (path == filename) {
          Serial.println("igual");
          file = root.openNextFile();
          filename = file.name();
          //root = file;
          PlayAudio(filename.c_str());
          //file.close(); 
        }
        else {
          //Serial.println("diferente");
          //Serial.println(path);
          //root = file;
          PlayAudio(filename.c_str());
          //file.close();
        }
      }
    }
    file.close(); 
    file = root.openNextFile();
  }

}



bool ConnectOpen(){
  // Scan, Sort, and Connect to WiFi   thanks to Leatherneck Garage
  // Link: https://www.instructables.com/id/Connect-an-ESP8266-to-Any-Open-WiFi-Network-Automa/
  Serial.println("Scanning for open networks...");
  if(WiFi.status() != WL_CONNECTED) {
    
    /* Clear previous modes. */
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    //WiFi.mode(WIFI_AP_STA); // tentando ativar modo AP + cliente
    delay(500);
    //WiFi.softAP("ESP8266 2342"); // tentando criar o AP
    /* Scan for networks to find open guy. */

    char ssid[32] = ""; /* SSID that to be stored to connect. */

    memset(ssid, 0, 32);
    int n = WiFi.scanNetworks();
    Serial.println("Scan complete!");
    if (n == 0) {
      Serial.println("No networks available.");
    } 
    else {
      Serial.print(n);
      Serial.println(" networks discovered.");
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }
      for (int i = 0; i < n; ++i) {
        Serial.println("The strongest open network is:");
        Serial.print(WiFi.SSID(indices[i]));
        Serial.print(" ");
        Serial.print(WiFi.RSSI(indices[i]));
        Serial.print(" ");
        Serial.print(WiFi.encryptionType(indices[i]));
        Serial.println();
        //if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) { // 'ENC_TYPE_NONE' another lib
        if(WiFi.encryptionType(indices[i]) == WIFI_AUTH_OPEN) {  
          memset(ssid, 0, 32);
          strncpy(ssid, WiFi.SSID(indices[i]).c_str(), 32);
          break;
        }
      }
  }

    delay(500);
    /* Global ssid param need to be filled to connect. */
    if(strlen(ssid) > 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      /* No pass for WiFi. We are looking for non-encrypteds. */
      WiFi.begin(ssid);
      unsigned short try_cnt = 0;
      /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
      while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / 500) {
        delay(500);
        Serial.print(".");
        try_cnt++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Connection Successful!");
        Serial.println("Your device IP address is ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("Connection FAILED");
      }
    } 
    else {
      Serial.println("No open networks available. :-(");  
    }
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
        ConnectOpen(); // connect to a strong open network
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
  //wifiConnected = tryToConnect();
  //if (!wifiConnected) {

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

  //}

  //return wifiConnected;
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
    ibox.progressBox("ChatterBox last message:", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
      form->update(0, line.c_str()); });
      /* ibox.setAutoOK(0);
      if (ibox.textInput("ChatterBox", "message:", message, 99, nullptr, "OK",false) == InputResult::Enter) {
        File file = SPIFFS.open(filename2, FILE_APPEND);
        file.print(message);
        file.print(RECORD_SEP);  
        file.close();
        Serial.println("Message from TTGO:");
        Serial.println(message); */
  //  Serial.print("New message: ");
  //  Serial.println(line);
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
  //if ( y >= 300 ) return 0;
  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      writePixel(x + i, y, bitmap[j * w + i]);
    }
  }

  // Return 1 to decode next block
  return 1;
}



// show image jpg
// code based on https://github.com/fdivitto/FabGL/discussions/160#discussioncomment-1423074
void ShowJPG(char * directory, char * filename)
{

  //Initialise SPIFFS:
  //if(!SPIFFS.begin()){
  //  Serial.println("SPIFFS Mount Failed");
  //  return;
  //}  
  //Serial.print("filename: ");
  //Serial.println(filename);
  //Serial.print("directory: ");
  //Serial.println(directory);
  fabgl::BitmappedDisplayController::queueSize = 1024; // deixou mais rápido, com mais 10 = extremamente lento (e não resolveu d mostrar tudo)
  //VGAController.queueSize = 2048; // 400x300 = 11740 ms
  //VGAController.queueSize = 256; // 400x300 = 11890 ms
  //cv.waitCompletion(false);

  //cv.endUpdate();
  //cv.reset(); // pane com o de baixo
  //cv.resetPaintOptions(); // pane

  /* bool resol; // true = 400x300 64c
  fabgl::StringList list;
  list.append("Mode 400x300 64colors");
  list.append("Mode 1280x720 64colors");
  list.select(0, true);
  int s = ibox.menu("Jpg View", "Choose the mode", &list);
  ibox.setAutoOK(5);
  if (s == 0) {
    resol = true;
  }
  else if (s == 1) {
    resol = false;
  } */


  ibox.end();

  // VGAController.setResolution(VGA_640x480_60Hz); // miss end of image
  //if (resol == true) {
    VGAController.begin();
    VGAController.setResolution(VGA_400x300_60Hz); // nice esticado
    //VGAController.setResolution("\"512x384@60Hz\" 32.5 512 525 593 672 384 385 388 403 -HSync -VSync DoubleScan VisibleBegins"); / estica e corta
  //}
  
  /* if (resol == false) {
    Serial.println("HelloAG");
    //fabgl::VGA2Controller VGAController;
    Serial.println("HelloER");
    VGAController.begin();
    Serial.println("Hello2");
    //fabgl::VGA2Controller VGAController;
    //fabgl::Canvas cv(&VGAController);
    Serial.println("Hello25");
    //VGAController.setResolution(SVGA_1280x720_60HzAlt1); //  error x2
    VGAController.setResolution(VGA_320x200_70Hz); 
    //VGAController.setResolution(SVGA_1024x768_60Hz); //  error
    // Modeline corrigido "400x300@60Hz" 20 400 424 488 536 300 302 304 316 -HSync -VSync DoubleScan VisibleBegins
    // menor modeline "320x200@70Hz" 12.5875 320 332 380 408 200 205 206 224 -HSync -VSync DoubleScan VisibleBegins
    Serial.println("Hello12");
    //cv.endUpdate();
    //cv.reset();
  } */
  
  //VGAController.setResolution(VGA_640x240_60Hz); // miss end of image
  //VGAController.setResolution(VGA_640x480_60Hz, 512, 300);
  //VGAController.setResolution(VGA_512x300_60Hz);
  //VGAController.setResolution(VGA_400x300_60Hz);
  //VGAController.setResolution(VGA_512x384_60Hz, 512, 300); // miss end of image, esticada
  //VGAController.setResolution(VGA_512x384_60Hz, 480, 270); // miss end of image, esticada
  //VGAController.setResolution(VGA_512x384_60Hz, 470, 300); // my monitor resolution is 1024 x 600 // miss end of image , pouco esticada
  //VGAController.setResolution("\"256x150@60.00\" 2.40 256 232 256 256 150 151 154 156 -HSync -Vsync"); // no image, error in modeline?
  //VGAController.setResolution("\"320x188_60.00\" 3.93 320 304 328 336 188 189 192 195 -HSync -Vsync"); // no image, error in modeline?
  //VGAController.setResolution("\"400x234_60.00\" 6.53 400 392 424 448 234 235 238 243 -HSync -Vsync"); // no suport in monitor, error in modeline?
  //VGAController.setResolution("\"512x300@60.00\" 11.05 512 504 552 592 300 301 304 311 -HSync -Vsync"); // no suport in monitor, error in modeline?

  //VGAController.release();
  //VGAController.begin();
  //VGAController.setResolution(VGA_400x300_60Hz); // nice
  
  //VGAController.setProcessPrimitivesOnBlank(true);
  //cv.clear();
  //Serial.println("Hello56");
  cv.setBrushColor(RGB888(0, 0, 0));
  //Serial.println("Hello78");
  cv.clear();
  //Serial.println("HelloGJ");
  //Serial.print("ho");
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  //TJpgDec.setJpgScale(1);
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

  //fabgl::NativePixelFormat(Mono);
  
  TJpgDec.getSdJpgSize(&w, &h, String("/")+filename);
  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

  int factor = 1;

  //if (resol == true) {
    //Resize if possible: 
    //if (w > 1600 or h > 1200) 
      //TJpgDec.setJpgScale(8);
    if (w > 800 or h > 600) {
      TJpgDec.setJpgScale(4);
      factor = 4;
    }
    else if (w > 400 or h > 300) {
      TJpgDec.setJpgScale(2);
      factor = 2;
    }
    else {
      TJpgDec.setJpgScale(1); 
      factor = 1;
    }

  // Draw the image, top left at 0,0
  TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
  
  // How much time did rendering take
  t = millis() - t;
  Serial.print(t); Serial.println(" ms");

  bool modo = true; // true = key next

  // take a keypress
  int scode = PS2Controller::keyboard()->getNextScancode();
  Serial.print(scode);
  if (scode == 102) {  // get out backspace
     esp_restart();
  }
  else if (scode == 27) { //  s
     modo = false; // false = slide show mode
  }
  else if (scode == 45) { //  r reduce image
    if (factor == 1) {
      TJpgDec.setJpgScale(2); 
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
    else if (factor == 2) {
      TJpgDec.setJpgScale(4); 
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
    else if (factor == 4) {
      TJpgDec.setJpgScale(8); 
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
  }
  else if (scode == 36) { //  e enlarge image
    if (factor == 4) {
      TJpgDec.setJpgScale(2); 
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
    else if (factor == 2) {
      TJpgDec.setJpgScale(1); 
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
    else if (factor == 1) {
      VGAController.setResolution(VGA_320x200_70Hz);
      TJpgDec.drawSdJpg(0, 0, String("/")+filename); 
    }
    scode = PS2Controller::keyboard()->getNextScancode();
    VGAController.setResolution(VGA_400x300_60Hz);
  }
  String directory2;
  directory2 += directory;
  directory2.remove(0, 3); // 0,3 "/sd" virou nada  0, 1 sd
  //strcat(directory2,"/");
  //strcat(directory2,directory[directory.length()+1];
  Serial.print("directory2: ");
  Serial.println(directory2);
  //directory2 = "/";
  Serial.print("directory2: ");
  Serial.println(directory2);
  File root = SD.open(directory2);
  Serial.print(root);
  while(1) {

      //Serial.print("cgei");
      File file = root.openNextFile();
      Serial.println(file);
      while (file) {
        if (file.isDirectory()) { }
        else {
          String temp = file.name();
          Serial.println(temp);
          if (temp.endsWith(".jpg") or temp.endsWith(".jpeg") or temp.endsWith(".jpe") or temp.endsWith(".jif") or temp.endsWith(".jfif") or temp.endsWith(".jfi")) {
            //if (resol == true) {
              //Resize if possible: (down scale...)
              TJpgDec.getSdJpgSize(&w, &h, String("/")+temp);
              Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
              //Resize if possible: 
              //if (w > 1600 or h > 1200) 
              //  TJpgDec.setJpgScale(8);
              if (w > 800 or h > 600) 
                TJpgDec.setJpgScale(4);
              else if (w > 400 or h > 300) 
                TJpgDec.setJpgScale(2);
              else 
                TJpgDec.setJpgScale(1); 
            //} 
            //cv.clear();
            TJpgDec.drawSdJpg(0, 0, String("/")+temp);
            if (modo == false) 
              delay(10000); // 10s
              //delay(5000); // 5s
            else if (modo == true) {
              int scode = PS2Controller::keyboard()->getNextScancode();
              Serial.print(scode);
              if (scode == 102) {  // get out backspace
                esp_restart();
              }
              else if (scode == 27) { //  s 240?
              modo = false; // false = slide show mode
              }
              else if (scode == 45) { //  r reduce
                if (factor == 1) {
                  TJpgDec.setJpgScale(2); 
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
                else if (factor == 2) {
                  TJpgDec.setJpgScale(4); 
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
                else if (factor == 4) {
                  TJpgDec.setJpgScale(8); 
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
              scode = PS2Controller::keyboard()->getNextScancode();
              }
              else if (scode == 36) { //  e enlarge image
                if (factor == 4) {
                  TJpgDec.setJpgScale(2); 
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
                else if (factor == 2) {
                  TJpgDec.setJpgScale(1); 
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
                else if (factor == 1) {
                  VGAController.setResolution(VGA_320x200_70Hz);
                  TJpgDec.drawSdJpg(0, 0, String("/")+temp);
                }
                scode = PS2Controller::keyboard()->getNextScancode();
                VGAController.setResolution(VGA_400x300_60Hz);
              }
            }
          }
        }
      file = root.openNextFile();
      }
      file.close();

  Serial.print("Hi");
  break;
  } 
  Serial.print("Hi2");
  // restart the app:
  //FileBrowser::unmountSDCard();   //Unmound SD
  //SD.end();
  //Serial.println("Card unMount!"); 
  //ibox.begin(VGA_640x480_60Hz, 500, 400, 4);
  //ibox.setBackgroundColor(RGB888(0, 0, 0));
  //JPGView(); 
  esp_restart();
} // end of ShowJPG



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

  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 
  
  //Initialise SPIFFS:
    if(!SPIFFS.begin()){
      Serial.println("SPIFFS Mount Failed");
      return;
    }
  SPIFFS.remove(filename2);  
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
    char message[100]; // for message from the TTGO VGA32
  while (1)
  {
  //Serial.println("Chegou aqui");
  dnsServer.processNextRequest();
  server.handleClient();
  }
} // End of app chatter Box




// App JPG View
void JPGView() {
  Serial.println("App JPG View started.");

  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  // try to turn off the wifi
  WiFi.mode(WIFI_OFF);

  //dnsServer.end();
  //server.end();
  SPIFFS.end();

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
    hasSD = false;
    Serial.println("SD not Found. ");
  }
  else {
    hasSD = true;
    Serial.println("SD Found. ");
  }

  // File Select
  //if (SPIFFS_MOUNT_PATH) {

  // origin
  //ibox.setupButton(0, "From Network");
  //ibox.setupButton(1, "From SD");
  //auto r = ibox.select("JPG View", "Please select a location", nullptr, nullptr, "Run");
  //fabgl::StringList list;
  //list.append("From Network"); //0
  //list.append("From SD");
  //int s = ibox.menu("JPG View", "Please select a location", &list);
  //auto r = ibox.message("JPG View", "Please select a location", nullptr, "From Network", "From SD");
  
  /* if (s == 0) {
      //case InputResult::Cancel:{ 
        // network
        String lista[20];
        int i = 0;
        File f = SD.open("/webimages.txt", "r"); // r = abrir para leitura
        while(f.available()) {
          //String line = f.readStringUntil('\n');
          //fabgl::StringList list;
          //list.append(f.readStringUntil('\n'));
          //list.appendFmt((f.readStringUntil('\n')).c_str());
          //list.appendFmt(line.c_str());
          if (i < 20) lista[i] = f.readStringUntil('\n');
          i = i + 1;
        }
        f.close();
        fabgl::StringList list;
        String address;
        for (int i = 0; i < 20; ++i)
          list.appendFmt(lista[i].c_str());
        int s = ibox.menu("JPG View", "Choose the site:", &list);
        //String address = lista[s].remove(lista[s].readStringUntil(';');;
        //address.remove(lista[s].readStringUntil(';');
        address.remove(0, address.lastIndexOf(';'));
        Serial.println(address);
        download(address.c_str());
        //break;
     // } 
      //case InputResult::Enter:{ 
        //segue
      //} */

  char filename[40] = "/";
  //String filename;
  char directory[40];
  //strcpy(directory, SPIFFS_MOUNT_PATH);
  strcpy(directory, "/");
  if (ibox.fileSelector("JPG View", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter) {}
  Serial.print("directory: ");
  Serial.println(directory);
  Serial.print("filename: ");
  Serial.println(filename);
  char filen[strlen(directory)+strlen(filename)+1] = "";
  strcat(filen,directory);
  Serial.print("filen: ");
  Serial.println(filen);
  strcat(filen,"/");
  strcat(filen,filename);
  //Serial.print("filen: ");
  //Serial.println(filen);
  String filen2;
  filen2 += filen;
  //Serial.print("filen2: ");
  //Serial.println(filen2);
  filen2.remove(0, 3);  
  Serial.print("filen2: ");
  Serial.println(filen2);
  char filen3[filen2.length() + 1];
  //filen3 = filen2.c_str(); //incompatible types in assignment of 'const char*' to 'char [50]'
  strcpy(filen3,filen2.c_str()); 

  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 
    
  //SD(SPI)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);
  if (!SD.begin(SD_CS, SPI)) {
      Serial.println("Card Mount Failed");
      //break;
  }
  else Serial.println("Card Mount Ok!");  

  ShowJPG(directory,filen3);
  
}



// App WebBrowser, no, download file now.
/* void DownloadFile() {
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
   
} */



void WifiSafe() { // app Wi-Fi Seguro
  Serial.println("App Wi-fi seguro iniciado");
  int senhaDesconhecida = 1;
  byte RedeSelecionada = 0;
  String RedeAlvo = " ";

    //Escolha da rede
    int n = WiFi.scanNetworks(); // escaneia e analiza as redes disponíveis
    Serial.print(n);
    if (n == 0) {
      Serial.println("no networks found");
      ibox.message("Error!", "No networks found!");
    } 

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

      ibox.progressBox("trying to connect", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
        form->update(0, RedeAlvo.c_str()); 
        WiFi.begin(RedeAlvo.c_str(), RedeAlvo.c_str());
      });
      unsigned short try_cnt = 0;
      while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / 500) {
        delay(500); //500
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
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / 500) {
          delay(500);
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
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / 500) {
          delay(500);
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
    
    while(f.available()) {
    
      WiFi.softAPdisconnect();
      delay(500);
      line = f.readStringUntil('\n');
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
        while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / 500) {
          delay(500);
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
} // app Wi-Fi Safe end



// App Web Radios
void WebRadios()
{
  Serial.println("WebRadio Player Start! ");

  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();

  //heap_caps_dump_all(); reset the system
  
  // connect to wifi
  tryToConnect();
  
  // No more display needed
  ibox.end();

  Audio audio(true, 1, 0);
  audio.setBufsize(15000, 0);

  cur_volume = 6;
  audio.setVolume(cur_volume); // 0...21
  //cur_station = 0;
  int language = 0; // 0 = no language, 1 = portugues
  
  audio.connecttohost("https://prod-52-91-107-216.wostreaming.net/goodkarma-wknramaac-ibc2"); // ESPN 850 AM news talk sports USA OK!
  
  //loop
  while (1) {
    audio.loop();

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
        if (language == 0) // many languages
        audio.connecttohost("https://jenny.torontocast.com:2000/stream/j1gold/stream?1683063865715"); // Radio J1 Gold Tóquio / Japão Server: Canada OK!
        // "https://jenny.torontocast.com:2000/stream/j1hits/stream?1683063993885"); // Radio J1 Hits Tóquio / Japão
        else if (language == 1) // Portugues
          audio.connecttohost("https://1132747t.ha.azioncdn.net/primary/gaucha_rbs.sdp/chunklist_47ea98af-3f33-4a88-9628-a617e3e40466.m3u8"); // Rádio Gaúcha Brasil Server:Brazil OK!
        else if (language == 2) // LAN
          audio.connecttohost("http://10.42.0.1/audio.m3u"); // open a file call 'audio.m3u in a Linux computer router wifi and webserver started.
      }
      else if (scode == 30) { // 2
        if (language == 0) 
          audio.connecttohost("https://zas7.ndx.co.za:8004/stream?1683061591597"); // Alex 89.1 FM Johanesburgo / África do Sul English Server:German OK!
        else if (language == 1) 
          audio.connecttohost("https://aovivord92fm.rbsdirect.com.br/memorystreams/HLS/92fm/92fm-32000.m3u8"); //92 FM pop news talk brazilian  Server:Brazil OK!
        else if (language == 2) 
          audio.connecttohost("http://192.168.43.1:8080/audio.mp3"); // open a file call 'audio.mp3 in a Android phone with router wifi and webserver app started. // not supported?
      }
      else if (scode == 38) { // 3
        if (language == 0) 
          audio.connecttohost("stream.1a-webradio.de/deutsch/mp3-128/vtuner-1a"); //05 1A Deutsche Hits Deutsch Server:Finland OK!
        else if (language == 1) 
          audio.connecttohost("http://8903.brasilstream.com.br:8903/stream?1683061799699"); //Rádio Itatiaia 610 AM 95.7 FM Belo Horizonte / MG - Brasil Server:Brazil OK!
      }
      else if (scode == 37) { // 4
        if (language == 0) 
          audio.connecttohost("https://stream.dashitradio.de/dashitradio/mp3-128/stream.mp3"); //DAS HITRADIO Deutsch English Server:Germany  OK!
        else if (language == 1) 
          audio.connecttohost("https://17513.live.streamtheworld.com/NOVABRASIL_SPAAC.aac"); //Nova Brasil FM news talk brazilian Server: Canada OK!
      }
      else if (scode == 46) { // 5
        if (language == 0) 
          audio.connecttohost("https://streaming.fabrik.fm/voc/echocast/audio/low/index.m3u8"); // Voice of the Cape news talk South Africa Server:South Africa OK!
        else if (language == 1) 
          audio.connecttohost("https://22063.live.streamtheworld.com/JBFMAAC1.aac"); //Rádio JBFM dance pop news talk hits Brasil Server: USA OK!
      }
      else if (scode == 54) { // 6
        if (language == 0) 
          audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3"); //  0 N - 70s on Radio Deutsch Server:Germany OK!
        else if (language == 1) 
          audio.connecttohost("http://13.stmip.net:9274/stream?1679232550177"); // Jornal 91.3 FM Aracaju / SE - Brasil Português Server:USA OK!
      }
      else if (scode == 61) { // 7
        if (language == 0) 
          audio.connecttohost("https://stream2.cnmns.net/paradisefm"); //Paradise FM pop news talk oldies Server:Australia OK!
        else if (language == 1) 
        audio.connecttohost("https://stream-073.zeno.fm/utfkkxfawp8uv"); //  MFM Radio 91.7 Luanda / Angola Server: Canada OK!
      }
      else if (scode == 62) { // 8
        if (language == 0) 
          audio.connecttohost("listen.rusongs.ru/ru-mp3-128"); // RUSSIAN SONGS Server:Russia OK!
        else if (language == 1) 
          audio.connecttohost("https://radios.vpn.sapo.pt/AO/radio3.mp3?1682556173856"); // Rádio Canal A 93.5 FM Luanda / Angola Server: Portugal OK!
      }
      else if (scode == 70) { // 9
        if (language == 0) 
          audio.connecttohost("https://rhema-radio.streamguys1.com/rhema-star.mp3"); // Star FM talk christian  Server:NZ English OK!
        else if (language == 1) 
          audio.connecttohost("http://95.217.75.38:7988/stream?1683064180842"); // Rádio Golo 89.2 FM Lisboa / Portugal Server: Finland OK!
      }
      else if (scode == 69) { // 0
        if (language == 0) 
          audio.connecttohost("https://sp3.servidorrprivado.com:10906/;stream.nsv"); //Radio Fénix 1330 AM news talk sports community Montevideo Argentina Server: Canada Espanish OK! 
        else if (language == 1) 
          audio.connecttohost("http://s1.xps-ip.eu:8118/stream?1683062052432"); // Nova Memória Matosinhos / Portugal Server:France fail! OK! Slow in Brazil
      }
      else if (scode == 77) {  // p for portugues
        language = 1;
        audio.connecttohost("https://1132747t.ha.azioncdn.net/primary/gaucha_rbs.sdp/chunklist_47ea98af-3f33-4a88-9628-a617e3e40466.m3u8"); // Rádio Gaúcha Brasil Server:Brazil OK!
      }
      else if (scode == 75) {  // l for LAN 
        language = 2;
        audio.connecttohost("http://10.42.0.1/audio.m3u"); // open a file call 'audio.m3u in a Linux computer router wifi
      }
      else if (scode == 29) {  // w for world
        language = 0;
        audio.connecttohost("https://prod-52-91-107-216.wostreaming.net/goodkarma-wknramaac-ibc2"); // ESPN 850 AM news talk sports USA
      }
      //else if (scode == 102) {  // get out backspace
      else if (scode == 118) {  // get out backspace
        esp_restart();
      }
    Serial.println(scode);
    }

    
  }

} // App Web Radios






// App Audio Player
void AudioPlayer(){
  Serial.println("Audio Player started");

  if (m_soundGen.playing()) m_soundGen.clear();
  delay(500);
  //m_soundGenerator         m_soundGen;
  m_soundGen.playSound(SineWaveformGenerator(), 163, 300);  //Mi
  m_soundGen.clear();
  m_soundGen.playSound(SineWaveformGenerator(), 158, 300);  //Re#
  delay(500);
  m_soundGen.clear();

  /*if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8)){ // @TODO: reduce to 4?
    hasSD = false;
    Serial.println("SD not Found. ");
  }
  else {
    hasSD = true;
    Serial.println("SD Found. ");
  } */

  // insert words
  //long tempo = 1; // 1 = 1min
  char words[40] = "";
  // user choose
  ibox.setAutoOK(0);
  auto r = ibox.textInput("MP3 Player", "Keywords:", words, 40, nullptr, "Search",false);
  switch (r) {
    //case InputResult::ButtonExt1:
    //Serial.println(URL);
    //getDisk(URL);
    //break;
    case InputResult::Enter:
    Serial.print("words: ");
    Serial.println(words);
    break;
  }

  /* char filename[70] = "/";
  char directory[50];
  strcpy(directory, "/");
  //if (ibox.fileSelector("Audio Player", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter) {
    //Serial.print("directory: ");
    //Serial.println(directory);
    //Serial.print("filename: ");
    //Serial.println(filename);
    char filen[strlen(directory)+strlen(filename)+1] = "";
    strcat(filen,directory);
    Serial.print("filen: ");
    Serial.println(filen);
    strcat(filen,"/");
    strcat(filen,filename);
    //Serial.print("filen: ");
    //Serial.println(filen);
    String filen2;
    filen2 += filen;
    //Serial.print("filen2: ");
    //Serial.println(filen2);
    filen2.remove(0, 3);  
    Serial.print("filen2: ");
    Serial.println(filen2); */
    
  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 

  // No more display needed
  ibox.end();

  SPIFFS.end();

  //Serial.end(); // more memory?

  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  // try to turn off the wifi
  WiFi.mode(WIFI_OFF);

  //SD(SPI)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);
  if (!SD.begin(SD_CS, SPI)) {
      Serial.println("Card Mount Failed");
      //break;
  }
  else Serial.println("Card Mount Ok!");  

  // chama função que procura m




  //Search for the file
  File root1 = SD.open("/");
  File file1 = root1.openNextFile();
  Serial.println(file1);
  while (file1) {
    String temp1 = file1.name();
    Serial.println(temp1);
    if (file1.isDirectory()) { 
      //Serial.println("Pasta:");
      //Serial.println(file);
      File root2 = SD.open(temp1);
      File file2 = root2.openNextFile();
      root = file1;
      path = file1.name();
      while (file2) {
        String temp2 = file2.name();
        Serial.println(temp2);
        if (file2.isDirectory()) {
          File root3 = SD.open(temp2);
          File file3 = root3.openNextFile();
          root = file2;
          path = file2.name();
          while (file3) {
            String temp3 = file3.name();
            Serial.println(temp3);
            if (file3.isDirectory()) {
              File root4 = SD.open(temp3);
              File file4 = root4.openNextFile();
              root = file3;
              path = file3.name();
              while (file4) {
                if (file4.isDirectory()) { }
                else { // root4
                  //root = file4;
                  String temp4 = file4.name();
                  Serial.println(temp4);
                  if (temp4.indexOf(words) != -1){ 
                    //Serial.println("Audio:");
                    //Serial.println(temp);
                    //Serial.println("temp.indexOf(words):");
                    //Serial.println(temp.indexOf(words));
                    if (temp4.endsWith(".mp3") or temp4.endsWith(".wav") or temp4.endsWith(".flac") or temp4.endsWith(".wma")) {
                      Serial.println("Acheiii, o arquivo é...");
                      Serial.println(temp4);
                      //strcpy(temp,filen); 
                      PlayAudio(temp4.c_str());
                    }
                  }
                }
              file4 = root4.openNextFile();
              }
            }
            else { // root3
              //root = file3;
              //Serial.println(temp);
              if (temp3.indexOf(words) != -1){ 
                //Serial.println("Audio:");
                //Serial.println(temp);
                //Serial.println("temp.indexOf(words):");
                //Serial.println(temp.indexOf(words));
                if (temp3.endsWith(".mp3") or temp3.endsWith(".wav") or temp3.endsWith(".flac") or temp3.endsWith(".wma")) {
                  Serial.println("Acheiii, o arquivo é...");
                  Serial.println(temp3);
                  //strcpy(temp,filen); 
                  PlayAudio(temp3.c_str());
                }
              }
            }
            file3 = root3.openNextFile();
              }    
            }
            else { // root2
              //root = file2;
              if (temp2.indexOf(words) != -1){ 
                //Serial.println("Audio:");
                //Serial.println(temp);
                //Serial.println("temp.indexOf(words):");
                //Serial.println(temp.indexOf(words));
                if (temp2.endsWith(".mp3") or temp2.endsWith(".wav") or temp2.endsWith(".flac") or temp2.endsWith(".wma")) {
                  Serial.println("Acheiii, o arquivo é...");
                  Serial.println(temp2);
                  //strcpy(temp,filen); 
                  PlayAudio(temp2.c_str());
                }
              }
          }
      file2 = root2.openNextFile();
      }
    }
    else { // root1
      //root = file1;
      //Serial.println(temp);
      if (temp1.indexOf(words) != -1){ 
        //Serial.println("Audio:");
        //Serial.println(temp);
        //Serial.println("temp.indexOf(words):");
        //Serial.println(temp.indexOf(words));
        if (temp1.endsWith(".mp3") or temp1.endsWith(".wav") or temp1.endsWith(".flac") or temp1.endsWith(".wma")) {
          Serial.println("Acheiii, o arquivo é...");
          Serial.println(temp1);
          //strcpy(temp,filen); 
          PlayAudio(temp1.c_str());
        }
      }
    }
  file1 = root1.openNextFile();
  }
  file1.close();
  esp_restart();
  // alguma variável pra indicar que foi ou não encontrado algum arquivo e som para reiniciar app.


  //char filen3[filen2.length() + 1];
  //filen3 = filen2.c_str(); //incompatible types in assignment of 'const char*' to 'char [50]'
  //strcpy(filen3,filen2.c_str()); 
  //PlayAudio(filen3);

} // End of app MP3 Player





void MusicMaker() {
    Serial.println("Music Maker started");

  fabgl::PS2Controller     PS2Controller;
  //PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);
  PS2Controller.begin();
  auto keyboard = PS2Controller.keyboard();

  if (!keyboard->isKeyboardAvailable())  // if keyboard detected
    {
      ibox.message("", "Keyboard not found!", nullptr, nullptr);
    }
  //Unmound SD
  FileBrowser::unmountSDCard();
  Serial.println("Card unMount!"); 

  // No more display needed
  ibox.end();

  SPIFFS.end();

  // try to turn off the wifi
  WiFi.mode(WIFI_OFF);

  //m_soundGenerator         m_soundGen;

//notes
int c = 16; //do 
int d = 18; //re 
int e = 21; //mi
int f = 22; //fa
int g = 24; //sol
int a = 28; // la
int b = 31;  // si
int cc = 32; // do
//oitava
int octave = 11; // octave = 1 means c -1 
int duration = 90; 
int effect = 0;
int volume = 100;
// duration ok: 10, 20, 

  while (1){
    //if (PS2Controller::keyboard()->scancodeAvailable()) {
    //  int scode = PS2Controller::keyboard()->getNextScancode();
    //if (keyboard->scancodeAvailable()) {
      int scode = keyboard->getNextScancode();
      Serial.println(scode);

      // notes
      if (scode == 28) { // a
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), c*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), e*octave, duration/2);
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), a*(octave-1), duration/2);  
          //m_soundGen.playSound(SineWaveformGenerator(), f*(octave-1), duration/2);  
          m_soundGen.playSound(SineWaveformGenerator(), c*(octave-1), duration/2, volume/2); 
          m_soundGen.playSound(SineWaveformGenerator(), c*(octave-2), duration/2, volume/4);   
        }
      }
      else if (scode == 27) { // s
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), d*octave, duration);   
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), f*octave, duration/2);  
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), b*(octave-1), duration/2);  
          //m_soundGen.playSound(SineWaveformGenerator(), g*(octave-1), duration/2);  
          m_soundGen.playSound(SineWaveformGenerator(), d*(octave-1), duration/2, volume/2);
          m_soundGen.playSound(SineWaveformGenerator(), d*(octave-2), duration/2, volume/4);    
        }
      }
      else if (scode == 35) { // d
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), e*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), g*octave, duration/2); 
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), c*octave, duration/2); 
          //m_soundGen.playSound(SineWaveformGenerator(), a*(octave-1), duration/2);   
          m_soundGen.playSound(SineWaveformGenerator(), e*(octave-1), duration/2, volume/2); 
          m_soundGen.playSound(SineWaveformGenerator(), e*(octave-2), duration/2, volume/4);   
        }
      }
      else if (scode == 43) { // f
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), f*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), a*octave, duration/2);  
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), d*octave, duration/2);  
          //m_soundGen.playSound(SineWaveformGenerator(), b*(octave-1), duration/2);  
          m_soundGen.playSound(SineWaveformGenerator(), f*(octave-1), duration/2, volume/2); 
          m_soundGen.playSound(SineWaveformGenerator(), f*(octave-2), duration/2, volume/4);   
        }
      }
      else if (scode == 52) { // g
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), g*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), b*octave, duration/2);
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), e*octave, duration/2);    
          //m_soundGen.playSound(SineWaveformGenerator(), c*octave, duration/2);    
          m_soundGen.playSound(SineWaveformGenerator(), g*(octave-1), duration/2, volume/2);  
          m_soundGen.playSound(SineWaveformGenerator(), g*(octave-2), duration/2, volume/4);  
        }
      }
      else if (scode == 51) { // h
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), a*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), cc*octave, duration/2);  
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), f*octave, duration/2);
          //m_soundGen.playSound(SineWaveformGenerator(), d*octave, duration/2);
          m_soundGen.playSound(SineWaveformGenerator(), a*(octave-1), duration/2, volume/2);  
          m_soundGen.playSound(SineWaveformGenerator(), a*(octave-2), duration/2, volume/4);  
        }
      }
      else if (scode == 59) { // j
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), b*octave, duration);  
        if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), d*(octave+1), duration/2);  
        else if (effect == 2) {
          //m_soundGen.playSound(SineWaveformGenerator(), g*octave, duration/2);
          //m_soundGen.playSound(SineWaveformGenerator(), e*octave, duration/2);
          m_soundGen.playSound(SineWaveformGenerator(), b*(octave-1), duration/2, volume/2); 
          m_soundGen.playSound(SineWaveformGenerator(), b*(octave-2), duration/2, volume/4);  
        }
      }
      else if (scode == 66) { // k
        if (effect == 0) m_soundGen.clear();
        m_soundGen.playSound(SineWaveformGenerator(), cc*octave, duration);  
          if (effect == 1) m_soundGen.playSound(SineWaveformGenerator(), e*(octave+1), duration/2);  
          else if (effect == 2) {
            //m_soundGen.playSound(SineWaveformGenerator(), a*octave, duration/2);
            //m_soundGen.playSound(SineWaveformGenerator(), f*octave, duration/2);
            m_soundGen.playSound(SineWaveformGenerator(), cc*(octave-1), duration/2, volume/2);  
            m_soundGen.playSound(SineWaveformGenerator(), cc*(octave-2), duration/2, volume/4);  
          }
      }
      // modify

      else if (scode == 21) {  // q
        if (octave > 1) octave = octave - 1;
      }
      else if (scode == 29) {  // w
        if (octave < 15) octave = octave + 1;
      }
      else if (scode == 36) {  // e
        if (duration > 10) duration = duration - 10;
      }
      else if (scode == 45) {  // r
        if (duration < 2000) duration = duration + 10;
      }
      else if (scode == 44) {  // t
        if (effect < 2) effect = effect + 1;
        else if (effect == 2) effect = 0;
      }
      // exit
      else if (scode == 102) {  // get out backspace
        esp_restart();
      }
      //Serial.print("Duration: ");
      //Serial.println(duration);
      //Serial.print("octave: ");
      //Serial.println(octave);
      //Serial.print("effect: ");
      //Serial.println(effect);
  }
    
  //}
} //End of app MusicMaker



// App Timer no video
void TimerApp() {
  Serial.println("TimerApp started");

  //m_soundGenerator         m_soundGen;

  if (m_soundGen.playing()) m_soundGen.clear();
  delay(500);
  //m_soundGenerator         m_soundGen;
  m_soundGen.playSound(SineWaveformGenerator(), 158, 300);  //Re#
  m_soundGen.clear();
  m_soundGen.playSound(SineWaveformGenerator(), 163, 300);  //Mi
  
  //m_soundGen.playSound(SineWaveformGenerator(), 233, 100);  // Y
  //m_soundGen.playSound(SineWaveformGenerator(), 411, 300);  // es
  //m_soundGen.playSound(SineWaveformGenerator(), 158, 500);  //Re#
  //delay(500);
  //if (m_soundGen.playing()) m_soundGen.clear();
  //m_soundGen.playSound(SineWaveformGenerator(), 163, 500);  //Mi
  //delay(500);
  //if (m_soundGen.playing()) m_soundGen.clear();
  //m_soundGen.playSound(SineWaveformGenerator(), 158, 500);  //Re#
  //m_soundGen.clear(); 
  //m_soundGen.playSound(SineWaveformGenerator(), 163, 100);  //Mi

  
  long tempo = 1; // 1 = 1min
  char ctempo[5] = "";
  // user choose minutes
  ibox.setAutoOK(0);
  auto r = ibox.textInput("Timer", "Time (minutes):", ctempo, 6, nullptr, "OK",false);
  switch (r) {
    //case InputResult::ButtonExt1:
    //Serial.println(URL);
    //getDisk(URL);
    //break;
    case InputResult::Enter:
    Serial.print("ctempo: ");
    Serial.println(ctempo);
    break;
  }
  //String stempo;
  //stempo += ctempo;
  char *ptr;
  //tempo = stoi(ctempo); //'stoi' was not declared in this scope
  //tempo = int(stempo); //invalid cast from type 'String' to type 'int'
  tempo = strtol(ctempo, &ptr, 5);
  Serial.print("tempo: ");
  Serial.println(tempo);

  for (int i = 0; i < tempo; ++i) { // para cada min
    Serial.print("i: ");
    Serial.println(i);
    for (int j = 0; j < 61; ++j) { // para cada seg.
      Serial.print("j: ");
      Serial.println(j);
      if (m_soundGen.playing()) m_soundGen.clear();
      m_soundGen.playSound(SineWaveformGenerator(), 163, 30);  //Mi
      //delay(999);
      delay(1000);
    }
  }
  // end
  //m_soundGenerator         m_soundGen;
  //m_soundGen.clear();
  //delay(100);
  if (m_soundGen.playing()) m_soundGen.clear();
  m_soundGen.playSound(SineWaveformGenerator(), 233, 10000);  // end
  delay(1000);

  
} // End app TimerApp
    



void setup()
{
  Serial.begin(115200); delay(500); printf("\n\n\nStart\n\n");// DEBUG ONLY

  randomSeed(analogRead(0));
  
  //Serial.println(ESP.getFreeHeap());
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
  //m_soundGen.playSound(SineWaveformGenerator(), 158, 300);  //Re# No audio before PCEmulator please!
  //m_soundGen.clear();
  //m_soundGen.playSound(SineWaveformGenerator(), 163, 300);  //Mi

  
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
    //m_soundGenerator         m_soundGen;
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
    list.append("Wi-Fi Config"); //1
    list.append("File Browser");
    list.append("JPG View");
    list.append("Web Radios");
    list.append("Audio Player");
    list.append("Wifi Safe");
    list.append("ChatterBox");
    list.append("Restart");

    list.select(0, true);
    //int s = ibox.menu("Applications list", "Click on an item", &list);
    int s = ibox.menu("Applications", "choose the app", &list);
    if (s == 0) {
      //if (hasSD == false) ibox.message("Error!", "microSD-CARD not found!", nullptr, nullptr); 
      //m_soundGenerator     m_soundGen2;
      //m_soundGen2.clear();
      //m_soundGen.clear();
      //m_soundGen.detach(SineWaveformGenerator);
      //m_soundGen.play(false);
      //m_soundGen.play(true);
      //delay(500);
      PCEmulator();
    }
    if (s == 1) {
      checkWiFi();
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
    if (s == 7) {
      //Initialise SPIFFS:
      if(!SPIFFS.begin()){
        Serial.println("SPIFFS Mount Failed");
        return;
      }  
      ChatterBox();
    }
    if (s == 8) esp_restart();
    //if (s == 9) 
  }
  
  else if (audioMode == true) { // Audio mode on
    //m_soundGenerator         m_soundGen;
    //m_soundGen.clear();
    delay(100);
    //m_soundGen.playSound(SineWaveformGenerator(), 233, 200);  // Y OK!
    //m_soundGen.playSound(SineWaveformGenerator(), 411, 200);  // es
    //m_soundGen.playSound(SineWaveformGenerator(), 262, 200);  // Y 
    //m_soundGen.playSound(SineWaveformGenerator(), 330, 200);  // E
    //m_soundGen.playSound(SineWaveformGenerator(), 392, 400); //  S
    m_soundGen.playSound(SineWaveformGenerator(), 262, 200);  // Dó 
    // take a keypress
    Serial.println("take a keypress");
    int scode = PS2Controller::keyboard()->getNextScancode();
    Serial.println(scode);
    
    if (scode == 22) { // 1 WebRádios
      m_soundGen.playSound(SineWaveformGenerator(), 494, 200);  // SI
      m_soundGen.playSound(SineWaveformGenerator(), 494, 200);  // SI
      WebRadios();
    }
    else if (scode == 30) { // 2 Chat server
      m_soundGen.playSound(SineWaveformGenerator(), 440, 200);  // la
      m_soundGen.playSound(SineWaveformGenerator(), 440, 200);  // la
      ChatterBox();
    }
    else if (scode == 38) { // 3 Music Maker()
      m_soundGen.playSound(SineWaveformGenerator(), 392, 200);  // sol
      m_soundGen.playSound(SineWaveformGenerator(), 392, 200);  // sol
      MusicMaker();
    }
    else if (scode == 37) { // 4 timer
      m_soundGen.playSound(SineWaveformGenerator(), 349, 200);  // fa
      m_soundGen.playSound(SineWaveformGenerator(), 349, 200);  // fa
      TimerApp();
    }
    else if (scode == 46) { // 5 Audio Player
      m_soundGen.playSound(SineWaveformGenerator(), 330, 200);  // mi
      m_soundGen.playSound(SineWaveformGenerator(), 330, 200);  // mi
      AudioPlayer();
    }
    
  }
} // end of loop


//O sketch usa 1776202 bytes (56%) de espaço de armazenamento para programas. O máximo são 3145728 bytes.
//Variáveis globais usam 57464 bytes (17%) de memória dinâmica, deixando 270216 bytes para variáveis locais. O máximo são 327680 bytes.
