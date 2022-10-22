
/*
 *      file    name            file.h
 */

typedef long Fpos_t; /* like fpos_t, but used in fgetpos2() fsetpos2() etc. */

#ifdef LSI_C
#include <dos.h>
#define FAR far
#define farptr_diff(a, b) \
	((int)(FP_SEG(a) - FP_SEG(b)) * 16L + (int)(FP_OFF(a) - FP_OFF(b)))
#else
#define FAR
#define farptr_diff(a, b) ((a) - (b))
#endif

#ifndef BCC
struct File{
	char FAR *top_adr;
	char FAR *cur_adr;
	char FAR *end_adr;
	char FAR *alloc_end_adr;
	int  flag; /* read 0 , write 1 */
#ifdef WINDOWS
	HANDLE f;
#else
	int f;
#endif
};
typedef struct File file;
typedef file *fileptr;
#else
typedef long fileptr;
#endif
 /* BCCとそれ以外で型宣言を分けるのはいい加減うっとうしくなったので
    fileptrでまとめることにした(Nide) */

fileptr fopen2(char *, char *);
void fclose2(fileptr);
#ifndef WINDOWS
fileptr fdopen2(int, char *);
#else
fileptr fdopen2(HANDLE, char *);
#endif
#ifdef BCC
int getc2(fileptr);
void putc2(int, fileptr);
void fseek2(fileptr, long, int);
long ftell2(fileptr);
void fgetpos2(fileptr, Fpos_t *);
void fsetpos2(fileptr, Fpos_t *);
#endif
void fclose3(fileptr, fileptr);
void fclose4(fileptr, fileptr);	/* 3のWRITE */
fileptr fmalloc(void);
void ffree(fileptr);

extern void owari(void);

#ifndef BCC
#define adjust_end_address(fp) /* Add Nide */ do { \
	if((fp)->cur_adr > (fp)->end_adr) (fp)->end_adr = (fp)->cur_adr; \
} while(0)
extern void check_alloc_amount(fileptr);
extern void check_alloc_amount_aux(fileptr);

#ifdef __GNUC__
#define check_alloc_amount(fp) (void)({ \
	if((fp)->cur_adr >= (fp)->alloc_end_adr) check_alloc_amount_aux(fp); \
	adjust_end_address(fp); \
})
#endif
#ifdef LSI_C
extern int putc2(int, file *);
extern int getc2(file *);
extern long ftell2(file *);
extern void fseek2(file *, long, int);
 /* putc2(), getc2(), ftell2() are defined in file.c */
#else
#define putc2(ca,fp) (check_alloc_amount(fp), *((fp)->cur_adr++) = (ca))
#define getc2(fp) \
  ((fp)->cur_adr == (fp)->end_adr ? EOF : (int)(unsigned char)*(fp)->cur_adr++)
#define ftell2(fp) (long)farptr_diff((fp)->cur_adr, (fp)->top_adr)
#define fseek2(fp, l, s) do { \
	adjust_end_address(fp); \
	switch(s){ \
	case SEEK_CUR: \
		(fp)->cur_adr += (long)(l); \
		break; \
	case SEEK_SET: \
		(fp)->cur_adr = (fp)->top_adr + (long)(l); \
		break; \
	case SEEK_END: \
		(fp)->cur_adr = (fp)->end_adr + (long)(l); \
	} \
	check_alloc_amount(fp); \
} while(0)
#endif
#define fgetpos2(fp, l) (void)(*(l) = (Fpos_t)ftell2(fp))
#define fsetpos2(fp, l) fseek2((fp), (long)*(l), SEEK_SET)
#endif

#define ungetc2(fp) fseek2((fp), -1L, SEEK_CUR)
 /* ungetc()と違って戻す文字は指定できない */

#undef extern

#ifdef NONANSI_REALLOC
#define realloc Realloc
#define free Free
void *realloc(void *, int);
void free(void *);
#endif
#ifdef MALLOC_DEBUG
#define malloc Malloc
#define realloc Realloc
#define free Free
void *malloc(int);
void *realloc(void *, int);
void free(void *);
#endif
