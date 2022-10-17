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

// #define SD_MOUNT_PATH "/SD"

#define SPIFFS_MOUNT_PATH  "/flash"
#define FORMAT_ON_FAIL     true

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
#include <FS.h>
#include "SPIFFS.h" // ESP32 only
fabgl::VGA16Controller VGAController;
//fabgl::VGAController VGAController; // dont show the complete image .jpg
fabgl::Canvas cv(&VGAController);




// try to connected using saved parameters
bool tryToConnect()
{
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
  FileBrowser fb(SD_MOUNT_PATH);

  char const * filename = nullptr;
  if (url) {
    if (strncmp("://", url + 4, 3) == 0) {
      // this is actually an URL
      filename = strrchr(url, '/') + 1;
      if (filename && !fb.exists(filename, false)) {
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
      if (fb.filePathExists(url))
        filename = url;
    }
  }
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




// App PCEmulator code
void PCEmulator()
{

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
  // Browse Files
  //ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
  //break;
  ////////////////////////////////////////////////////
  // File Select
  if (SPIFFS_MOUNT_PATH) {
    char filename[16] = "/";
    //String filename;
    char directory[32];
    strcpy(directory, SPIFFS_MOUNT_PATH);
    if (ibox.fileSelector("File Select", "Filename: ", directory, sizeof(directory) - 1, filename, sizeof(filename) - 1) == InputResult::Enter) {
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

      // the lib use another way so:
      SPIFFS.end();
      
      ShowJPG(filename);
      //delay(2000);
    }
        
  //Serial.print("hi");
  }
}






void setup()
{
  Serial.begin(115200); delay(500); printf("\n\n\nStart\n\n");// DEBUG ONLY

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

  ibox.onPaint = [&](Canvas * canvas) { drawInfo(canvas); };

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8))   // @TODO: reduce to 4?
    ibox.message("Error!", "This app requires a SD-CARD!", nullptr, nullptr); 

  // uncomment to format SD!
  //FileBrowser::format(fabgl::DriveType::SDCard, 0);

  esp_register_shutdown_handler(shutdownHandler);

  //updateDateTime();

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

  //ibox.setAutoOK(6);
  //int idx = preferences.getInt("dconf", 0);

  //for (bool showDialog = true; showDialog; ) {

    //StringList dconfs;
    //for (auto conf = mconf.getFirstItem(); conf; conf = conf->next)
    //  dconfs.append(conf->desc);
    //dconfs.select(idx, true);
  /*ibox.setAutoOK(0);
  ibox.setupButton(0, "PCEmulator");
  ibox.setupButton(1, "ChatterBox");
  ibox.setupButton(2, "JPG View");
  ibox.setupButton(3, "Update time");
  ibox.setupButton(4, "File Browser");
  
  //auto r = ibox.select("Applications list", "Please select a application", &dconfs, nullptr, " ");
  auto r = ibox.select("Applications list", "Please select a application", &dconfs, nullptr, nullptr);
    idx = dconfs.getFirstSelected(); */


  // Example of simple menu (items from StringList)
  Serial.println("string list");
  fabgl::StringList list;
  list.append("PCEmulator"); //0
  list.append("ChatterBox");
  list.append("File Browser");
  list.append("Update time");
  list.append("JPG View");
  list.select(1, true);
  //int s = ibox.menu("Applications list", "Click on an item", &list);
  int s = ibox.menu("Applications list", ".", &list);
  if (s == 0) PCEmulator();
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
    FileBrowser::mountSPIFFS(FORMAT_ON_FAIL, SPIFFS_MOUNT_PATH);
    ibox.folderBrowser("Browse Files", "/");
    //ibox.folderBrowser("Browse Files", SPIFFS_MOUNT_PATH);
    //ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
  }
  if (s == 3) updateDateTime();
  if (s == 4) {
    JPGView();
  }
  //if (s == 5) 
  //if (s == 6) 
  //if (s == 7) 
  //if (s == 8) 
  //if (s == 9) 
  Serial.println("string list ok");
  //delay(1000);
  //ibox.messageFmt("", nullptr, "OK", "You have selected item %d", s);





   /* switch (r) {
      case InputResult::ButtonExt0:
        // App PCEMulator
        PCEmulator();
        break;
      case InputResult::ButtonExt1:
        // App ChatterBox
        ChatterBox();
        break;
      case InputResult::ButtonExt2:
        // App JPG View
        JPGView();
        break;
      case InputResult::ButtonExt3:
        // conect to internet and update date/time
        updateDateTime();
        break;
      //case InputResult::ButtonExt4:
        // File Browser
      //  fileBrowser = new uiFileBrowser(frame, Point(10, 25), Size(140, 180));
      //  fileBrowser->setDirectory("/");
      //  break;
      case InputResult::Enter:
        showDialog = false;
        break;
        default:
        break;
    }
    // next selection will not have timeout
    ibox.setAutoOK(0); 
  // }
  */
}
