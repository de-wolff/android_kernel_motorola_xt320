#ifndef _RTS_RANDOM_H_
#define _RTS_RANDOM_H_

/* borrowed random() from FreeBSD stdlib */
#if defined(TARGETENV_win32)
long random(void);
void srandom(unsigned long);
char *initstate(unsigned long, char *, long);
char *setstate(char *);
#endif /* TARGETENV_win32 */

#endif /* _RTS_RANDOM_H_ */
