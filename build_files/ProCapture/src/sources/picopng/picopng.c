// picoPNG version 20080503 (cleaned up and ported to c by kaitek)
// Copyright (c) 2005-2008 Lode Vandevenne
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//   2. Altered source versions must be plainly marked as such, and must not be
//      misrepresented as being the original software.
//   3. This notice may not be removed or altered from any source distribution.

// Copy From: http://forge.voodooprojects.org/svn/chameleon/branches/cparm/i386/modules/GUI/picopng.c
// Modified by Magewell for driver support
// Copyright (c) 2015 Magewell Electronics Co., Ltd.

#include "picopng.h"

/*************************************************************************************************/
int Zlib_decompress(struct karray *out, struct karray *in) // returns error value
{
    int ret = 0;
    size_t out_size = karray_get_size(out);

    ret = os_zlib_uncompress(karray_get_data(out), &out_size,
                                  karray_get_data(in), karray_get_size(in));

    if (ret == OS_RETURN_SUCCESS && out_size > 0) {
        // Only now we know the true size of out, resize it to that
        karray_set_count(out, out_size, true);
    }

    return ret;
}

/*************************************************************************************************/

#define PNG_SIGNATURE	0x0a1a0a0d474e5089ull

#define CHUNK_IHDR		0x52444849
#define CHUNK_IDAT		0x54414449
#define CHUNK_IEND		0x444e4549
#define CHUNK_PLTE		0x45544c50
#define CHUNK_tRNS		0x534e5274


uint32_t PNG_readBitFromReversedStream(size_t *bitp, const uint8_t *bits)
{
    uint32_t result = (bits[*bitp >> 3] >> (7 - (*bitp & 0x7))) & 1;
    (*bitp)++;
    return result;
}

uint32_t PNG_readBitsFromReversedStream(size_t *bitp, const uint8_t *bits, uint32_t nbits)
{
    uint32_t i, result = 0;
    for (i = nbits - 1; i < nbits; i--)
        result += ((PNG_readBitFromReversedStream(bitp, bits)) << i);
    return result;
}

void PNG_setBitOfReversedStream(size_t *bitp, uint8_t *bits, uint32_t bit)
{
    bits[*bitp >> 3] |= (bit << (7 - (*bitp & 0x7)));
    (*bitp)++;
}

uint32_t PNG_read32bitInt(const uint8_t *buffer)
{
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

int PNG_checkColorValidity(uint32_t colorType, uint32_t bd) // return type is a LodePNG error code
{
    if ((colorType == 2 || colorType == 4 || colorType == 6)) {
        if (!(bd == 8 || bd == 16))
            return 37;
        else
            return 0;
    } else if (colorType == 0) {
        if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16))
            return 37;
        else
            return 0;
    } else if (colorType == 3) {
        if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8))
            return 37;
        else
            return 0;
    } else
        return 31; // nonexistent color type
}

uint32_t PNG_getBpp(const PNG_info_t *info)
{
    uint32_t bitDepth, colorType;
    bitDepth = info->bitDepth;
    colorType = info->colorType;
    if (colorType == 2)
        return (3 * bitDepth);
    else if (colorType >= 4)
        return (colorType - 2) * bitDepth;
    else
        return bitDepth;
}

int PNG_readPngHeader(PNG_info_t *info, const uint8_t *in, size_t inlength)
{
    // read the information from the header and store it in the Info
    if (inlength < 29) {
        return 27; // error: the data length is smaller than the length of the header
    }
    if (*(uint64_t *) in != PNG_SIGNATURE) {
        return 28; // no PNG signature
    }
    if (*(uint32_t *) &in[12] != CHUNK_IHDR) {
        return 29; // error: it doesn't start with a IHDR chunk!
    }
    info->width = PNG_read32bitInt(&in[16]);
    info->height = PNG_read32bitInt(&in[20]);
    info->bitDepth = in[24];
    info->colorType = in[25];
    info->compressionMethod = in[26];
    if (in[26] != 0) {
        return 32; // error: only compression method 0 is allowed in the specification
    }
    info->filterMethod = in[27];
    if (in[27] != 0) {
        return 33; // error: only filter method 0 is allowed in the specification
    }
    info->interlaceMethod = in[28];
    if (in[28] > 1) {
        return 34; // error: only interlace methods 0 and 1 exist in the specification
    }
    return PNG_checkColorValidity(info->colorType, info->bitDepth);
}

int PNG_paethPredictor(int a, int b, int c) // Paeth predicter, used by PNG filter type 4
{
    int p, pa, pb, pc;
    p = a + b - c;
    pa = p > a ? (p - a) : (a - p);
    pb = p > b ? (p - b) : (b - p);
    pc = p > c ? (p - c) : (c - p);
    return (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
}

int PNG_unFilterScanline(uint8_t *recon, const uint8_t *scanline, const uint8_t *precon,
                          size_t bytewidth, uint32_t filterType, size_t length)
{
    size_t i;
    if (!recon) {
        return 1; // FIXME : find the appropiate error return
    }
    switch (filterType) {
        case 0:
            for (i = 0; i < length; i++)
                recon[i] = scanline[i];
            break;
        case 1:
            for (i = 0; i < bytewidth; i++)
                recon[i] = scanline[i];
            for (i = bytewidth; i < length; i++)
                recon[i] = scanline[i] + recon[i - bytewidth];
            break;
        case 2:
            if (precon)
                for (i = 0; i < length; i++)
                    recon[i] = scanline[i] + precon[i];
            else
                for (i = 0; i < length; i++)
                    recon[i] = scanline[i];
            break;
        case 3:
            if (precon) {
                for (i = 0; i < bytewidth; i++)
                    recon[i] = scanline[i] + precon[i] / 2;
                for (i = bytewidth; i < length; i++)
                    recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
            } else {
                for (i = 0; i < bytewidth; i++)
                    recon[i] = scanline[i];
                for (i = bytewidth; i < length; i++)
                    recon[i] = scanline[i] + recon[i - bytewidth] / 2;
            }
            break;
        case 4:
            if (precon) {
                for (i = 0; i < bytewidth; i++)
                    recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(0, precon[i], 0));
                for (i = bytewidth; i < length; i++)
                    recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(recon[i - bytewidth],
                                                                           precon[i], precon[i - bytewidth]));
            } else {
                for (i = 0; i < bytewidth; i++)
                    recon[i] = scanline[i];
                for (i = bytewidth; i < length; i++)
                    recon[i] = (uint8_t) (scanline[i] + PNG_paethPredictor(recon[i - bytewidth], 0, 0));
            }
            break;
        default:
            return 36; // error: nonexistent filter type given
    }

    return 0;
}

void PNG_adam7Pass(uint8_t *out, uint8_t *linen, uint8_t *lineo, const uint8_t *in, uint32_t w,
                   size_t passleft, size_t passtop, size_t spacex, size_t spacey, size_t passw, size_t passh,
                   uint32_t bpp)
{
    size_t bytewidth, linelength;
    uint32_t y;
    uint8_t *temp;
    int PNG_error;
    // filter and reposition the pixels into the output when the image is Adam7 interlaced. This
    // function can only do it after the full image is already decoded. The out buffer must have
    // the correct allocated memory size already.
    if (passw == 0)
        return;
    bytewidth = (bpp + 7) / 8;
    linelength = 1 + ((bpp * passw + 7) / 8);
    for (y = 0; y < passh; y++) {
        size_t i, b;
        uint8_t filterType = in[y * linelength], *prevline = (y == 0) ? 0 : lineo;
        PNG_error = PNG_unFilterScanline(
                    linen,&in[y * linelength + 1], prevline,
                    bytewidth, filterType,
                    (w * bpp + 7) / 8);
        if (PNG_error)
            return;
        if (bpp >= 8)
            for (i = 0; i < passw; i++)
                for (b = 0; b < bytewidth; b++) // b = current byte of this pixel
                    out[bytewidth * w * (passtop + spacey * y) + bytewidth *
                        (passleft + spacex * i) + b] = linen[bytewidth * i + b];
        else
            for (i = 0; i < passw; i++) {
                size_t obp, bp;
                obp = bpp * w * (passtop + spacey * y) + bpp * (passleft + spacex * i);
                bp = i * bpp;
                for (b = 0; b < bpp; b++)
                    PNG_setBitOfReversedStream(&obp, out, PNG_readBitFromReversedStream(&bp, linen));
            }
        temp = linen;
        linen = lineo;
        lineo = temp; // swap the two buffer pointers "line old" and "line new"
    }
}

int PNG_convert(PNG_info_t *info, struct karray *out, const uint8_t *in)
{	// converts from any color type to 32-bit. return value = LodePNG error code
    size_t i, c;
    uint32_t bitDepth, colorType;
    size_t numpixels, bp;
    uint8_t *out_data;

    bitDepth = info->bitDepth;
    colorType = info->colorType;
    numpixels = info->width * info->height;
    bp = 0;
    if (!karray_set_count(out, numpixels * 4, false)) {
        return 1; // FIXME : find the appropiate error return
    }
    out_data = karray_get_data(out);

    if (bitDepth == 8 && colorType == 0) // greyscale
        for (i = 0; i < numpixels; i++) {
            out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[i];
            out_data[4 * i + 3] = (info->key_defined && (in[i] == info->key_r)) ? 0 : 255;
        }
    else if (bitDepth == 8 && colorType == 2) // RGB color
        for (i = 0; i < numpixels; i++) {
            for (c = 0; c < 3; c++)
                out_data[4 * i + c] = in[3 * i + c];
            out_data[4 * i + 3] = (info->key_defined && (in[3 * i + 0] == info->key_r) &&
                                   (in[3 * i + 1] == info->key_g) && (in[3 * i + 2] == info->key_b)) ? 0 : 255;
        }
    else if (bitDepth == 8 && colorType == 3) // indexed color (palette)
        for (i = 0; i < numpixels; i++) {
            if (4U * in[i] >= karray_get_count(&info->arr_palette))
                return 46;
            for (c = 0; c < 4; c++) // get rgb colors from the palette
                out_data[4 * i + c] =
                        *(uint8_t *)karray_get_element(
                            &info->arr_palette, 4 * in[i] + c);
        }
    else if (bitDepth == 8 && colorType == 4) // greyscale with alpha
        for (i = 0; i < numpixels; i++) {
            out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[2 * i + 0];
            out_data[4 * i + 3] = in[2 * i + 1];
        }
    else if (bitDepth == 8 && colorType == 6)
        for (i = 0; i < numpixels; i++)
            for (c = 0; c < 4; c++)
                out_data[4 * i + c] = in[4 * i + c]; // RGB with alpha
    else if (bitDepth == 16 && colorType == 0) // greyscale
        for (i = 0; i < numpixels; i++) {
            out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[2 * i];
            out_data[4 * i + 3] = (info->key_defined && (256U * in[i] + in[i + 1] == info->key_r))
            ? 0 : 255;
        }
    else if (bitDepth == 16 && colorType == 2) // RGB color
        for (i = 0; i < numpixels; i++) {
            for (c = 0; c < 3; c++)
                out_data[4 * i + c] = in[6 * i + 2 * c];
            out_data[4 * i + 3] = (info->key_defined &&
                                   (256U * in[6 * i + 0] + in[6 * i + 1] == info->key_r) &&
                                   (256U * in[6 * i + 2] + in[6 * i + 3] == info->key_g) &&
                                   (256U * in[6 * i + 4] + in[6 * i + 5] == info->key_b)) ? 0 : 255;
        }
    else if (bitDepth == 16 && colorType == 4) // greyscale with alpha
        for (i = 0; i < numpixels; i++) {
            out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = in[4 * i]; // msb
            out_data[4 * i + 3] = in[4 * i + 2];
        }
    else if (bitDepth == 16 && colorType == 6)
        for (i = 0; i < numpixels; i++)
            for (c = 0; c < 4; c++)
                out_data[4 * i + c] = in[8 * i + 2 * c]; // RGB with alpha
    else if (bitDepth < 8 && colorType == 0) // greyscale
        for (i = 0; i < numpixels; i++) {
            uint32_t value = (PNG_readBitsFromReversedStream(&bp, in, bitDepth) * 255) /
            ((1 << bitDepth) - 1); // scale value from 0 to 255
            out_data[4 * i + 0] = out_data[4 * i + 1] = out_data[4 * i + 2] = (uint8_t) value;
            out_data[4 * i + 3] = (info->key_defined && value &&
                                   (((1U << bitDepth) - 1U) == info->key_r) && ((1U << bitDepth) - 1U)) ? 0 : 255;
        }
    else if (bitDepth < 8 && colorType == 3) // palette
        for (i = 0; i < numpixels; i++) {
            uint32_t value = PNG_readBitsFromReversedStream(&bp, in, bitDepth);
            if (4 * value >= karray_get_count(&info->arr_palette))
                return 47;
            for (c = 0; c < 4; c++) // get rgb colors from the palette
                out_data[4 * i + c] =
                        *(uint8_t *)karray_get_element(
                            &info->arr_palette, 4 * value + c);
        }
    return 0;
}

int PNG_decode(PNG_info_t *info, const uint8_t *in, uint32_t size)
{
    size_t pos;
    struct karray arr_idat; // the data from idat chunks
    bool IEND;
    uint32_t bpp;
    struct karray arr_scanlines;
    uint8_t *scanlines_data;
    size_t bytewidth, outlength;
    uint8_t *out_data;
    int PNG_error;

    PNG_error = 0;
    if (info == NULL || size == 0 || in == 0) {
        return 48; // the given data is empty
    }

    PNG_error = PNG_readPngHeader(info, in, size);
    if (PNG_error)
        return PNG_error;

    karray_init(&info->arr_palette, sizeof(uint8_t));
    karray_init(&info->arr_image, sizeof(uint8_t));

    karray_init(&arr_idat, sizeof(uint8_t));
    karray_init(&arr_scanlines, sizeof(uint8_t));

    pos = 33; // first byte of the first chunk after the header
    IEND = false;
    info->key_defined = false;
    // loop through the chunks, ignoring unknown chunks and stopping at IEND chunk. IDAT data is
    // put at the start of the in buffer
    while (!IEND) {
        size_t i, j;
        size_t chunkLength;
        uint32_t chunkType;

        if (pos + 8 >= size) {
            PNG_error = 30; // error: size of the in buffer too small to contain next chunk
            goto out;
        }
        chunkLength = PNG_read32bitInt(&in[pos]);
        pos += 4;
        if (chunkLength > 0x7fffffff) {
            PNG_error = 63;
            goto out;
        }
        if (pos + chunkLength >= size) {
            PNG_error = 35; // error: size of the in buffer too small to contain next chunk
            goto out;
        }
        chunkType = *(uint32_t *) &in[pos];
        if (chunkType == CHUNK_IDAT) { // IDAT: compressed image data chunk
            if(!karray_append(&arr_idat, (void *)&in[pos + 4], chunkLength)) {
                PNG_error = 1;
                goto out;
            }

            pos += (4 + chunkLength);
        } else if (chunkType == CHUNK_IEND) { // IEND
            pos += 4;
            IEND = true;
        } else if (chunkType == CHUNK_PLTE) { // PLTE: palette chunk
            uint8_t alpha = 255;
            pos += 4; // go after the 4 letters
            if (!karray_set_count(&info->arr_palette, 4 * (chunkLength / 3),
                                  false)) {
                PNG_error = 1;
                goto out;
            }

            if (karray_get_count(&info->arr_palette) > (4 * 256)) {
                PNG_error = 38; // error: palette too big
                goto out;
            }
            for (i = 0; i < karray_get_count(&info->arr_palette); i += 4) {
                for (j = 0; j < 3; j++)
                    // RGB
                    karray_set_element(&info->arr_palette, i + j, (void *)&in[pos++]);
                karray_set_element(&info->arr_palette, i + 3, &alpha); // alpha
            }
        } else if (chunkType == CHUNK_tRNS) { // tRNS: palette transparency chunk
            pos += 4; // go after the 4 letters
            if (info->colorType == 3) {
                if (4 * chunkLength > karray_get_count(&info->arr_palette)) {
                    PNG_error = 39; // error: more alpha values given than there are palette entries
                    goto out;
                }
                for (i = 0; i < chunkLength; i++)
                    karray_set_element(&info->arr_palette, 4 * i + 3,
                                       (void *)&in[pos++]);
            } else if (info->colorType == 0) {
                if (chunkLength != 2) {
                    PNG_error = 40; // error: this chunk must be 2 bytes for greyscale image
                    goto out;
                }
                info->key_defined = true;
                info->key_r = info->key_g = info->key_b = 256 * in[pos] + in[pos + 1];
                pos += 2;
            } else if (info->colorType == 2) {
                if (chunkLength != 6) {
                    PNG_error = 41; // error: this chunk must be 6 bytes for RGB image
                    goto out;
                }
                info->key_defined = true;
                info->key_r = 256 * in[pos] + in[pos + 1];
                pos += 2;
                info->key_g = 256 * in[pos] + in[pos + 1];
                pos += 2;
                info->key_b = 256 * in[pos] + in[pos + 1];
                pos += 2;
            } else {
                PNG_error = 42; // error: tRNS chunk not allowed for other color models
                goto out;
            }
        } else { // it's not an implemented chunk type, so ignore it: skip over the data
            if (!(in[pos + 0] & 32)) {
                // error: unknown critical chunk (5th bit of first byte of chunk type is 0)
                PNG_error = 69;
                goto out;
            }
            pos += (chunkLength + 4); // skip 4 letters and uninterpreted data of unimplemented chunk
        }
        pos += 4; // step over CRC (which is ignored)
    }
    bpp = PNG_getBpp(info);
    // now the out buffer will be filled
    if (!karray_set_count(
                &arr_scanlines,
                ((info->width * (info->height * bpp + 7)) / 8) + info->height,
                false)) {
        PNG_error = 1;
        goto out;
    }
    PNG_error = Zlib_decompress(&arr_scanlines, &arr_idat);
    if (PNG_error)
        return PNG_error; // stop if the zlib decompressor returned an error
    bytewidth = (bpp + 7) / 8;
    outlength = (info->height * info->width * bpp + 7) / 8;
    // time to fill the out buffer
    if (!karray_set_count(&info->arr_image, outlength, false)) {
        PNG_error = 1;
        goto out;
    }
    out_data = outlength ? karray_get_data(&info->arr_image) : 0;
    scanlines_data = karray_get_data(&arr_scanlines);
    if (info->interlaceMethod == 0) { // no interlace, just filter
        size_t y, obp, bp;
        size_t linestart, linelength;
        linestart = 0;
        // length in bytes of a scanline, excluding the filtertype byte
        linelength = (info->width * bpp + 7) / 8;
        if (bpp >= 8) {// byte per byte
            for (y = 0; y < info->height; y++) {
                uint32_t filterType = scanlines_data[linestart];
                const uint8_t *prevline;
                prevline = (y == 0) ? 0 : &out_data[(y - 1) * info->width * bytewidth];
                PNG_error = PNG_unFilterScanline(&out_data[linestart - y],
                        &scanlines_data[linestart + 1],
                        prevline, bytewidth, filterType, linelength);
                if (PNG_error)
                    goto out;
                linestart += (1 + linelength); // go to start of next scanline
            }
        } else { // less than 8 bits per pixel, so fill it up bit per bit
            struct karray arr_templine; // only used if bpp < 8
            karray_init(&arr_templine, sizeof(uint8_t));
            karray_set_count(&arr_templine, (info->width * bpp + 7) >> 3, false);
            for (y = 0, obp = 0; y < info->height; y++) {
                uint32_t filterType = scanlines_data[linestart];
                const uint8_t *prevline;
                prevline = (y == 0) ? 0 : &out_data[(y - 1) * info->width * bytewidth];

                PNG_error = PNG_unFilterScanline(
                            karray_get_data(&arr_templine),
                            &scanlines_data[linestart + 1],
                            prevline,
                            bytewidth,
                            filterType,
                            linelength);
                if (PNG_error) {
                    karray_free(&arr_templine);
                    goto out;
                }

                for (bp = 0; bp < info->width * bpp;)
                    PNG_setBitOfReversedStream(
                                &obp,
                                out_data,
                                PNG_readBitFromReversedStream(
                                    &bp, karray_get_data(&arr_templine))
                                );
                linestart += (1 + linelength); // go to start of next scanline
            }
            karray_free(&arr_templine);
        }
    } else { // interlaceMethod is 1 (Adam7)
        int i;
        struct karray arr_scanlineo, arr_scanlinen; // "old" and "new" scanline

        size_t passw[7] = {
            (info->width + 7) / 8, (info->width + 3) / 8, (info->width + 3) / 4,
            (info->width + 1) / 4, (info->width + 1) / 2, (info->width + 0) / 2,
            (info->width + 0) / 1
        };
        size_t passh[7] = {
            (info->height + 7) / 8, (info->height + 7) / 8, (info->height + 3) / 8,
            (info->height + 3) / 4, (info->height + 1) / 4, (info->height + 1) / 2,
            (info->height + 0) / 2
        };
        size_t passstart[7] = { 0 };
        size_t pattern[28] = { 0, 4, 0, 2, 0, 1, 0, 0, 0, 4, 0, 2, 0, 1, 8, 8, 4, 4, 2, 2, 1, 8, 8,
            8, 4, 4, 2, 2 }; // values for the adam7 passes

        karray_init(&arr_scanlineo, sizeof(uint8_t));
        karray_init(&arr_scanlinen, sizeof(uint8_t));

        for (i = 0; i < 6; i++)
            passstart[i + 1] = passstart[i] + passh[i] * ((passw[i] ? 1 : 0) + (passw[i] * bpp + 7) / 8);

        if (karray_set_count(&arr_scanlineo, (info->width * bpp + 7) / 8, false)) {
            PNG_error = 1;
            goto out;
        }
        if (karray_set_count(&arr_scanlinen, (info->width * bpp + 7) / 8, false)) {
            PNG_error = 1;
            karray_free(&arr_scanlineo);
            goto out;
        }
        for (i = 0; i < 7; i++)
            PNG_adam7Pass(
                        out_data,
                        karray_get_data(&arr_scanlinen),
                        karray_get_data(&arr_scanlineo),
                        &scanlines_data[passstart[i]],
                        info->width,
                        pattern[i],
                        pattern[i + 7],
                        pattern[i + 14],
                        pattern[i + 21],
                        passw[i],
                        passh[i],
                        bpp);

        karray_free(&arr_scanlineo);
        karray_free(&arr_scanlinen);
    }
    if (info->colorType != 6 || info->bitDepth != 8) { // conversion needed
        struct karray arr_copy;

        karray_init(&arr_copy, sizeof(uint8_t));

        if (!karray_copy(&arr_copy, karray_get_data(&info->arr_image),
                    karray_get_count(&info->arr_image))) {
            PNG_error = 1;
            goto out;
        }

        PNG_error = PNG_convert(info, &info->arr_image, karray_get_data(&arr_copy));
        if (PNG_error) {
            karray_free(&arr_copy);
            return PNG_error;
        }

        karray_free(&arr_copy);
    }

out:
    karray_free(&arr_idat);
    karray_free(&arr_scanlines);
    karray_free(&info->arr_palette);
    return PNG_error;
}

/*************************************************************************************************/

int loadEmbeddedPngImage(uint8_t *pngData, int pngSize, png_pix_info_t *dinfo)
{
    PNG_info_t info;
    int error = 0;

    if (dinfo == NULL)
        return OS_RETURN_ERROR;

    error = PNG_decode(&info, pngData, pngSize);
    if (error != 0) {
        return OS_RETURN_ERROR;
    } else if ((info.width > 0xffff) || (info.height > 0xffff)) {
        return OS_RETURN_ERROR;
    } else if ((info.width * info.height * 4)
               != karray_get_count(&info.arr_image)) {
        return OS_RETURN_ERROR;
    }

    dinfo->width = info.width;
    dinfo->height = info.height;
    os_memcpy(&dinfo->arr_image, &info.arr_image, sizeof(dinfo->arr_image));

    return OS_RETURN_SUCCESS;
}

