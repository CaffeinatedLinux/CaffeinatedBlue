/*
 * Fixed-point sampling rate conversion library
 * Developed by Ken Cooke (kenc@real.com)
 * May 2003
 *
 * Core filtering functions, generic version.
 * This version is fairly optimized for ARMv4.
 */

#include "resample-core.h"
#define __int64 long long

/* no clear improvement */
//#define _OPT_
#if defined (_OPT_)
#if 0
__inline int MULHI(int a, int b) {
	__asm mov	eax, a
	__asm imul	b
	__asm mov	eax, edx
}
#endif
static inline int MULHI(int a, int b) {   
    int low;
    int high;
    asm ("SMULL %0, %1, %2, %3\n"
            :"=&r"(low), "=&r"(high)
            :"%r"(a), "r"(b)
            :"cc");   
    return high;
} 
#else
/* a fast MULHI is important, so verify this compiles well */
#define MULHI(a,b) ((int)(((__int64)(a) * (__int64)(b)) >> 32))
#endif

short *
RATCoreNChans(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
	short *rwgptr, *lwgptr, *revptr;
    short nchans = s->nchans;
    int acc[nchans];
    int i, j, sign;
	int *tab;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	pcmptr += s->offset;

    while (pcmptr+(nchans-1) < pcmend) {

        revptr = pcmptr - 1;
        for (i = 0; i < nchans; i++)
            acc[i] = 1 << 14;

		/* FIR filter */
        for (j = s->nwing >> 1; j != 0; j--) {
            for (i = 0; i < nchans; i++) {
                acc[i] += (*pcmptr++) * (*lwgptr);
            }
            lwgptr++;
            for (i = 0; i < nchans; i++) {
                acc[i] += (*pcmptr++) * (*lwgptr);
            }
            lwgptr++;
            for (i = nchans-1; i >= 0; i--) {
                acc[i] += (*revptr--) * (*rwgptr);
            }
            rwgptr++;
            for (i = nchans-1; i >= 0; i--) {
                acc[i] += (*revptr--) * (*rwgptr);
            }
            rwgptr++;
		}
        if (s->nwing & 0x1) {
            for (i = 0; i < nchans; i++) {
                acc[i] += (*pcmptr++) * (*lwgptr);
            }
            lwgptr++;
            for (i = nchans-1; i >= 0; i--) {
                acc[i] += (*revptr--) * (*rwgptr);
            }
            rwgptr++;
        }
        for (i = 0; i < nchans; i++) {
            acc[i] >>= 15;
        }

        /* saturate */
        for (i = 0; i < nchans; i++) {
            if ((sign = (acc[i] >> 31)) != (acc[i] >> 15))
                acc[i] = sign ^ ((1<<15)-1);
        }

        for (i = 0; i < nchans; i++)
            *outptr++ = (short) acc[i];

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];
	}

	s->offset = (int)(pcmptr - pcmend);
	s->rwing = rwgptr;
	s->lwing = lwgptr;

	return outptr;
}

short *
ARBCoreNChans(short *pcmptr, short *pcmend, short *outptr, state_t *s)
{
    short nchans = s->nchans;
	register short *rwgptr, *lwgptr, *revptr;
    register short *rwgptr1, *lwgptr1;
    int acc0[nchans];
    int acc1[nchans];
    int pcmstep, i, j, sign;
	int *tab;
	uint phasef;

	rwgptr = s->rwing;
	lwgptr = s->lwing;
	phasef = s->phasef;
	pcmptr += s->offset;

	/* phase+1 */
	tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
	rwgptr1 = rwgptr + tab[0];
	lwgptr1 = lwgptr + tab[1];
	pcmstep = tab[2];

    while (pcmptr+pcmstep+nchans-1 < pcmend) {

        revptr = pcmptr - 1;
        for (i = 0; i < nchans; i++) {
            acc0[i] = 1 << 14;
            acc1[i] = 1 << 14;
        }

		if (!pcmstep) {

            for (j = s->nwing; j != 0; j--) {
                short pcm[nchans];

                for (i = 0; i < nchans; i++) {
                    pcm[i] = (*pcmptr++);
                }

                for (i = 0; i < nchans; i++) {
                    acc0[i] += pcm[i] * (*lwgptr);
                    acc1[i] += pcm[i] * (*lwgptr1);
                }
                lwgptr++;
                lwgptr1++;

                for (i = nchans-1; i >= 0; i--) {
                    pcm[i] = (*revptr--);
                }

                for (i = nchans-1; i >= 0; i--) {
                    acc1[i] += pcm[i] * (*rwgptr1);
                    acc0[i] += pcm[i] * (*rwgptr);
                }
                rwgptr1++;
                rwgptr++;
			}

		} else {

            for (j = s->nwing; j != 0; j--) {

                for (i = 0; i < nchans; i++) {
                    acc0[i] += (*pcmptr++) * (*lwgptr);
                }
                lwgptr++;
                for (i = 0; i < nchans; i++) {
                    acc1[i] += *(pcmptr+i) * (*lwgptr1);
                }
                lwgptr1++;

                for (i = nchans-1; i >= 0; i--) {
                    acc1[i] += *(revptr+i+1) * (*rwgptr1);
                }
                rwgptr1++;
                for (i = nchans-1; i >= 0; i--) {
                    acc0[i] += (*revptr--) * (*rwgptr);
                }
                rwgptr++;
			}
		}

        /* interpolate */
        for (i = 0; i < nchans; i++) {
            acc0[i] = (acc0[i] >> 1) + MULHI(acc1[i] - acc0[i], phasef >> 1);
            acc0[i] >>= 14;
        }

        /* saturate */
        for (i = 0; i < nchans; i++) {
            if ((sign = (acc0[i] >> 31)) != (acc0[i] >> 15))
                acc0[i] = sign ^ ((1<<15)-1);
        }

        for (i = 0; i < nchans; i++) {
            *outptr++ = (short) acc0[i];
        }

		/* step phase fraction */
		phasef += s->stepf;
		if (phasef < s->stepf) {
			rwgptr = rwgptr1;
			lwgptr = lwgptr1;
			pcmptr += pcmstep;
		}

		/* step phase by N */
		tab = (rwgptr > s->stepNptr ? s->stepNbak : s->stepNfwd);
		rwgptr += tab[0];
		lwgptr += tab[1];
		pcmptr += tab[2];

		/* phase+1 */
		tab = (rwgptr >= s->step1ptr ? s->step1bak : s->step1fwd);
		rwgptr1 = rwgptr + tab[0];
		lwgptr1 = lwgptr + tab[1];
		pcmstep = tab[2];
	}

	s->offset = (int)(pcmptr - pcmend);
	s->rwing = rwgptr;
	s->lwing = lwgptr;
	s->phasef = phasef;

	return outptr;
}
