////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "math.h"
#include "color-matrix.h"

// CIE xyz colorspace
// x = X / (X + Y + Z)
// y = Y / (X + Y + Z)
// z = Z / (X + Y + Z) = 1 - x - y

// Colorspace definition (RGB)
// * Three primaries of (x, y) tuple, defines color space vector (1, 0, 0), (0, 1, 0), (0, 0, 1)
// * White point (x, y) tuple, defines (1, 1, 1)
// * Transfer function from linear to non-linear color representaion
// * May define RGB->YUV encoding

// NOTE
// * YUV is not a colorspace, but just a different way of representing colors.
// * All colorspace can represent all colors, but after quantization & saturation, some colors < 0 or > 1 are cut off.
// * Colorspace are linear, but CRT display are not linear, so a transfer function is used to go from linear to a non-linear color representation.
// * NOTE: OpenGL use linear RGB by default.

// Color spaces:
// BT.470 (BT.601 PAL/SECAM):				R (0.640, 0.330), G (0.290, 0.600), B (0.150, 0.060), W (0.31271, 0.32902), V = 1.099 * exp(L, 0.45) - 0.099 (L >= 0.018) or V = 4.500 * L (L < 0.018), BT.601 YCbCr encoding
// SMPTE 170M, SMPTE RP 145 (BT.601 NTSC):	R (0.630, 0.340), G (0.310, 0.595), B (0.155, 0.070), W (0.31271, 0.32902), V = 1.099 * exp(L, 0.45) - 0.099 (L >= 0.018) or V = 4.500 * L (L < 0.018), BT.601 YCbCr encoding
// BT.709:									R (0.640, 0.330), G (0.300, 0.600), B (0.150, 0.060), W (0.31271, 0.32902), V = 1.099 * exp(L, 0.45) - 0.099 (L >= 0.018) or V = 4.500 * L (L < 0.018), BT.709 YCbCr encoding
// BT.2020:									R (0.708, 0.292), G (0.170, 0.797), B (0.131, 0.046), W (0.31271, 0.32902), V = 1.0993 * exp(L, 0.45) - 0.0993 (L >= 0.0181) or V = 4.500 * L (L < 0.0181), BT.2020 YCbCr encoding
// sRGB:									R (0.640, 0.330), G (0.300, 0.600), B (0.150, 0.060), W (0.31271, 0.32902), BT.601 YCbCr encoding based
// AdobeRGB:								R (0.640, 0.330), G (0.210, 0.710), B (0.150, 0.060), W (0.31271, 0.32902), Gamma = 2.19921875, BT.601 YCbCr encoding
// Adobe Wide Gamut RGB:					R (0.735, 0.265), G (0.115, 0.826), B (0.157, 0.018), W (0.34567, 0.35850), Gamma = 2.19921875

// Video sources:
// RGB: CIE XYZ => * MATRIX_XYZ -> f_transfer() -> Quantization() -> Saturation() => Non-linear RGB
// YUV: CIE XYZ => * MATRIX_XYZ -> f_transfer() -> * MATRIX_YUV -> Quantization() -> Saturation() => Non-linear YUV

void clear_matrix(matrix_t matrix)
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == j)
                matrix[i][j] = 1 * ACCURACY_MAGIC;
            else
                matrix[i][j] = 0;
		}
	}
}

void multiply_matrix(matrix_t dest, matrix_t src)
{
    int dest_row, col, src_row;
    long long af_row[4];
    long long value;
	
	for (dest_row = 0; dest_row < 4; dest_row++) {
		for (col = 0; col < 4; col++) {
			af_row[col] = dest[dest_row][col];
		}

		for (col = 0; col < 4; col++) {
            value = 0;
			for (src_row = 0; src_row < 4; src_row++) {
                value += MAGIC_MULTI(src[src_row][col], af_row[src_row]);
			}
			dest[dest_row][col] = value;
		}
	}
}

void rgb_set_saturation(matrix_t matrix, short saturation)
{
    long long value, lumr, lumg, lumb, value2;
    saturation = _limit(saturation, -100, 100);
    if (saturation == 0)
        return;

    value = ACCURACY_MAGIC + saturation * (ACCURACY_MAGIC / 100);
    value2 = 1 * ACCURACY_MAGIC - value;
    lumr = 3086LL * (ACCURACY_MAGIC / 10000);
    lumg = 6094LL * (ACCURACY_MAGIC / 10000);
    lumb = 820LL * (ACCURACY_MAGIC / 10000);

    {
        matrix_t saturation_matrix = {
            {MAGIC_MULTI(lumr, value2) + value, MAGIC_MULTI(lumg, value2), MAGIC_MULTI(lumb, value2), 0},
            {MAGIC_MULTI(lumr, value2), MAGIC_MULTI(lumg, value2) + value, MAGIC_MULTI(lumb, value2), 0},
            {MAGIC_MULTI(lumr, value2), MAGIC_MULTI(lumg, value2), MAGIC_MULTI(lumb, value2) + value, 0},
            {0, 0, 0, 1 * ACCURACY_MAGIC}
        };

        multiply_matrix(matrix, saturation_matrix);
    }
}

void rgb_set_hue(matrix_t matrix, short  hue)
{
    long long cos_val, sin_val;
    long long lumr, lumg, lumb;

    hue = _limit(hue, -180, 180);

    cos_val = (long long)i_cos(hue) * (ACCURACY_MAGIC / 10000);
    sin_val = (long long)i_sin(hue) * (ACCURACY_MAGIC / 10000);
    lumr = MAGIC_FLOAT(213, 1000);
    lumg = MAGIC_FLOAT(715, 1000);
    lumb = MAGIC_FLOAT(72, 1000);

    {
        matrix_t saturation_matrix = {
            {
                lumr + MAGIC_MULTI(cos_val, ACCURACY_MAGIC - lumr) + MAGIC_MULTI(sin_val, -lumr),
                lumg + MAGIC_MULTI(cos_val, -lumg) + MAGIC_MULTI(sin_val, -lumg),
                lumb + MAGIC_MULTI(cos_val, -lumb) + MAGIC_MULTI(sin_val, -lumb),
                0,
            },
            {
                lumr + MAGIC_MULTI(cos_val, -lumr) + MAGIC_MULTI(sin_val, MAGIC_FLOAT(143, 1000)),
                lumg + MAGIC_MULTI(cos_val, ACCURACY_MAGIC - lumg) + MAGIC_MULTI(sin_val, MAGIC_FLOAT(140, 1000)),
                lumb + MAGIC_MULTI(cos_val, -lumb) + MAGIC_MULTI(sin_val, MAGIC_FLOAT(-283, 1000)),
                0,
            },
            {
                lumr + MAGIC_MULTI(cos_val, -lumr) + MAGIC_MULTI(sin_val, (-(ACCURACY_MAGIC - lumr))),
                lumg + MAGIC_MULTI(cos_val, -lumg) + MAGIC_MULTI(sin_val, lumg),
                lumb + MAGIC_MULTI(cos_val, ACCURACY_MAGIC - lumb) + MAGIC_MULTI(sin_val, lumb),
                0
            },
            { 0, 0, 0, 1 * ACCURACY_MAGIC },
        };

        multiply_matrix(matrix, saturation_matrix);
    }
}

void rgb_set_contrast(matrix_t mdest, short contrast_r, short contrast_g, short contrast_b)
{
    long long valuer, valueg, valueb;

    contrast_r = _limit(contrast_r, -100, 100);
    contrast_g = _limit(contrast_g, -100, 100);
    contrast_b = _limit(contrast_b, -100, 100);
    if (0 == contrast_r && 0 == contrast_g && 0 == contrast_b)
        return;

    valuer = ACCURACY_MAGIC + (ACCURACY_MAGIC / 100) * contrast_r;
    valueg = ACCURACY_MAGIC + (ACCURACY_MAGIC / 100) * contrast_g;
    valueb = ACCURACY_MAGIC + (ACCURACY_MAGIC / 100) * contrast_b;

    {
        matrix_t contrast_matrix = {
            {valuer, 0, 0, 0},
            {0, valueg, 0, 0},
            {0, 0, valueb, 0},
            {0, 0, 0, ACCURACY_MAGIC}
        };

        multiply_matrix(mdest, contrast_matrix);
    }
}

void rgb_set_brightness(matrix_t mdest, short brightness_r, short brightness_g, short brightness_b)
{
    if (0 == brightness_r && brightness_g == 0 && brightness_b == 0)
        return;

    {
        matrix_t brightness_matrix = {
            { ACCURACY_MAGIC, 0, 0, MAGIC_FLOAT(brightness_r, 1) },
            { 0, ACCURACY_MAGIC, 0, MAGIC_FLOAT(brightness_g, 1) },
            { 0, 0, ACCURACY_MAGIC, MAGIC_FLOAT(brightness_b, 1) },
            { 0, 0, 0, ACCURACY_MAGIC}
        };

        multiply_matrix(mdest, brightness_matrix);
    }
}

void rgb_set_scale_offset(matrix_t mdest,
        short rscale, short roffset,
        short gscale, short goffset,
        short bscale, short boffset
        )
{
    matrix_t scale_offset_matrix = {
        { MAGIC_FLOAT(rscale, 1), 0, 0, MAGIC_FLOAT(roffset, 1) },
        { 0, MAGIC_FLOAT(gscale, 1), 0, MAGIC_FLOAT(goffset, 1) },
        { 0, 0, MAGIC_FLOAT(bscale, 1), MAGIC_FLOAT(boffset, 1) },
        { 0, 0, 0, ACCURACY_MAGIC}
    };

    multiply_matrix(mdest, scale_offset_matrix);
}

void yuv_set_sat_hue(matrix_t mdest, short saturation, short hue, int colordepth)
{
    long long lsat, lc1, lc2;
    short addon_mul = 128 << (colordepth - 8);

    saturation = _limit(saturation, -100, 100);
    hue = _limit(hue, -180, 180);
    if (0 == saturation && 0 == hue)
        return;

    lsat = ACCURACY_MAGIC + saturation * (ACCURACY_MAGIC / 100);
    lc1 = os_div64(i_cos(hue) * lsat, 10000);
    lc2 = os_div64(i_sin(hue) * lsat, 10000);

    {
        matrix_t hue_matrix = {
            {ACCURACY_MAGIC, 0, 0, 0},
            {0, lc1, lc2, addon_mul * (ACCURACY_MAGIC - lc1 - lc2)},
            {0, -lc2, lc1, addon_mul * (ACCURACY_MAGIC - lc1 + lc2)},
            {0, 0, 0, ACCURACY_MAGIC}
        };

        multiply_matrix(mdest, hue_matrix);
    }
}

void yuv_set_contrast(matrix_t mdest, short contrast, int colordepth)
{
    long long laddon, lvalue;
    short addon_mul = 128 << (colordepth - 8);

    contrast = _limit(contrast, -100, 100);
    if (0 == contrast)
        return;

    lvalue = ACCURACY_MAGIC + (long long)contrast * (ACCURACY_MAGIC / 100);
    laddon = addon_mul * (ACCURACY_MAGIC - lvalue);

    {
        matrix_t contrast_matrix = {
            {lvalue, 0, 0, laddon},
            {0, lvalue, 0, laddon},
            {0, 0, lvalue, laddon},
            {0, 0, 0, ACCURACY_MAGIC}
        };

        multiply_matrix(mdest, contrast_matrix);
    }
}

void yuv_set_brightness(matrix_t mdest, short brightness)
{
    if (0 == brightness)
        return;

    {
        matrix_t brightness_matrix = {
            { ACCURACY_MAGIC, 0, 0, (long long)brightness * ACCURACY_MAGIC},
            { 0, ACCURACY_MAGIC, 0, 0 },
            { 0, 0, ACCURACY_MAGIC, 0 },
            { 0, 0, 0, ACCURACY_MAGIC },
        };

        multiply_matrix(mdest, brightness_matrix);
    }
}

void yuv_set_scale_offset(matrix_t mdest,
        short yscale, short yoffset,
        short uscale, short uoffset,
        short vscale, short voffset
        )
{
    matrix_t scale_offset_matrix = {
        { MAGIC_FLOAT(yscale, 1), 0, 0, MAGIC_FLOAT(yoffset, 1) },
        { 0, MAGIC_FLOAT(uscale, 1), 0, MAGIC_FLOAT(uoffset, 1) },
        { 0, 0, MAGIC_FLOAT(vscale, 1), MAGIC_FLOAT(voffset, 1) },
        { 0, 0, 0, ACCURACY_MAGIC}
    };

    multiply_matrix(mdest, scale_offset_matrix);
}

static void RGBNormalization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qr, int nColorDepth)
{
	int nMin = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (16 << (nColorDepth - 8)) : 0;
	int nMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (235 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	int nRange = (nMax - nMin);
	long long fScale = ACCURACY_MAGIC / nRange;
	long long fAddOn = MAGIC_FLOAT_LL(-nMin, nRange);

    {
        matrix_t afNormalizationMatrix = {
            { fScale,	0,		0,		fAddOn	},
            { 0,		fScale,	0,		fAddOn	},
            { 0,		0,		fScale,	fAddOn	},
            { 0,		0,		0,		ACCURACY_MAGIC	}
        };

        multiply_matrix(matrixDest, afNormalizationMatrix);
    }
}

static void RGBQuantization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qr, int nColorDepth)
{
	int nMin = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (16 << (nColorDepth - 8)) : 0;
	int nMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (235 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	long long fScale = (nMax - nMin) * ACCURACY_MAGIC;
	long long fAddOn = nMin * ACCURACY_MAGIC;

    {
        matrix_t afQuantizationMatrix = {
            { fScale,	0,		0,		fAddOn	},
            { 0,		fScale,	0,		fAddOn	},
            { 0,		0,		fScale,	fAddOn	},
            { 0,		0,		0,		ACCURACY_MAGIC	}
        };

        multiply_matrix(matrixDest, afQuantizationMatrix);
    }
}

static void YUVNormalization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qr, int nColorDepth)
{
	int nMin = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (16 << (nColorDepth - 8)) : 0;
	int nUVZero = (128 << (nColorDepth - 8));
	int nYMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (235 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	int nUVMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (240 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	int nYRange = (nYMax - nMin);
	int nUVRange = (nUVMax - nMin);
	long long fYScale = ACCURACY_MAGIC / nYRange;
	long long fYAddOn = MAGIC_FLOAT_LL(-nMin, nYRange);
	long long fUVScale = ACCURACY_MAGIC / nUVRange;
	long long fUVAddOn = MAGIC_FLOAT_LL(-nUVZero, nUVRange);

    {
        matrix_t afNormalizationMatrix = {
            { fYScale,	0,		    0,		    fYAddOn		},
            { 0,		fUVScale,	0,		    fUVAddOn	},
            { 0,		0,		    fUVScale,	fUVAddOn	},
            { 0,		0,		    0,		    ACCURACY_MAGIC		}
        };

        multiply_matrix(matrixDest, afNormalizationMatrix);
    }
}

static void YUVQuantization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qr, int nColorDepth)
{
	int nMin = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (16 << (nColorDepth - 8)) : 0;
	int nYMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (235 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	int nUVMax = (qr == MWCAP_VIDEO_QUANTIZATION_LIMITED) ? (240 << (nColorDepth - 8)) : ((1 << nColorDepth) - 1);
	long long fYScale = (nYMax - nMin) * ACCURACY_MAGIC;
	long long fUVScale = (nUVMax - nMin) * ACCURACY_MAGIC;
	long long fYAddOn = nMin * ACCURACY_MAGIC;
	long long fUVAddOn = (128 << (nColorDepth - 8)) * ACCURACY_MAGIC;

    {
        matrix_t afQuantizationMatrix = {
            { fYScale,	0,		    0,		    fYAddOn		},
            { 0,		fUVScale,	0,		    fUVAddOn	},
            { 0,		0,		    fUVScale,	fUVAddOn	},
            { 0,		0,		    0,		    ACCURACY_MAGIC		}
        };

        multiply_matrix(matrixDest, afQuantizationMatrix);
    }
}

static void RGBToYUV601(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
        { 299000,  587000,  114000, 0 },
        { -168736, -331264, 500000, 0 },
        { 500000,  -418688, -81312, 0 },
        { 0, 0, 0, ACCURACY_MAGIC },
	};

	multiply_matrix(matrixDest, afCSCMatrix);
}

static void YUV601ToRGB(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
		{ ACCURACY_MAGIC,		0,		   1402000,		0 },
		{ ACCURACY_MAGIC,		-344136,   -714136,	    0 },
		{ ACCURACY_MAGIC,		1772000,   0,		    0 },
		{ 0,		0,		0,		ACCURACY_MAGIC }
	};
	multiply_matrix(matrixDest, afCSCMatrix);
}

static void RGBToYUV709(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
		{	212600,	    715200,	    72200,	    0	},
		{	-114572,	-385428,	500000,		0	},
		{	500000,		-454153,	-45847,	    0	},
		{	0,		0,		0,		ACCURACY_MAGIC	}
	};
	multiply_matrix(matrixDest, afCSCMatrix);
}

static void YUV709ToRGB(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
		{ ACCURACY_MAGIC,		0,		    1574800,	0 },
		{ ACCURACY_MAGIC,		-187324,    -468124,	0 },
		{ ACCURACY_MAGIC,		1855600,	0,  		0 },
		{	0,		0,		0,		ACCURACY_MAGIC	}
	};
	multiply_matrix(matrixDest, afCSCMatrix);
}

// TODO:
// For constant luminance
// Y = 0.2627 * R + 0.6780 * G + 0.0593 * B
// U = (B - Y <= 0) ? (B - Y) / 1.9404 : (B - Y) / 1.5820
// V = (R - Y <= 0) ? (R - Y) / 1.7182 : (R - Y) / 0.9938
static void RGBToYUV2020(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
        { 262700,       678000,     59300,      0   },
        { -139630,      -360370,    500000,     0   },
        { 500000,       -459786,    -40214,     0   },
        { 0,            0,          0,          ACCURACY_MAGIC }
	};
	multiply_matrix(matrixDest, afCSCMatrix);
}

// TODO:
// For constant luminance
// R = (V <= 0) ? Y + 1.7182 * V : Y + 0.9938 * V
// G = Four group coeffs selected by U, V
// B = (U <= 0) ? Y + 1.9404 * U : Y + 1.5820 * U
static void YUV2020ToRGB(matrix_t matrixDest)
{
	matrix_t afCSCMatrix = {
        { ACCURACY_MAGIC,   0,        1474600,      0 },
        { ACCURACY_MAGIC,   -164550,  -571350,      0 },
        { ACCURACY_MAGIC,   1881400,  0,            0 },
        { 0,                0,        0,   ACCURACY_MAGIC}
	};
	multiply_matrix(matrixDest, afCSCMatrix);
}

//  YUV601 = (M_YUV_QUANT * M_RGB_YUV601 * M_RGB_NORMAL) * RGB
static void CSCSetRGBToYUV601(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV601(matrixDest);
	RGBNormalization(matrixDest, qrInput, nColorDepth);
}

// RGB = (M_RGB_QUANT * M_YUV601_RGB * M_YUV_NORMAL) * YUV;
static void CSCSetYUV601ToRGB(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	RGBQuantization(matrixDest, qrOutput, nColorDepth);
	YUV601ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

//  YUV709 = (M_YUV_QUANT * M_RGB_YUV709 * M_RGB_NORMAL) * RGB
static void CSCSetRGBToYUV709(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV709(matrixDest);
	RGBNormalization(matrixDest, qrInput, nColorDepth);
}

// RGB = (M_RGB_QUANT * M_YUV709_RGB * M_YUV_NORMAL) * YUV;
static void CSCSetYUV709ToRGB(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	RGBQuantization(matrixDest, qrOutput, nColorDepth);
	YUV709ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

//  YUV2020 = (M_YUV_QUANT * M_RGB_YUV2020 * M_RGB_NORMAL) * RGB
static void CSCSetRGBToYUV2020(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV2020(matrixDest);
	RGBNormalization(matrixDest, qrInput, nColorDepth);
}

// RGB = (M_RGB_QUANT * M_YUV2020_RGB * M_YUV_NORMAL) * YUV;
static void CSCSetYUV2020ToRGB(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	RGBQuantization(matrixDest, qrOutput, nColorDepth);
	YUV2020ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV709 = (M_YUV_QUANT * M_RGB_YUV709 * M_YUV601_RGB * M_YUV_NORMAL) * YUV601
static void CSCSetYUV601ToYUV709(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV709(matrixDest);
	YUV601ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV2020 = (M_YUV_QUANT * M_RGB_YUV2020 * M_YUV601_RGB * M_YUV_NORMAL) * YUV601
static void CSCSetYUV601ToYUV2020(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV2020(matrixDest);
	YUV601ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV601 = (M_YUV_QUANT * M_RGB_YUV601 * M_YUV709_RGB * M_YUV_NORMAL) * YUV709
static void CSCSetYUV709ToYUV601(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV601(matrixDest);
	YUV709ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV2020 = (M_YUV_QUANT * M_RGB_YUV2020 * M_YUV709_RGB * M_YUV_NORMAL) * YUV709
static void CSCSetYUV709ToYUV2020(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV2020(matrixDest);
	YUV709ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV601 = (M_YUV_QUANT * M_RGB_YUV601 * M_YUV2020_RGB * M_YUV_NORMAL) * YUV2020
static void CSCSetYUV2020ToYUV601(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV601(matrixDest);
	YUV2020ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV709 = (M_YUV_QUANT * M_RGB_YUV709 * M_YUV2020_RGB * M_YUV_NORMAL) * YUV2020
static void CSCSetYUV2020ToYUV709(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	RGBToYUV709(matrixDest);
	YUV2020ToRGB(matrixDest);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

// RGB = (M_RGB_QUANT * M_RGB_NORMAL) * RGB
static void CSCSetRGBQuantization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	RGBQuantization(matrixDest, qrOutput, nColorDepth);
	RGBNormalization(matrixDest, qrInput, nColorDepth);
}

// YUV = (M_YUV_QUANT * M_YUV_NORMAL) * YUV
static void CSCSetYUVQuantization(matrix_t matrixDest, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	YUVQuantization(matrixDest, qrOutput, nColorDepth);
	YUVNormalization(matrixDest, qrInput, nColorDepth);
}

void csc_set_conversion(matrix_t matrixDest, MWCAP_VIDEO_COLOR_FORMAT csInput, MWCAP_VIDEO_QUANTIZATION_RANGE qrInput,
        MWCAP_VIDEO_COLOR_FORMAT csOutput, MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int nColorDepth)
{
	if (csInput == csOutput && qrInput == qrOutput)
		return;

	switch (csInput) {
	case MWCAP_VIDEO_COLOR_FORMAT_RGB:
		switch (csOutput) {
		case MWCAP_VIDEO_COLOR_FORMAT_RGB:
			CSCSetRGBQuantization(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
			CSCSetRGBToYUV601(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
			CSCSetRGBToYUV709(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
			CSCSetRGBToYUV2020(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
        default:
            break;
		}
		break;

	case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
		switch (csOutput) {
		case MWCAP_VIDEO_COLOR_FORMAT_RGB:
			CSCSetYUV601ToRGB(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
			CSCSetYUVQuantization(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
			CSCSetYUV601ToYUV709(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
			CSCSetYUV601ToYUV2020(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
        default:
            break;
		}
		break;

	case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
		switch (csOutput) {
		case MWCAP_VIDEO_COLOR_FORMAT_RGB:
			CSCSetYUV709ToRGB(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
			CSCSetYUV709ToYUV601(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
			CSCSetYUVQuantization(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
			CSCSetYUV709ToYUV2020(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
        default:
            break;
		}
		break;

	case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
		switch (csOutput) {
		case MWCAP_VIDEO_COLOR_FORMAT_RGB:
			CSCSetYUV2020ToRGB(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV601:
			CSCSetYUV2020ToYUV601(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV709:
			CSCSetYUV2020ToYUV709(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
		case MWCAP_VIDEO_COLOR_FORMAT_YUV2020:
			CSCSetYUVQuantization(matrixDest, qrInput, qrOutput, nColorDepth);
			break;
        default:
            break;
		}
		break;
    default:
        break;
	}
}

void get_color_matrix_params(matrix_t matrix,
        MWCAP_VIDEO_COLOR_FORMAT csOutput,
        MWCAP_VIDEO_SATURATION_RANGE srOutput,
        int nColorDepth,
        int coeff_frac_bits,
        const int *pinput_order,
        const int *poutput_order,
        color_matrix_params_t *params)
{
	// Calc min/max value
	u16 wMin = 0, wMaxRGBY = 0, wMaxUV = 0;
    short coeff_mul = (1 << coeff_frac_bits);
    // Saturation
    matrix_t matrixSat;
    int row, col;

    for (row = 0; row < 3; row++) {
        for (col = 0; col < 3; col++) {
            if (matrix[row][col] < -8000000)
                matrixSat[row][col] = -8000000;
            else if (matrix[row][col] > 7000000)
                matrixSat[row][col] = 7000000;
            else
                matrixSat[row][col] = matrix[row][col];
        }

        if (matrix[row][3] < -2048000000)
            matrixSat[row][3] = -2048000000;
        else if (matrix[row][3] > 2047000000)
            matrixSat[row][3] = 2047000000;
        else
            matrixSat[row][3] = matrix[row][3];
    }

    params->ch0.coeff0 = (short)((int)(matrixSat[poutput_order[0]][pinput_order[0]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch0.coeff1 = (short)((int)(matrixSat[poutput_order[0]][pinput_order[1]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch0.coeff2 = (short)((int)(matrixSat[poutput_order[0]][pinput_order[2]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch0.addon  = (short)((int)(matrixSat[poutput_order[0]][3]) / ACCURACY_MAGIC);

    params->ch1.coeff0 = (short)((int)(matrixSat[poutput_order[1]][pinput_order[0]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch1.coeff1 = (short)((int)(matrixSat[poutput_order[1]][pinput_order[1]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch1.coeff2 = (short)((int)(matrixSat[poutput_order[1]][pinput_order[2]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch1.addon  = (short)((int)(matrixSat[poutput_order[1]][3]) / ACCURACY_MAGIC);

    params->ch2.coeff0 = (short)((int)(matrixSat[poutput_order[2]][pinput_order[0]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch2.coeff1 = (short)((int)(matrixSat[poutput_order[2]][pinput_order[1]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch2.coeff2 = (short)((int)(matrixSat[poutput_order[2]][pinput_order[2]] * coeff_mul) / ACCURACY_MAGIC);
    params->ch2.addon  = (short)((int)(matrixSat[poutput_order[2]][3]) / ACCURACY_MAGIC);

	switch (srOutput) {
	case MWCAP_VIDEO_SATURATION_FULL:
		wMin = 0;
		wMaxRGBY = wMaxUV = ((1 << nColorDepth) - 1);
		break;

	case MWCAP_VIDEO_SATURATION_LIMITED:
		wMin = (16 << (nColorDepth - 8));
		wMaxRGBY = (235 << (nColorDepth - 8));
		wMaxUV = (240 << (nColorDepth - 8));
		break;

	case MWCAP_VIDEO_SATURATION_EXTENDED_GAMUT:
		wMin = (1 << (nColorDepth - 8));
		wMaxRGBY = wMaxUV = (255 << (nColorDepth - 8)) - 1;
		break;

    default:
        break;
	}

	params->ch0.min = wMin;
	params->ch1.min = wMin;
	params->ch2.min = wMin;

	if (csOutput == MWCAP_VIDEO_COLOR_FORMAT_RGB) {
		params->ch0.max = wMaxRGBY;
		params->ch1.max = wMaxRGBY;
		params->ch2.max = wMaxRGBY;
	}
	else {
		params->ch0.max = (poutput_order[0] == COLOR_MATRIX_CH_Y) ? wMaxRGBY : wMaxUV;
		params->ch1.max = (poutput_order[1] == COLOR_MATRIX_CH_Y) ? wMaxRGBY : wMaxUV;
		params->ch2.max = (poutput_order[2] == COLOR_MATRIX_CH_Y) ? wMaxRGBY : wMaxUV;
	}
}

