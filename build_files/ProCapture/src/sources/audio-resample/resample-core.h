#pragma once

#if (defined(__linux) || defined(__linux__) || defined(__gnu_linux__))
#include <linux/types.h>
#else
typedef unsigned int uint;
#endif

typedef struct state_t {
	int up;
	int dn;
	int nchans;
	int nwing;
	int nhist;
	int nstart;
	int offset;
	uint phasef;
	uint stepf;
	short *histbuf;
	short *filter;
	short *rwing;
	short *lwing;
	short *stepNptr;
	int stepNfwd[3];
	int stepNbak[3];
	short *step1ptr;
	int step1fwd[3];
	int step1bak[3];
    short *(*ResampleCore)(short *, short *, short *, struct state_t *);
} state_t;

/* core functions */
short *ARBCoreNChans(short *pcmptr, short *pcmend, short *outptr, state_t *s);
short *RATCoreNChans(short *pcmptr, short *pcmend, short *outptr, state_t *s);
