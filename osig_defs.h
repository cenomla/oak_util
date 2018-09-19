#pragma once


#ifdef __OSIG__
#define _reflect(x) __attribute__((annotate("osig_reflect:"#x)))
#define _exclude __attribute__((annotate("osig_exclude")))
#define _opaque __attribute__((annotate("osig_opaque")))
#define _volatile __attribute__((annotate("osig_volatile")))
#else
#define _reflect(x)
#define _exclude
#define _opaque
#define _volatile
#endif

namespace oak::catagory {
	struct primitive {};
	struct none {};
}

