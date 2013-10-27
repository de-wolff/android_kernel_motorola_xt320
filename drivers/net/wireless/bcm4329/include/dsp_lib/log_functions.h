#ifdef __cplusplus
extern "C" {
#endif
/* Portable Windows DLL symbol export macro for DPI import tf's */
#define DPI_DLLESPEC

/* Refer to 64-bit integers with typedefs "int64_t" and "uint64_t" */
#ifndef _WIN32
#include <inttypes.h>
#endif
	DPI_DLLESPEC int hw_logsum(int ibits, int fbits, int fbits_output, int f, int g);
	DPI_DLLESPEC int hw_alog(int fbits, int x);
	DPI_DLLESPEC int hw_log(int fbits, int x);
	DPI_DLLESPEC double hw_logsumd(int ibits, int fbits, double f, double g);
	DPI_DLLESPEC double hw_alogd(int fbits, double x);
	DPI_DLLESPEC double hw_logd(int fbits, double x);
#ifdef __cplusplus
}
#endif
