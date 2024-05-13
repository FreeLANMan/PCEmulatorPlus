# PCEmulatorPlus
The PCEmulator by Fabrizio Di Vittorio plus applications for a more complete system. For the TTGO VGA32 1.4 board and compatibles.

- What is this for?
  
This code allows you to use a microcontroller-based board (Esp32) as if it were a computer. This is accomplished by combining a 16-bit PC emulator (8086-CGA/Hercules) with more modern applications (MP3 player, JPG image viewer, microSD card file manager, etc.).

- What are the advantages?
  
This allows you to have a low-power, cheap, offgrid, and highly modular computer. Suitable for extreme contexts (lack of electricity, market crisis, etc.)

- What are the disadvantages?
  
The computer built using this code has a much lower processing capacity than what is normally seen. You won't be able to watch videos (properly), surf the Internet or play photorealistic action games.

Apps:

**With Display:**
- PCEmulator (IBM PC/XT emulator with CGA and Hercules graphics);
- ChatterBox (Horizontal chat server);
- File Browser;
- Jpg View (see images from SD card) (only 400x300, 64 colors max.;
- Web Radios (Online Radios from many countrys);
- Audio Player (play .mp3, .wav, etc. from SD);
- Wi-fi Safe (test security of wifi);
- Commodore VIC-20 emulator (Only PAL, only crt and prg files).

**With Audio:**
1) Web Radios
2) Chat Server
3) Audio Maker
4) Timer
5) Audio Player (play .mp3, .wav, etc. from SD);

See the wiki for limitations and how to use this system.

The system is being designed to work even if part of the hardware is unavailable, thus avoiding the production of electronic waste in the event of a component failure.
What works without...
- microSD card: ChatterBox, Web Radios, Wi-fi Safe, Audio Maker,
- Sound output: Wi-fi Safe, File Browser, Jpg View, PCEmulator,
- Wi-Fi: Audio Player, File Browser, Jpg View, PCEmulator, Audio Maker,
- Mouse: PCEmulator, File Browser, Jpg View, PCEmulator, Audio Player, Timer, Audio Maker,
- Keyboard: File Browser, ChatterBox, Wi-fi Safe,
- PSRAM: ChatterBox, Wi-fi Safe, File Browser, Timer, 
- VGA output: ChatterBox, Web Radios, Audio Maker, Audio Player, Timer, 

If you don't use the video output you will need to use a keyboard and the audio output.
Theoretically, some applications can even be used by a blind person if an accessible keyboard is connected.

Disks for the PCEmulator:
https://drive.google.com/drive/folders/1OFRNx3wDCHZ4ajmrT9v_o90OhCe7HoOJ?usp=share_link

Used libs:
FabGL by Fabrizio Di Vittorio version 1.0.9 https://github.com/fdivitto/FabGL/releases/tag/v1.0.9
TJpg_Decoder by Bodmer version 0.0.2 https://github.com/Bodmer/TJpg_Decoder/releases/tag/0.0.2
ESP32-audioI2S por Wolle https://github.com/schreibfaul1/ESP32-audioI2S
And
Board arduino-esp32 by espressif version 1.0.6 https://github.com/espressif/arduino-esp32/releases/tag/1.0.6
