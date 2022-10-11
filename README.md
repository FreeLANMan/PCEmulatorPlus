# PCEmulatorPlus
The PCEmulator by Fabrizio Di Vittorio plus more applications for a more complete system.

Release 0.3:
-- App JPG View: Only .jpg files on internal memory (SPIFFS). Only files with <1Mb for now. Only 512x384 pixels visible (can be change in the code)
Original code: https://github.com/fdivitto/FabGL/discussions/160#discussioncomment-1423074

(Alternatively you can view .gif images in 4 colors and 320x200 using the 'Compushow' application available on the HD-Geral disk image, by PCEmulator: https://drive.google.com/file/d/1sEvAZ998l3wTq4KzkuHQPrPWqZv5kday/view?usp= sharing
I recommend 1st converting the image using the GIMP application with scale image/canvas to fit in 320x200, sharpen filter, indexed mode: 'generate optimum pallete maximum colors 4', 'color dithering Floyd-Steinberg Reduced'.)

-- App ChatterBox: Horizontal chat room server (connects to a network and opens an AP to receive connections). This application can be used even if the Esp32 is not connected to a video monitor. Just repeat the keys used to activate it.
Original code: https://github.com/fenwick67/esp32-chatterbox

-- The PCEmulator: https://github.com/fdivitto/FabGL/tree/master/examples/VGA/PCEmulator

Used libs:
TJpg_Decoder by Bodmer version 0.0.2 https://github.com/Bodmer/TJpg_Decoder/releases/tag/0.0.2
FabGL by Fabrizio Di Vittorio version 1.0.9 https://github.com/fdivitto/FabGL/releases/tag/v1.0.9
And
Board arduino-esp32 by espressif version 1.0.6 https://github.com/espressif/arduino-esp32/releases/tag/1.0.6

"The sketch uses 1558186 bytes (49%) of storage space for programs. The maximum is 3145728 bytes.
Global variables use 55616 bytes (16%) of dynamic memory, leaving 272064 bytes for local variables. The maximum is 327680 bytes."
