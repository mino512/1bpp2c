
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


