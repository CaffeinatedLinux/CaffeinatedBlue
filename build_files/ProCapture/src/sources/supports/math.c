////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

static const int g_sin_table[] = {
	0,
	175,
	349,
	523,
	698,
	872,
	1045,
	1219,
	1392,
	1564,
	1736,
	1908,
	2079,
	2250,
	2419,
	2588,
	2756,
	2924,
	3090,
	3256,
	3420,
	3584,
	3746,
	3907,
	4067,
	4226,
	4384,
	4540,
	4695,
	4848,
	5000,
	5150,
	5299,
	5446,
	5592,
	5736,
	5878,
	6018,
	6157,
	6293,
	6428,
	6561,
	6691,
	6820,
	6947,
	7071,
	7193,
	7314,
	7431,
	7547,
	7660,
	7771,
	7880,
	7986,
	8090,
	8192,
	8290,
	8387,
	8480,
	8572,
	8660,
	8746,
	8829,
	8910,
	8988,
	9063,
	9135,
	9205,
	9272,
	9336,
	9397,
	9455,
	9511,
	9563,
	9613,
	9659,
	9703,
	9744,
	9781,
	9816,
	9848,
	9877,
	9903,
	9925,
	9945,
	9962,
	9976,
	9986,
	9994,
	9998,
	10000
};

int i_cos(short angle)
{
	angle = angle % 360;

	if (angle < 0)
		return i_cos(360 + angle);
	else if (angle <= 90)
		return g_sin_table[90 - angle];
	else if (angle <= 180)
		return -i_cos(180 - angle);
	else if (angle <= 270)
		return -i_cos(angle - 180);
	else
		return i_cos(360 - angle);
}

int i_sin(short angle)
{
	angle = angle % 360;

	if (angle < 0)
		return i_sin(360 + angle);
	else if (angle <= 90)
		return g_sin_table[angle];
	else if (angle <= 180)
		return i_sin(180 - angle);
	else if (angle <= 270)
		return -i_sin(angle - 180);
	else
		return -i_sin(360 - angle);
}

int GCD(int a, int b)
{
	if (b == 0)
		return a;
	else
		return GCD(b, a % b);
}

