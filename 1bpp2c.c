/*
 * bmp2c.c - Convert 1bpp BMP files to C arrays
 *
 * Supports bottom-up and top-down BMPs with proper 4-byte row padding.
 * Intended for use with embedded devices for sprites, masks, or codepage typefaces.
 *
 * Usage: 1bpp2c input.bmp output.h
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

static inline uint32_t ABS32_U(int32_t v) {
    return (uint32_t)((v < 0) ? -v : v);
}

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
#pragma pack(pop)


BITMAPFILEHEADER bf;
BITMAPINFOHEADER bi;

FILE *f, *out;
char *infile, *outfile;

uint32_t row_bytes; // padded bytes per row
uint32_t i, x, b, bit_index; // loop variables
int32_t y, abs_height, abs_width, row_idx; // signed loop variables

uint8_t pixel, byte, flip_vertical; // misc
uint8_t *row; // source image pointer

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s input.bmp output.h\n", argv[0]);
        return 1;
    }

    infile = argv[1];
    outfile = argv[2];

    #if (_MSC_VER >= 1400)
        fopen_s(&f, infile, "rb");
    #else
        f = fopen(infile, "rb");
    #endif

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

    row_bytes = ((ABS32_U(bi.biWidth) + 31) / 32)*4; // padded to 4-byte boundary

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

    fprintf(out, "#define BMP_WIDTH  %d\n", abs_width);
    fprintf(out, "#define BMP_HEIGHT %d\n", abs_height);
    fprintf(out, "\n\n");
    fprintf(out, "unsigned char bmp_data[] = {\n");


    flip_vertical = (bi.biHeight > 0);

    for (row_idx = 0; row_idx < abs_height; row_idx++) {
        y = flip_vertical ? (abs_height - 1 - row_idx) : row_idx;

        // Seek to the start of the row in the BMP file
        fseek(f, bf.bfOffBits + y * row_bytes, SEEK_SET);
        fread(row, 1, row_bytes, f);

        // Pack 8 pixels into one byte, MSB = leftmost pixel
        for (x = 0; x < abs_width; x += 8) {
            byte = 0;
            for (b = 0; b < 8; b++) {
                if (x + b < abs_width) {
                    bit_index = 7 - b;             // MSB = leftmost
                    pixel = (row[(x + b) / 8] >> bit_index) & 1;
                    byte |= pixel << (7 - b);
                    }
                    // remaining bits are already 0 for padding
                }
                // Print byte to output C array (inside x loop)
                fprintf(out, "0x%02X%s", byte, (x + 8 < abs_width) ? ", " : ",\n");
           }
    }
    fprintf(out, "};\n");
    free(row);
    fclose(f);
    fclose(out);

    printf("Converted %s -> %s (%dx%d)\n", infile, outfile, abs_width, abs_height);
    return 0;
}
