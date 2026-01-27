////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "ospi/ospi.h"
#include "sha1.h"

typedef u32 DWORD;
typedef u8  BYTE;
typedef void  VOID;

// NLF - INTERNAL FUNCTION
// Purpose:	To perform a minor calculation of the SHA-1 algorithm
// Inputs:	Three DWORD which are numbers for the calculation, and an BYTE which
//				is used to tell which calculation to perform
// Outputs:	An DWORD which is the result of the performed calculation
DWORD NLF(DWORD b, DWORD c, DWORD d, BYTE num)
{
	DWORD ret;

	if (num < 20)
		ret = ((b & c) | ((~b) & d));
	else if (num < 40)
		ret = (b ^ c ^ d);
	else if (num < 60)
		ret = ((b & c) | (b & d) | (c & d));
	else
		ret = (b ^ c ^ d);

	return ret;
}

// KTN - INTERNAL FUNTION
// Purpose:	To return a contant based on the input of the function, used for the
//				SHA-1 algorithm
// Inputs:	An BYTE which is used to decide what number to return
// Outputs:	An DWORD which is one of four constants
DWORD KTN(BYTE num)
{
	DWORD ret;

	if (num < 20)
		ret = 0x5A827999;
	else if (num < 40)
		ret = 0x6ED9EBA1;
	else if (num < 60)
		ret = 0x8F1BBCDC;
	else
		ret = 0xCA62C1D6;

	return ret;
}

DWORD make32(BYTE a, BYTE b, BYTE c, BYTE d) 
{
	return (a << 24) | (b << 16) | (c << 8) | d;
}

BYTE make8(DWORD a, int i)
{
	return (a >> (i * 8)) & 0xFF;
}

// SHA1_MAC
// Purpose:	To use SHA-1 calculation to generate a message authentication code (MAC)
// Inputs:	A pointer to an array containing the 64-byte input data
// Outputs: a pointer to an array in which the 20 byte MAC will be placed
VOID SHA1_MAC(const BYTE* pbData, BYTE* pbMAC)
{
	DWORD hash[5];
	BYTE i;

	DWORD MTword[80];
	DWORD ShftTmp;
	DWORD Temp;


	// {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
	hash[0] = 0x67000000;
	hash[1] = 0xEF000000;
	hash[2] = 0x98000000;
	hash[3] = 0x10000000;
	hash[4] = 0xC3000000;

	hash[0] = hash[0] | 0x01;
	hash[1] = hash[1] | 0x89;
	hash[2] = hash[2] | 0xFE;
	hash[3] = hash[3] | 0x76;
	hash[4] = hash[4] | 0xF0;

	hash[0] = hash[0] | 0x2300;
	hash[1] = hash[1] | 0xAB00;
	hash[2] = hash[2] | 0xDC00;
	hash[3] = hash[3] | 0x5400;
	hash[4] = hash[4] | 0xE100;

	hash[0] = hash[0] | 0x450000;
	hash[1] = hash[1] | 0xCD0000;
	hash[2] = hash[2] | 0xBA0000;
	hash[3] = hash[3] | 0x320000;
	hash[4] = hash[4] | 0xD20000;

	for (i=0; i < 16; i++) {
		MTword[i] = (make32(*(pbData +  (i * 4)), *(pbData +  (i * 4) + 1), *(pbData +  (i * 4) + 2), *(pbData +  (i * 4) + 3)));
	}

	for (; i < 80; i++) {
		ShftTmp = (MTword[i-3] ^ MTword[i-8] ^ MTword[i-14] ^ MTword[i-16]);
		MTword[i] = (((ShftTmp << 1) & 0xFFFFFFFE) | ((ShftTmp >> 31) & 0x00000001));
	}

	for (i = 0; i < 80; i++) {
		ShftTmp = (((hash[0] << 5) & 0xFFFFFFE0) | ((hash[0] >> 27) & 0x0000001F));
		Temp = (NLF(hash[1],hash[2],hash[3],i) + hash[4] + KTN(i) + MTword[i] + ShftTmp);
		hash[4] = hash[3];
		hash[3] = hash[2];
		hash[2] = (((hash[1] << 30) & 0xC0000000) | ((hash[1] >> 2) & 0x3FFFFFFF));
		hash[1] = hash[0];
		hash[0] = Temp;
	}

	for (i = 5; i > 0; i--) {
		*pbMAC++ = make8(hash[i - 1], 0);
		*pbMAC++ = make8(hash[i - 1], 1);
		*pbMAC++ = make8(hash[i - 1], 2);
		*pbMAC++ = make8(hash[i - 1], 3);
	}
}
