
/*
 *      file    name            win.h
 */

/* #define UNIX */
/* #define WINDOWS */
/* #define BCC */

/* 衝突回避 */
#ifdef UNIX
# undef BCC
# undef WINDOWS
#endif

/* derive MSDOS */
#if defined(BCC) || defined(LSI_C)
# ifndef MSDOS
#  define MSDOS
# endif
#endif

#ifdef GLOBAL_VALUE_DEFINE
# define GLOBAL
#else
# define GLOBAL extern
#endif

#include <stdio.h>
#ifdef MSG_TO_STDOUT
# define STDERR stdout
#else
# define STDERR stderr
#endif

#ifndef WINDOWS
# define wsprintf  sprintf
# define TRUE 1
# define FALSE 0
GLOBAL int hWnd3;
GLOBAL char text[8192];
# define InvalidateRect(i, j, k)	/* no-op */
# define UpdateWindow(i) (fputs(text, STDERR), text[0] = '\0') /* i not used */
#else
# include <windows.h>
extern HWND hWnd3;
extern char text[8192];
#endif
GLOBAL char Msg[512]; /* タイトル(最大256字)が入ったりするので256では不足 */

#undef GLOBAL
