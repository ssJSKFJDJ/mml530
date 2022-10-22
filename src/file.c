
/*
 *      file    name            file.c
 */

/*
 * file.c には，以下の事柄に関する関数が入っている．
 * ・ファイルのオープン/クローズ
 * ・メモリの確保/解放
 * ・メモリへのアクセス
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win.h"
#include "file.h"

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

#ifdef WINDOWS
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef MSDOS
# ifdef BCC
# include <alloc.h> /* include if TC or BC */
# endif
#include <fcntl.h> /* for fopen, O_RDONLY */
#include <sys/stat.h> /* for pmode(S_IREAD, S_IWRITE), creat */
#include <io.h> /* for filelength, creat */
#endif /* But this file is not used when using BCC... */

#ifdef LSI_C
# include <farstr.h>
# define far_malloc farmalloc
# define far_realloc farrealloc
# define far_free farfree
#else
# define far_malloc malloc
# define far_realloc realloc
# define far_free free
#endif

extern void mml_err(int);

#ifdef UNIX
static long Filelength(int fd)
{
	struct stat sb;
	fstat(fd, &sb);
	return sb.st_size;
}
#else
#define Filelength filelength
#endif

#if defined(UNIX)
 /* UNIXでファイルのread/writeをより安全に行う仕掛け。read()やwrite()が
    1バイトも読み書きしないうちに割り込まれたり、write()が一度に全て書き込め
    なかったりするのを防止 */
#ifndef EINTR
#define Read read
#else
static int Read(int fd, char *buf, int amount)
{
	int got;
	while((got = read(fd, buf, amount)) == -1 && errno == EINTR);
	return got;
}
#endif

static int Write(int fd, char *buf, int amount)
{
	int written, ret = 0;

	while(amount){
		if((written = write(fd, buf, amount)) == -1){
#ifdef EINTR
			if(errno == EINTR) continue;
#endif
			return -1;
		}
		buf += written, amount -= written, ret += written;
	}
	return ret;
}
#elif defined(LSI_C) /* far pointerへのread/write */
static long Read(int fd, char far *buf, long amount)
{
	char tmp[BUFSIZ];
	long tmp_total = 0;

	while(amount){
		int tmp_amount = (amount > BUFSIZ ? BUFSIZ : (int)amount);
		int tmp_read;

		tmp_read = read(fd, tmp, tmp_amount);
		if(tmp_read == -1) return -1;
		far_memcpy(buf, (char far *)tmp, tmp_read);
		tmp_total += tmp_read;
		if(tmp_read < tmp_amount || tmp_amount < BUFSIZ) break;
		buf += (long)BUFSIZ;
		amount -= BUFSIZ;
	}
	return tmp_total;
}

static long Write(int fd, char far *buf, long amount)
{
	char tmp[BUFSIZ];
	long tmp_total = 0;

	while(amount){
		int tmp_amount = (amount > BUFSIZ ? BUFSIZ : (int)amount);
		int tmp_wrote;

		far_memcpy((char far *)tmp, buf, tmp_amount);
		tmp_wrote = write(fd, tmp, tmp_amount);
		if(tmp_wrote == -1) return -1;
		tmp_total += tmp_wrote;
		if(tmp_wrote < tmp_amount || tmp_amount < BUFSIZ) break;
		buf += (long)BUFSIZ;
		amount -= BUFSIZ;
	}
	return tmp_total;
}
#else /* !UNIX && !LSI_C */
#define Write write
#define Read read
#endif

#ifndef WINDOWS
void write_error(void)
{
	perror("write() or close() error: ");
	owari();
}
#endif

#ifndef __GNUC__
void check_alloc_amount(file *fp)
{
	if(fp->cur_adr >= fp->alloc_end_adr) check_alloc_amount_aux(fp);
	adjust_end_address(fp); /* ついでに */
}
#endif /* GCCではマクロ */

void check_alloc_amount_aux(file *fp) /* Add Nide */
{
	char FAR *prev_top_adr;
	long amount;

	prev_top_adr = fp->top_adr;
	amount = farptr_diff(fp->cur_adr, fp->top_adr) + 40000L;
	fp->top_adr = far_realloc(fp->top_adr, amount);
	if(fp->top_adr == NULL) mml_err(61);

	fp->cur_adr = fp->top_adr + farptr_diff(fp->cur_adr, prev_top_adr);
	fp->end_adr = fp->top_adr + farptr_diff(fp->end_adr, prev_top_adr);
	fp->alloc_end_adr = fp->top_adr + amount;
}

#ifdef WINDOWS
file *fdopen2(HANDLE f, char c2[])
#else
file *fdopen2(int f, char c2[])
#endif
{
	long l;
	file *fp = (file *)malloc(sizeof(file));

	if(fp == NULL) return NULL; /* 異常終了 */
	fp->top_adr = NULL; /* top_adr確保以前にfclose2()が呼び出された場合に
		ffree()が確保してないtop_adrをfreeしようとするのを防止 */
	fp->f = f;

	/* ここで、c2[1]が'b'ならfをバイナリモードにする措置をすべき。だが、
	   OSやコンパイラによって措置は異なる。BCCの場合、mml2mid.cで_fmodeに
	   O_BINARYを代入しているので強制バイナリモード(もっともBCCでは本
	   ファイルは使われないのだが) */
#if !defined(UNIX) && !defined(LSI_C)
	if(c2[1] == 'b') setmode(f, O_BINARY);
#endif

	if(c2[0] == 'r'){
#ifdef WINDOWS
		DWORD size;
#endif

		fp->flag = 0;
#ifdef WINDOWS
		l = GetFileSize(f,NULL);
#else
		l = Filelength(f);
#endif
		fp->top_adr = fp->cur_adr = far_malloc(l);
		if(fp->top_adr == NULL){ /* 異常終了 */
			free(fp); /* fはcloseしない */
			return NULL;
		}
#ifdef WINDOWS
		ReadFile(f,fp->top_adr,l,&size,NULL);
#else
		(void)Read(f,fp->top_adr,l);
#endif
		fp->alloc_end_adr = fp->end_adr = fp->top_adr + l;
	} else { /* c2[0] == 'w' */
		fp->flag = 1;

	/* どうせテンポマップしか書かないのだから，あまりたくさん
		メモリを確保しなくてもよい． */
	/* と思ったけど、$$$の関係でメモリを確保しないとダメになった． */
#ifdef WINDOWS
		l = 420000L;
#else
		l = 65000L;
#endif /* 今は必要に応じてreallocするからあまり大きく取らなくていいのだが */
		fp->top_adr = fp->cur_adr = far_malloc(l);
		if(fp->top_adr == NULL){ /* 異常終了 */
			free(fp); /* fはcloseしない */
			return NULL;
		}
		fp->end_adr = fp->top_adr;
		fp->alloc_end_adr = fp->top_adr + l;
	}
	return fp; /* 正常終了 */
}

file *fopen2(char c1[], char c2[])
{
	file *fp;
#ifdef WINDOWS
	HANDLE f;
#else
	int f;
#endif

	if(c2[0] == 'r'){
#ifdef WINDOWS
		f = CreateFile(c1,GENERIC_READ,0,NULL,
				 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#else
# ifdef BCC
		f = open(c1, O_RDONLY);
# else
		f = open(c1, c2[1] == 'b' ? O_RDONLY|O_BINARY : O_RDONLY);
# endif
#endif
	} else { /* c2[0] == 'w' */
#ifdef WINDOWS
		f = CreateFile(c1,GENERIC_WRITE,0,NULL,
				 CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
#else
# ifdef BCC
		f = creat(c1, S_IREAD | S_IWRITE);
# else
 		f = open(c1,
			c2[1] == 'b' ? O_CREAT|O_TRUNC|O_WRONLY|O_BINARY :
						   O_CREAT|O_TRUNC|O_WRONLY,
			0666);
# endif
#endif
	}
#ifdef WINDOWS
	if(f == INVALID_HANDLE_VALUE) return NULL; /* 異常終了 */
#else
	if(f == -1) return NULL; /* 異常終了 */
#endif

	fp = fdopen2(f, c2);
#ifdef WINDOWS
	if(fp == NULL) CloseHandle(f);
#else
	if(fp == NULL) close(f);
#endif
	return fp;
}

void fclose2(file *fp)
{
#ifdef WINDOWS
	if(fp->f != INVALID_HANDLE_VALUE){
		if(fp->flag == 1){
			DWORD size;

			adjust_end_address(fp);
			WriteFile(fp->f,fp->top_adr,
			    (long)(fp->end_adr - fp->top_adr),&size,NULL);
		}
		CloseHandle(fp->f);
	}
#else
	if(fp->f != -1){
		if(fp->flag == 1){
			adjust_end_address(fp);
			if(Write(fp->f, fp->top_adr,
			    (long)farptr_diff(fp->end_adr, fp->top_adr)) == -1)
				write_error();
		}
		if(close(fp->f) == -1) write_error();
	}
#endif

	ffree(fp);
}

/* fpa の後ろに fpb をくっつけて fpa をクローズする */
/* fpb はメモリの解放もクローズもしない */
void fclose3(file *fpa, file *fpb)
{
	adjust_end_address(fpa);
	adjust_end_address(fpb);
#ifdef WINDOWS
	{
		DWORD size;
		WriteFile(fpa->f,fpa->top_adr,
			  (long)(fpa->end_adr - fpa->top_adr),&size,NULL);
		WriteFile(fpa->f,fpb->top_adr,
			  (long)(fpb->end_adr - fpb->top_adr),&size,NULL);
	}
	CloseHandle(fpa->f);
#else
	if(Write(fpa->f, fpa->top_adr,
		 (long)farptr_diff(fpa->end_adr, fpa->top_adr)) == -1 ||
	   Write(fpa->f, fpb->top_adr,
		 (long)farptr_diff(fpb->end_adr, fpb->top_adr)) == -1 ||
	   close(fpa->f) == -1){
		write_error();
	} /* write()やclose()の戻り値はチェックすべき。fclose4()も同様 */
#endif

	ffree(fpa);
}

/* ファイルをオープンせずに malloc するだけ */
/* malloc に失敗すると NULL を返す */
file *fmalloc(void)
{
	long l;
	file *fp = (file *)malloc(sizeof(file));

	if(fp == NULL) return NULL; /* 異常終了 */

	/* fp->f = 0; */
	/* fp->flag = 1; */

	/* SMF がスッポリ入るぐらいメモリを確保したい */
#ifdef WINDOWS
	l = 420000L;
#else
	l = 65000L;
#endif
	fp->top_adr = fp->cur_adr = far_malloc(l);
	if(fp->top_adr == NULL){
		free(fp);
		return NULL; /* 異常終了 */
	}
	fp->end_adr = fp->top_adr;
	fp->alloc_end_adr = fp->top_adr + l;

	return fp; /* 正常終了 */
}

/* fp のメモリを解放する */
void ffree(file *fp)
{
	if(fp->top_adr != NULL) far_free(fp->top_adr);
	free(fp);
}

/* fpa の部分に fpb の内容を書く fpa をクローズする */
/* fpb はメモリの解放もクローズもしない */
void fclose4(file *fpa, file *fpb)
{
	adjust_end_address(fpa);
	adjust_end_address(fpb);
#ifdef WINDOWS
	{
		DWORD size;
		WriteFile(fpa->f, fpb->top_adr,
			  (long)(fpb->end_adr - fpb->top_adr), &size, NULL);
	}
	CloseHandle(fpa->f);
#else
	if(Write(fpa->f, fpb->top_adr,
		 (long)farptr_diff(fpb->end_adr, fpb->top_adr)) == -1 ||
	   close(fpa->f) == -1){
		write_error();
	}
#endif

	ffree(fpa);
}

#ifdef LSI_C
int putc2(int ca, file *fp)
{
	check_alloc_amount(fp);
	*(fp->cur_adr) = ca;
	fp->cur_adr += 1L;
	return ca;
}
int getc2(file *fp)
{
	int ca;

	if(fp->cur_adr == fp->end_adr) return EOF;
	ca = (int)(unsigned char)*(fp->cur_adr);
	fp->cur_adr += 1L;
	return ca;
}
long ftell2(file *fp)
{
	return (long)farptr_diff(fp->cur_adr, fp->top_adr);
}
void fseek2(file *fp, long l, int s)
{
	adjust_end_address(fp);
	switch(s){
	case SEEK_CUR:
		fp->cur_adr += l;
		break;
	case SEEK_SET:
		fp->cur_adr = fp->top_adr + l;
		break;
	case SEEK_END:
		fp->cur_adr = fp->end_adr + l;
	}
	check_alloc_amount(fp);
}
#endif

#ifdef NONANSI_REALLOC
#undef realloc
#undef free
void *Realloc(void *p, int s){
	return p==NULL? malloc(s) : realloc(p,s);
}
void Free(void *p){
	if(p != NULL) free(p);
}
#endif

#ifdef MALLOC_DEBUG
#undef malloc
#undef realloc
#undef free
void *Malloc(int s){
	void *q = malloc(s);

	fprintf(stderr, "  malloc->%08lx(%d)\n",(long)q,s);
	return q;
}
void *Realloc(void *p, int s){
	void *q = realloc(p,s);

	fprintf(stderr, "%08lx->%08lx(%d)\n",(long)p,(long)q,s);
	return q;
}
void Free(void *p){
	free(p);
	fprintf(stderr, "%08lx->free\n",(long)p);
}
#endif
