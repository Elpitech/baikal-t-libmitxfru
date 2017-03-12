#ifndef __COMMON_H__
#define __COMMON_H__
#define msg(...) {if (!qflag) { printf (__VA_ARGS__);}; }
#define log(...) {if (!qflag) { printf ("L["TAG"]: "__VA_ARGS__);}; }
#define warn(...) {if (!qflag) { printf ("W["TAG"]: "__VA_ARGS__);}; }
#define err(...) {if (!qflag) { printf ("E["TAG"]: "__VA_ARGS__);}; }


extern bool qflag;

#endif/*__NO_COMMON_H__*/
