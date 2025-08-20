/*
 * bmp2c.c - Convert 1bpp BMP files to C arrays
 *
 * Supports bottom-up and top-down BMPs with proper 4-byte row padding.
 * Intended for use with embedded devices for sprites, masks, or codepage typefaces.
 *
 * Usage: 1bpp2c.exe input.bmp output.h -lsb(optional: LSB first bit order)
 *
 * Author: Braum Swink - Project Architect, Debugging, Portability, Testing.
 * Contributor: ChatGPT - Reviewer, Standards Advisor, Documentation, Technical Consultant.
 * Date: 2025-08-18
 *
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// various flags
#define FLAG_FLIP 0x0001
#define FLAG_PAL  0x0002

// returns an absolute value
static inline uint32_t ABS32_U(int32_t v) {
    return (uint32_t)((v < 0) ? -v : v);
}

// bit order
static inline uint8_t pack_lsb(uint8_t b) {
    // swap half-bytes
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    // swap pairs
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    // swap individual bits
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

// bitmap data types
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
}RGBQUAD;
#pragma pack(pop)

// headers & file handling
BITMAPFILEHEADER bf;
BITMAPINFOHEADER bi;

FILE* f, * out;
char* infile, * outfile;

uint32_t row_bytes; // padded bytes per row
uint32_t i, x, b, bit_index; // loop variables
uint32_t abs_height, abs_width; // absolute values of bitmap dimentions
int32_t y, row_idx, a; // signed loop variables

uint8_t pixel, byte, flip_vertical; // misc
uint16_t flag_bits; // argument flags

uint8_t* row; // source image pointer

// Combined MSDOS and Posix style flag handling
uint8_t parse_flag(const char* flag, char flag_c, const char* flag_str) {
    if (!flag || flag[0] == '\0') return 0;
    if (flag[1] == '\0') return 0;

    // Single-character flag: -x
    if (flag[0] == '-' && flag[1] == flag_c && flag[2] == '\0') {
        return 1;
    }

    // Long-form POSIX flag: --example
    if (flag[0] == '-' && flag[1] == '-' && strcmp(flag + 2, flag_str) == 0) {
        return 2;
    }

    // Single-character MS-DOS style flag: /x (case-insensitive)
    if (flag[0] == '/' && (flag[1] | 0x20) == (flag_c | 0x20) && flag[2] == '\0') {
        return 3;
    }

    // MS-DOS style flag: /example (case-insensitive)
    if (flag[0] == '/' && _stricmp(flag + 1, flag_str) == 0) {
        return 4;
    }

    return 0;
}

int main(int argc, char** argv) {
    // help screen
    if(parse_flag(argv[1], 'h', "help")){
        printf("\n\nCommand line flags:\n\n");
        printf("Usage: 1bpp2c input.bmp output.h\n");
        printf("Help:  1bpp2c -h, /h, --help, or /help => this screen\n\n");

        printf("Combineable flags:\n\n");

        printf("LSB:   1bpp2c input.bmp output.h -l, /l, --lsb, or /lsb\n       output least signifigant bit of each byte first.\n\n");
        printf("PAL:   1bpp2c input.bmp output.h -p, /p, --pal, or /pal\n       output two color palette if present or default palette if absent.\n\n");

        return 0;
    }

    // default: no arguments
    if (argc < 3) {
        printf("Usage: 1bpp2c input.bmp output.h\n");
        printf("Help:  1bpp2c -h, /h, --help, or /help for more options\n");
        return 1;
    }

    // cycle through flags
    flag_bits = 0;
    if (argc > 3) {
        printf("flags detected:\n");
        for (a = 3; a < argc; a++) {
            if (!((parse_flag(argv[a], 'l', "lsb")!=0) || (parse_flag(argv[a], 'p', "pal") != 0))) {
                printf(" %s: not supported\n", argv[a]);
                return 1;
            }
            if (parse_flag(argv[a], 'l', "lsb")) flag_bits |= FLAG_FLIP;
            if (parse_flag(argv[a], 'p', "pal")) flag_bits |= FLAG_PAL;
            printf(" %s\n", argv[a]);
        }
    }

    // file setup
    infile = argv[1];
    outfile = argv[2];

#if (_MSC_VER >= 1400)
    fopen_s(&f, infile, "rb");
#else
    f = fopen(infile, "rb");
#endif

    // error checking
    if (!f) {
        perror("Failed to open input file");
        return 1;
    }

    fread(&bf, sizeof(BITMAPFILEHEADER), 1, f);
    if (bf.bfType != 0x4D42) { // 'BM'
        printf("Not a BMP file\n");
        fclose(f);
        return 1;
    }

    fread(&bi, sizeof(BITMAPINFOHEADER), 1, f);
    if (bi.biBitCount != 1) {
        printf("BMP is not 1bpp\n");
        fclose(f);
        return 1;
    }

    if (bi.biCompression){
        printf("BMP compression not supported. %u\n", bi.biCompression);
        fclose(f);
        return 1;
    }

    row_bytes = ((ABS32_U(bi.biWidth) + 31) / 32) * 4; // padded to 4-byte boundary

    row = (uint8_t*)malloc(row_bytes);
    if (!row) {
        perror("malloc failed");
        fclose(f);
        return 1;
    }

#if (_MSC_VER >= 1400)
    fopen_s(&out, outfile, "w");
#else
    out = fopen(outfile, "w");
#endif

    if (!out) {
        perror("Failed to open output file");
        free(row);
        fclose(f);
        return 1;
    }

    abs_height = ABS32_U(bi.biHeight);
    abs_width = ABS32_U(bi.biWidth);

    // begin output


    printf("writing header\n");
    fprintf(out, "// BMP_WIDTH may not be a multiple of 8; the last byte of each row may contain unused bits.\n");
    fprintf(out, "#define BMP_WIDTH  %d\n", abs_width);
    fprintf(out, "#define BMP_HEIGHT %d\n", abs_height);
    fprintf(out, "\n\n");
    if(flag_bits & FLAG_PAL){
        fprintf(out, "// Bit order: LSB first.\n");
    } else {
        fprintf(out, "// Bit order: MSB first.\n");
    }

    fprintf(out, "unsigned char bmp_data[] = {\n");

    flip_vertical = (bi.biHeight > 0);

    // process and output bitmap data


    printf("writing pixel data\n");
    if (flag_bits & FLAG_FLIP) printf("LSB bit order\n");
    for (row_idx = 0; row_idx < bi.biHeight; row_idx++) {
        y = flip_vertical ? (abs_height - 1 - row_idx) : row_idx;

        // Seek to the start of the row in the BMP file
        fseek(f, bf.bfOffBits + y * row_bytes, SEEK_SET);
        fread(row, 1, row_bytes, f);

        // Pack 8 pixels into one byte, MSB = leftmost pixel
        for (x = 0; x < abs_width; x += 8) {
            // Grab a packed byte straight from the BMP row
            uint8_t byte = row[x >> 3];

            // Handle devices that need reversed bit order
            if (flag_bits & FLAG_FLIP) {
                byte = pack_lsb(byte);   // inline bit-reversal
            }

            // If width isn t multiple of 8, clear extra bits in the last byte
            if (x + 8 > abs_width) {
                int valid_bits = abs_width - x;
                byte &= 0xFF << (8 - valid_bits);
            }
            // Write directly to output array
            fprintf(out, "0x%02X%s", byte, (x + 8 < abs_width) ? ", " : ",\n");
        }
    }
    fprintf(out, "};\n\n");

    // output palette
    if (flag_bits & FLAG_PAL) {
        RGBQUAD pal[2];
        if(!bi.biClrUsed) {
            // default palette
            printf("default palette\n");
            fprintf(out, "// color order: blue, green, red \n");
            fprintf(out, "unsigned char default_pal[] = {\n");
            fprintf(out, "0x00, 0x00, 0x00, \n");
            fprintf(out, "0xff, 0xff, 0xff, \n");
            fprintf(out, "};\n\n");
        } else if (bi.biClrUsed == 2) {
            // real palette
            printf("writing palette\n");
            fseek(f, bf.bfOffBits - (sizeof(RGBQUAD) * 2), SEEK_SET);
            fread(pal, sizeof(RGBQUAD), 2, f);
            fprintf(out, "// color order: blue, green, red \n");
            fprintf(out, "unsigned char bmp_pal[] = {\n");
            fprintf(out, "0x%02X, 0x%02X, 0x%02X%s", pal[0].blue, pal[0].green, pal[0].red, ", \n");
            fprintf(out, "0x%02X, 0x%02X, 0x%02X%s", pal[1].blue, pal[1].green, pal[1].red, ", \n");
            fprintf(out, "};\n\n");
        } else {
            fprintf(stderr, "Unsupported palette size for 1bpp BMP: %u\n", bi.biClrUsed);
            return 1;
        }
    }

    // free pointers
    free(row);
    fclose(f);
    fclose(out);

    // completion message
    printf("Converted %s -> %s (%d x %d)\n", infile, outfile, (unsigned int)abs_width, (unsigned int)abs_height);
    return 0;
}
