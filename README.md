Working with microcontrollers often means squeezing every last byte of memory. One of the biggest challenges is simply getting bitmaps into your code efficiently.  

It’s often the case that a simple user interface is preferred on a microcontroller requiring only 2-color images for output on devices like
LED-arrays, LCD, OLED, etc...

While there are online tools for converting images into code, I discovered that online code converters only seem to produce 16bpp, 24bpp, and 32bpp formats.
Those image formats take up an extraordinary amount of space on the microcontroller and are limited to high-color displays.

###### Formula:
  - 1bpp  = (width\*height)/8
  - 16bpp  = (width\*height*2) 
  - 24bpp  = (width\*height*3) 
  - 32bpp  = (width\*height*4) 
 
#### Example:  
- `256 x 128` @ 1bpp → **4,096 bytes**  
- The same image at only 16bpp → **65,536 bytes** (!)

That’s a **16× memory saving**. Perfect for microcontrollers.  

#### The solution: 1bpp2c

`1bpp2c` takes a **(1bpp) .bmp file** and converts it into **C code array** that can be used with a wide range of compilers and environments, including:
- Arduino IDE  
- Visual Studio Code / PlatformIO  
- ESP-IDF  
- and more

#### Features:
- Converts `.bmp` images (1bpp) into C arrays
- Output ready for direct use in microcontroller projects
- No bloat — only 2 colors stored
- Extremely small memory footprint
- Defaults to **MSB** bit order (bits are read left to right) 
- Added in v1.1.0
  - *--lsb* flag for **LSB** first bit order.
  - *--pal* flag for exporting the 2-color palette as an array.

#### Usage Example:

Input: `logo.bmp` (1bpp)  
1bpp2c logo.bmp logo.h *--pal*

###### Example output:

> #define BMP_WIDTH  256
>
> #define BMP_HEIGHT 128


> // Bit order: MSB first.
>
> unsigned char bmp_data[] = {
>
> 0x7A, 0x31, 0x37, // ...
>
> };


> // color order: blue, green, red
>
> unsigned char default_pal[] = {
>
> 0x00, 0x00, 0x00, 
>
> 0xff, 0xff, 0xff, 
>
> };


