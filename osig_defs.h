#pragma once


#ifdef __OSIG__
#define _reflect(x) __attribute__((annotate("osig_reflect:"#x)))
#define _exclude __attribute__((annotate("osig_exclude")))
#else
#define _reflect(x)
#define _exclude
#endif

