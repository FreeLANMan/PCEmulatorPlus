# PCEmulatorPlus
The PCEmulator by Fabrizio Di Vittorio plus more applications for a more complete system.

Apps:
- PCEmulator by Fabrizio Di Vittorio;
- ChatterBox (Horizontal chat server);
- File Browser;
- Jpg View (see images from internal memory);
- Web Radios (Online Radios from many countrys);
- Audio Player (play .mp3, .wav, etc. from SD);
- Wi-fi Safe (test security of wifi);
- Audio Maker (comming soon!)

See the wiki for limitations and how to use this system.

The system is being designed to work even if part of the hardware is unavailable, thus avoiding the production of electronic waste in the event of a component failure.
What works without...
- microSD card: ChatterBox, Web Radios, Wi-fi Safe, 
- Sound output: Wi-fi Safe, File Browser, Jpg View, 
- Wi-Fi: Audio Player, File Browser, Jpg View, 
- Mouse: PCEmulator, File Browser, Jpg View, 
- Keyboard: File Browser, ChatterBox, Wi-fi Safe,
- PSRAM: ChatterBox, Wi-fi Safe, File Browser, 
- VGA output: ChatterBox, Web Radios, Audio Maker (comming soon!)

Theoretically, some applications can even be used by a blind person if an accessible keyboard is connected.

Used libs:
FabGL by Fabrizio Di Vittorio version 1.0.9 https://github.com/fdivitto/FabGL/releases/tag/v1.0.9
TJpg_Decoder by Bodmer version 0.0.2 https://github.com/Bodmer/TJpg_Decoder/releases/tag/0.0.2
ESP32-audioI2S por Wolle https://github.com/schreibfaul1/ESP32-audioI2S
And
Board arduino-esp32 by espressif version 1.0.6 https://github.com/espressif/arduino-esp32/releases/tag/1.0.6



Release 0.7 notes:

Now you'll also be able to listen to audio files (mp3, wav, ...) directly from the micro SD card. Adding this functionality required a lot of system resources and is therefore severely limited.
Limitations:
- Mono only
- No controls, you will have to change the volume on the device you are using as a speaker.
- Only 1 audio file at a time.

This functionality will probably be changed in future versions since the plan is to have applications that work without needing a display and the way 'Audio Player' is currently working still has this need.


Release 0.6:
Now with Webradios!

How to use WebRadio:
- Keys from 1 to 0 to choose the radio.
- 'Up' and 'Down' keys to change the volume.
- 'Backspace' key to exit the app.
- The key needs to be pressed for about 10 seconds to work.
- You can add radios by changing the code. It would even be possible to place several radios on the letter keys. You can also change the code to play playlists over your local area network (LAN).

The idea is to make the system also work without a monitor and will be implemented in future versions with more apps.

Used libs:
TJpg_Decoder by Bodmer version 0.0.2 https://github.com/Bodmer/TJpg_Decoder/releases/tag/0.0.2
FabGL by Fabrizio Di Vittorio version 1.0.9 https://github.com/fdivitto/FabGL/releases/tag/v1.0.9
And
Board arduino-esp32 by espressif version 1.0.6 https://github.com/espressif/arduino-esp32/releases/tag/1.0.6
ESP32-audioI2S por Wolle https://github.com/schreibfaul1/ESP32-audioI2S

"O sketch usa 1766738 bytes (56%) de espaço de armazenamento para programas. O máximo são 3145728 bytes.
Variáveis globais usam 57456 bytes (17%) de memória dinâmica, deixando 270224 bytes para variáveis locais. O máximo são 327680 bytes."



Release 0.5:
Five Apps in main menu:
-- PCEmulator (The main app for use many O.S. and apps)

-- App ChatterBox: Horizontal chat room server (connects to a network and opens an AP to receive connections). This application can be used even if the Esp32 is not connected to a video monitor. Just repeat the keys used to activate it: Enter, wait <10 seconds press Enter again.
Original code: https://github.com/fenwick67/esp32-chatterbox

-- App JPG View: Only .jpg files on internal memory (SPIFFS). Only files with <1Mb for now. Only 512x384 pixels visible (can be change in the code)
Original code: https://github.com/fdivitto/FabGL/discussions/160#discussioncomment-1423074
Recomended: Use the Hercules mode and convert the imagens to see in 'Compushow' or VUIMG.
(Alternatively you can view .gif images in 4 colors and 320x200 using the 'Compushow' application available on the HD-Geral disk image, by PCEmulator: https://drive.google.com/file/d/1sEvAZ998l3wTq4KzkuHQPrPWqZv5kday/view?usp= sharing
I recommend 1st converting the image using the GIMP application with scale image/canvas to fit in 320x200, sharpen filter, indexed mode: 'generate optimum pallete maximum colors 4', 'color dithering Floyd-Steinberg Reduced'.)

-- File Browser: by Fabrizio Di Vittorio. You can move files between internal memory and microSD card.

-- Update time: by Fabrizio Di Vittorio. Download the correct date and time from the internet. I intend to add functions in future.

Used libs:
TJpg_Decoder by Bodmer version 0.0.2 https://github.com/Bodmer/TJpg_Decoder/releases/tag/0.0.2
FabGL by Fabrizio Di Vittorio version 1.0.9 https://github.com/fdivitto/FabGL/releases/tag/v1.0.9
And
Board arduino-esp32 by espressif version 1.0.6 https://github.com/espressif/arduino-esp32/releases/tag/1.0.6

"O sketch usa 1558458 bytes (49%) de espaço de armazenamento para programas. O máximo são 3145728 bytes.
Variáveis globais usam 55616 bytes (16%) de memória dinâmica, deixando 272064 bytes para variáveis locais. O máximo são 327680 bytes."

Disks for the PCEmulator:
https://drive.google.com/drive/folders/1OFRNx3wDCHZ4ajmrT9v_o90OhCe7HoOJ?usp=share_link
