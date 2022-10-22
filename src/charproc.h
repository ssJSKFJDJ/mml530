
/*
 *      file    name            charproc.h
 */

void free_all_macros(void);
void init_all_macros(void);
void scanmacro(void);
int Getbyte(int, int);
#define getbyte(x) Getbyte(x, 0)
extern void owari(void);

#include <ctype.h>
#ifndef isoctal
#define isoctal(c) (isdigit(c) && (c) < '8')
#endif
#define is_alpha(c) (isascii(c) && isalpha(c))
#define is_upper(c) (isascii(c) && isupper(c))
#define is_lower(c) (isascii(c) && islower(c))
#define to_upper(c) (is_lower(c) ? toupper(c) : (c))
#define to_lower(c) (is_upper(c) ? tolower(c) : (c))
#define is_alnum(c) (isascii(c) && isalnum(c))
#define is_digit(c) (isascii(c) && isdigit(c))
#define is_octal(c) (isascii(c) && isoctal(c))
#define is_xdigit(c) (isascii(c) && isxdigit(c))
#define is_space(c) (isascii(c) && isspace(c))
 /* toupperの定義域が英小文字に限られたり、isupperなどの定義域が7ビット文字に
    限られたりするC処理系もあるので要注意 */
#define xtoi(c) ((c) - ((c)>='a' ? 'a'-10 : (c)>='A' ? 'A'-10 : '0'))
#define dtoi(c) ((c) - '0')
#define Ismskanji1(c) (0x81<=(c) && ((c)<=0x9f || (0xe0<=(c) && (c)<=0xfc)))
#define ismskanji1(c) Ismskanji1((unsigned char)(c))

struct master_step {
	long step;
	int linenum;
};

#define mml_warn(i) mml_err(-(i))
