////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __COLOR_MATRIX_H__
#define __COLOR_MATRIX_H__

#include "ospi/ospi.h"

#include "mw-procapture-extension.h"

#define ACCURACY_MAGIC 1000000
#define MAGIC_MULTI(a, b) os_div64((long long)(a) * (b), ACCURACY_MAGIC)
#define MAGIC_FLOAT(numerator, denominator) ((long long)numerator * (ACCURACY_MAGIC / denominator))
#define MAGIC_FLOAT_LL(numerator, denominator) os_div64((long long)numerator * ACCURACY_MAGIC, denominator)

typedef long long matrix_t[4][4];

#define COLOR_MATRIX_CH_Y       0
#define COLOR_MATRIX_CH_U       1
#define COLOR_MATRIX_CH_V       2
#define COLOR_MATRIX_CH_R       0
#define COLOR_MATRIX_CH_G       1
#define COLOR_MATRIX_CH_B       2

typedef struct _color_channel_params {
    short coeff0;
    short coeff1;
    short coeff2;
    short addon;
    unsigned int min;
    unsigned int max;
} color_channel_params_t;

typedef struct  _color_matrix_params {
    color_channel_params_t ch0;
    color_channel_params_t ch1;
    color_channel_params_t ch2;
} color_matrix_params_t;

void clear_matrix(matrix_t matrix);

void multiply_matrix(matrix_t dest, matrix_t src);

void rgb_set_saturation(matrix_t matrix, short saturation);
void rgb_set_hue(matrix_t matrix, short  hue);
void rgb_set_contrast(matrix_t mdest, short contrast_r, short contrast_g, short contrast_b);
void rgb_set_brightness(matrix_t mdest, short brightness_r, short brightness_g, short brightness_b);
void rgb_set_scale_offset(matrix_t mdest,
        short rscale, short roffset,
        short gscale, short goffset,
        short bscale, short boffset
        );

void yuv_set_sat_hue(matrix_t mdest, short saturation, short hue, int colordepth);
void yuv_set_contrast(matrix_t mdest, short contrast, int colordepth);
void yuv_set_brightness(matrix_t mdest, short brightness);
void yuv_set_scale_offset(matrix_t mdest,
        short yscale, short yoffset,
        short uscale, short uoffset,
        short vscale, short voffset
        );

void csc_set_conversion(matrix_t mdest, MWCAP_VIDEO_COLOR_FORMAT cs_input,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrInput, MWCAP_VIDEO_COLOR_FORMAT cs_output,
        MWCAP_VIDEO_QUANTIZATION_RANGE qrOutput, int colordepth);

void get_color_matrix_params(matrix_t matrix,
        MWCAP_VIDEO_COLOR_FORMAT csOutput,
        MWCAP_VIDEO_SATURATION_RANGE srOutput,
        int nColorDepth,
        int coeff_frac_bits,
        const int *pinput_order,
        const int *poutput_order,
        color_matrix_params_t *params);

#endif /* __COLOR_MATRIX_H__ */

