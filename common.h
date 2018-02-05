#ifndef __COMMON_H__
#define __COMMON_H__
#define fmsg(...) {if (!qflag) { printf (__VA_ARGS__);}; }
#define flog(...) {if (!qflag) { printf ("L["TAG"]: "__VA_ARGS__);}; }
#define fwarn(...) {if (!qflag) { printf ("W["TAG"]: "__VA_ARGS__);}; }
#define ferr(...) {if (!qflag) { printf ("E["TAG"]: "__VA_ARGS__);}; }

#define xstr(a) str(a)
#define str(a) #a

extern bool qflag;

#endif/*__NO_COMMON_H__*/
