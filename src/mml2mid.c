
/*
 *	Program name		MML->MID Compiler  Version 5.30b
 *      Program written         A.Monden        1993/2/7
 *      Program modified        A.Monden        2001/2/24
 *                              H.Fujii (MKR)   1998/3/3
 *                              H.Kuroda        1998/4/1
 *                              N.Nide          2002/9/5 - 2011/6/12
 */

/*
 *      file    name            mml2mid.c
 */

#define VER "5.30"

#define GLOBAL_VALUE_DEFINE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef UNIX
#include <unistd.h> /* isatty() */
#endif
#include "win.h"
#include "file.h"
#include "charproc.h"
#include "mmlproc.h"
#include "mml2mid.h"

#ifdef BCC
#include <alloc.h> /* include if TC or BC */
#include <fcntl.h> /* for O_BINARY */
#endif

#ifdef UNIX
#include <fcntl.h> /* for O_BINARY */
#endif

#ifdef WINDOWS
#include <setjmp.h>
static jmp_buf env;	/* ���顼������ */
#endif

static fileptr fp0;	/* ���ϥե�����(mml) */
fileptr fp1;	/* �ץ�ץ�����ե�����(mml)��񤭹��� */
fileptr fp2;	/* �ƥ�ݥޥåװʳ���SMF��񤭹����� */
fileptr fp3;	/* ���ϥե�����(mid) �ǽ�ϥƥ�ݡ��ޥåפ��񤫤�� */
#ifdef WINDOWS
static char outfile[128]; /* ���ϥե�����̾(�Ѵ����Ի��Υե���������) */
#else
static char *outfile = NULL;
#endif
int timebase;           /* ������١��� 48,60,80,96,120,160,240,480 */
static Fpos_t trkno;    /* �ȥ�å�����񤭹����� */
int trknum;             /* �ȥ�å��� */
Fpos_t trksize;         /* �ȥ�å���������񤭹����� */
static Fpos_t trkend;   /* �ȥ�å��ν�λ���� */
static Fpos_t tposize;  /* �ƥ�ݥȥ�å��Υȥ�å���������񤭹����� */
static char *title = NULL;	/* �ʤΥ����ȥ� */
static char *copyright = NULL;	/* �ʤ�������� */
Fpos_t lastlenpos;       /* �Ǹ�˲�Ĺ��񤭹�������ؤΥݥ��� */
long lastlen;            /* �Ǹ�˽񤭹������Ĺ���� */
int tnum; /* �ѡ���̾��  �ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� 0,1,2 */
int talf; /* �ȥ�å�̾���ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� A,B,C */
int trans = 0; /* ���ץ���󥹥��å�T�ˤ��žĴ�� */
int x68k = 0;  /* <, > ��դˤ��륹���å� */
int x68k2 = 0; /* (, ) ��դˤ��륹���å� */
int german_scale = 0; /* ��̾��ɥ���ή��CDEFGAH�ˤ��륹���å� */
char track_map[320];	/* MKR�ɲ� */
int backcompati;

#ifndef WINDOWS
static void sw(void);
#endif

void remove_file_and_owari(void);
static void remove_file_and_owari0(void);
static void remove_file_and_owari1(void);
static void remove_file_and_owari2(void);
void owari(void);
static int META_event(int);
extern int escchr(int);

static void smfheader(fileptr, int); /* SMF�Υإå�����Ϥ��� */
void smftrkheader(fileptr, Fpos_t *); /* SMF�ȥ�å��إå����� */
static void smftrkend(fileptr, Fpos_t *);
				/* SMF�ȥ�å���λ���٥�Ƚ��� */
extern void write_length(long, fileptr);
static void puttitle(char *, char *);
static void analyze(void);
extern void clear_master_step(void);
extern int alloc_master_step(void);

 /* �ץ�ץ������˻Ȥ���¤�� */
typedef struct {
	char *buf;
	int amount;

	fileptr fpi;
	char *curfile, *virt_curfile;
	 /* �����ץ�ץ����Ȥζ�Ĵ�Ѥ˽��Ϥ���ե�����̾����\�פ�����������
	    ����Ƥ����ꤹ��Τǡ�fopen2()���Ƥ�ɬ�����������Ϥ��ʤ� */
	int lineno, pending_eols;
	int inside_comment; /* �֡�*�ס���*���פ���ʤ餽�γ��Ϲ��ֹ桢�Ǥʤ���0 */
} prepro_linebuf;
static void getsp(char *, fileptr, fileptr);
static void getppinfo(char *, prepro_linebuf *, fileptr);
static void getinclude(char *, prepro_linebuf *, fileptr);
static void gettitle(char *, char *, prepro_linebuf *);
static int getintdirective(char *, char *, prepro_linebuf *);
static void getswap(char *, prepro_linebuf *);
static void prepro_error(char *, prepro_linebuf *);
static void prepro_illdirective(prepro_linebuf *);
static void prepro_illstring(prepro_linebuf *);
static void prepro_illcont(prepro_linebuf *);
static void prepro_nomem(void);

static fileptr fp4;
static int rstatus[MAXTRKNUM];
static Fpos_t trktop[MAXTRKNUM];
static int fmat = 1;
#ifndef DEFAULT_MSCODE
int mskanji = 0;
#else
int mskanji = 1;
#endif
int warnmode = 0;
static int classic_behavior = 0;

static void format1to0(void);
static int get1(int i);
static long getlength(int x);
static void MIDI_event(int x, int code);
static void write_code(int x);

static char *strnDup(char *s, int n)
{
	int l = strlen(s);
	char *p;

	if(l < n) n = l;
	if(NULL != (p = malloc(n + 1))) strncpy(p, s, n), p[n] = '\0';
	return p;
}

#ifndef WINDOWS
static char err_mem[] = "ERROR! Insufficient memory\n";

static void add_env_options(char *envname, int *argcp, char ***argvp)
{
	char	*env, *p;
	int	nargc, i, nargv_size;
	char	**nargv;

	if(NULL == (env = getenv(envname))) return;
	if(NULL == (env = strdup(env))) goto err_nomem;
	nargv = malloc(sizeof(char *) * (nargv_size = *argcp + 6));
	if(NULL == nargv) goto err_nomem;
	 /* nargv�� (*argvp)[0], env��ʬ��, (*argvp)[1��(*argcp-1)], NULL
	    ��������� nargv�Υ�������(*argcp+1 + env��ʬ��Ŀ�)����ɬ�� */
	nargv[0] = (*argvp)[0];
	for(p = env, nargc = 1; *p; p++){
		if(!is_space(*p)){
			if(nargc + *argcp >= nargv_size){
				nargv = realloc(nargv,
					(nargv_size += 5) * sizeof(char *));
				if(NULL == nargv) goto err_nomem;
			}
			nargv[nargc++] = p;
			do p++; while(*p && !is_space(*p));
			if(!*p) break; else *p = '\0';
		}
	}
	for(i = 1; i < *argcp; ) nargv[nargc++] = (*argvp)[i++];
	nargv[nargc] = NULL;
	*argcp = nargc, *argvp = nargv;
	return;

err_nomem:
	fprintf(STDERR, err_mem);
	owari();
}

static void options(char *opt, int *argcp, char ***argvp)
 /* Add Nide; ���ץ����ʸ����opt����Ϥ���*argcp��*argvp��ʤ�� */
{
	int c;

	while((c = *opt++) != '\0'){
/* CASE_INSENSITIVE_OPTS��define����Ƥ���ȡ����ץ�������ʸ����ʸ����
   Ʊ��� */
#ifdef CASE_INSENSITIVE_OPTS
		c = to_lower(c);
#endif
		switch(c){
		case 'f': /* format 0/1 */
			fmat ^= 1; continue;
		case 'x': /* swap '>', '<' */
			x68k ^= 1; continue;
		case 'v': /* swap ')', '(' */
			x68k2 ^= 1; continue;
		case 'm': /* Assume MS-kanji code */
			mskanji ^= 1; continue;
		case 'w': /* Only warning on unrecognized symbols */
			warnmode ^= 1; continue;
		case 'o': case 'n':
			fprintf(STDERR, "option switch '%c' is obsolete. Ignored.\n", c);
			continue;
		case 't': /* transpose */
			if(!*opt){
				if(!--*argcp){
					fprintf(STDERR, "ERROR!  switch '%c' needs an argument\n", c);
					sw();
				}
				opt = *++*argvp;
			}
			trans = atoi(opt);
			break;
		case 'c': /* Classic behavior. Accept unclosed comment and
			     regard a space at top of line as comment-line */
			classic_behavior ^= 1;
			break;
		default:
			fprintf(STDERR, "ERROR! unknown switch '%c'\n", c);
			sw();
		}
		break;
	}
	--*argcp, ++*argvp;
}

#if defined(MSDOS)
# ifdef LSI_C
# define VER_TARGET " (DOS version, compiled by LSI-C)"
# else
# define VER_TARGET " (DOS version)"
# endif
#elif defined(EGCSW32)
#define VER_TARGET " (Win32 console version)"
#elif defined(UNIX)
#define VER_TARGET " (UNIX version)"
#else
#define VER_TARGET ""
#endif

#ifndef BCC
static char stdio_name[] = "-";
#endif

int main(int argc, char *argv[])
{
	char *srcfile, *base;
	static char MML_EXT[] = ".mml", MID_EXT[] = ".mid";

	fprintf(STDERR, "MML->MID Compiler Ver" VER VER_TARGET "\n");

	/* option switch handling -------- >>>>>>>>>> */
	add_env_options("MML2MID_OPTIONS", &argc, &argv);
	 /* completely rewritten by Nide; �ޤ���Ƭ�Ρ�-�פǻϤޤ������
		���ץ����ȸ��ʤ��Ʋ��� */
	for(--argc, ++argv; argc && **argv == '-' && argv[0][1];
		options(*argv + 1, &argc, &argv));

	 /* ���μ��ΰ�����MML�ե�����̾ */
	if(!argc--) sw();
	srcfile = *argv++;

/* USR_NONMINUS_OPTS���������Ƥ���С�MML�ե�����̾�μ��ΰ�������-�פ�
   �Ϥޤ�ʤ�2ʸ���ʲ��ξ��ˤ����ʹߤ�̵���˥��ץ��������������� */
#ifdef USE_NONMINUS_OPTS
	if(!(argc && **argv != '-' && strlen(*argv) <= 2))
#endif
	{
		 /* ���θ�ˡ�-�פǻϤޤ����������֤ϥ��ץ����Ȥ��Ʋ��� */
		for(; argc && **argv == '-' && argv[0][1];
			options(*argv + 1, &argc, &argv));
		 /* ���μ��˰���������Ф��줬MID�ե�����̾��outfile��free()���ʤ���
		    argv�˻Ȥ��Ƥ���ʸ�����񤭴����ʤ��Τǡ�outfile�Ȥ���*argv��
		    ľ�ܻȤäƻپ�ʤ� */
		if(argc) argc--, outfile = *argv++;
	}

	 /* �Ĥ�����ϥ��ץ����â��USE_NONMINUS_OPTS���������Ƥ��ʤ�����
	    ��-�פǻϤޤ�ʤ����ץ���������ʤ� */
	while(argc){
		char *p = *argv;
#ifndef USE_NONMINUS_OPTS
		if(*p != '-' || !*++p) sw();
#else
		if(*p == '-' && !*++p) sw();
#endif
		options(p, &argc, &argv);
	}
	/* <<<<<<<<< -------- option switch handling */

#ifdef BCC
	_fmode = O_BINARY; /* for BCC �����Х��ѿ� _fmode ���ͤ����� */
#endif

	/* �ǥ��쥯�ȥ�̾��Ρ�.�פ��ĥ�ҤλϤޤ�ȸ�ǧ����Τ��ɻߤΤ��� */
#if !defined(MSDOS) && !defined(EGCSW32)
	if((base = strrchr(srcfile, '/')) == NULL) base = srcfile;
#else
	{ /* '/'��'\\'��ѥ����ڤꡣ����˥ե�����̾�δ������θ */
		char *p;
		for(base = p = srcfile; *p; p++){
			if(*p == '/' || *p == '\\' || *p == ':') base = p;
			else if(ismskanji1(*p) && p[1]) p++;
		}
	}
#endif

	/* mml file open -------- >>>>>>>>>> */
#ifndef BCC /* BCC�Ǥϥե������Ϣ�δؿ���file.asm�Ǽ¸�����Ƥ��뤿��
	       fdopen2()�����ʤ� */
	if(!strcmp(srcfile, stdio_name)){ /* read stdin */
		fp0 = fdopen2(0, "rb");
		if(fp0 == NULL){
			fprintf(STDERR, err_mem);
			owari();
		}
	} else /* ��ĥ�Ҥʤ��Ǥ⤽�Υե�����̾����˻�褦���ѹ�����(Nide) */
#endif
	if((fp0 = fopen2(srcfile, "rb")) == NULL) do {
		if(strchr(base, '.') == NULL){ /* ��ĥ�Ҥʤ��ʤ��.mml�פ��դ���try */
			char *file1;

			if((file1 = malloc(strlen(srcfile) + sizeof(MML_EXT))) == NULL){
				fprintf(STDERR, err_mem);
				owari();
			}
			sprintf(file1, "%s%s", srcfile, MML_EXT);
			fp0 = fopen2(file1, "rb");
			free(file1);
			if(fp0 != NULL) break; /* ���������顼�򥹥��å� */
		}
		fprintf(STDERR, "ERROR! Unable to open %s\n", srcfile);
		owari();
	} while(0);
	/* <<<<<<<<< -------- mml file open */

	/* mid file open -------- >>>>>>>>>> */
	if(outfile == NULL){ /* �ޤ����ϥե�����̾����ޤäƤ��ʤ���� */
#ifndef BCC
		if(!strcmp(srcfile, stdio_name)){
# ifdef MSG_TO_STDOUT
			fprintf(STDERR, "ERROR! No output file specified " \
				"while input is stdin.\n");
			fclose2(fp0);
			owari();
# else
			outfile = srcfile; /* �������stdout */
# endif
		} else
#endif
		{
			outfile = malloc(strlen(srcfile) + sizeof(MID_EXT));
			if(outfile == NULL){
				fprintf(STDERR, err_mem);
				fclose2(fp0);
				owari();
			}

	 		strcpy(outfile, srcfile);
			if(strchr(base, '.') == NULL){
				strcat(outfile, MID_EXT);
			} else {
				strcpy(strrchr(outfile, '.'), MID_EXT);
			}
		}
	}
#if !defined(BCC) && !defined(MSG_TO_STDOUT)
	if(!strcmp(outfile, stdio_name)){ /* output to stdout */
		if(isatty(1)){
			fprintf(STDERR, "MIDI output not written to a terminal.\n");
			fclose2(fp0);
			owari();
		}
		fp3 = fdopen2(1, "wb");
		if(fp3 == NULL){
			fprintf(STDERR, err_mem);
			fclose2(fp0);
			owari();
		}
	} else
#endif
	if((fp3 = fopen2(outfile, "wb")) == NULL) do {
		 /* Windows2000�ˤϡ��������ץ��餫��.mid�ե������ʰױ���
		    ����Ȥ��Υե������write open�Ǥ��ʤ��ʤ�Х�������??
		    �к��Ȥ��ư��ٺ�����ƺƻ�Ԥ��Ƥߤ뤬��ͭ�������� */
/* #ifdef EGCSW32 */ /* ͭ���ǤϤʤ��ä��ΤǤȤ��� */
#if 0
		if(access(outfile, W_OK) == 0 && unlink(outfile) == 0 &&
		   (fp3 = fopen2(outfile, "wb")) != NULL){
			break; /* ���������顼�򥹥��å� */
		}
		 /* Symbolic link�Τ���OS�ǤϤ������֤򤷤ƤϤ����ʤ�(Symbolic
		    link�������Ƥ��ޤ�)���ޤ����ޤȤ�ʥե����륢���������¤�
		    �����򤷤Ƥ���OS�Ǥ⤳�����֤Ϥ��ʤ����������Ȼפ�(�ե�����
		    �θ��¤�����Ʊ���Ǥʤ��ʤ�)��*/
#endif
		fprintf(STDERR, "ERROR! Unable to open %s\n", outfile);
		fclose2(fp0);
		owari();
	} while(0);
	/* <<<<<<<<< -------- mid file open */

	analyze();
	InvalidateRect(hWnd3,NULL,TRUE);
	UpdateWindow(hWnd3);
	ffree(fp1);
	return 0; /* ���ｪλ */
}
#endif

#ifdef WINDOWS
/* . */
/* file1 �� ���� mml �ե������̾�� */
/* file2 �� ���� mid �ե������̾�� */
/* file2[0] == '\0' �ʤ�С����ϥե�����̾��ư�������� */
/* ��ư�������� foo.mml �����Ϥʤ�� foo.mid  ����ϥե�����Ȥ��� */
/* . */
/* static��Ĥ���� Windows�ץ���फ��ƤӽФ��ʤ��ʤ� */
int mml_smf(char *file1,char *file2,int xx,int ff,int oo,int tt)
{
	int i;
	char *l;

	if(setjmp(env)){
		return 1; /* longjmp ������ä��Ȥ��¹Ԥ���� */
	}

	/* if(strlen(text)>400) text[0]='\0'; */
	text[0]='\0';
/*	wsprintf(Msg,
	"MML->MID Compiler Ver" VER "  By A.Monden & MKR & H.Kuroda\n\n");
	 */
/*
	wsprintf(Msg,"MML->MID Compiler Ver" VER "\n\n");
	strcat(text,Msg);
	*/
	/* InvalidateRect(hWnd3,NULL,TRUE); */
	/* UpdateWindow(hWnd3); */

	trans=tt;   /* �ȥ�󥹥ݡ��� */
	fmat=ff;    /* �ե����ޥå� */
	x68k = xx;  /* >��<�����촹���륪�ץ���� */
	x68k2 = oo; /* )��(�����촹���륪�ץ���� */

	/* mml file open -------- >>>>>>>>>> */
	if((fp0 = fopen2(file1, "rb")) == NULL){
		wsprintf(Msg,"ERROR! File open error '%s'\n",file1);
		strcat(text,Msg);
		owari();
	}
	/* <<<<<<<<<< -------- mml file open */

	/* mid file open -------- >>>>>>>>>> */
	file2[0]='\0';  /* 98/03/24 */
	if(file2[0] == '\0'){
		strcpy(outfile,file1);
		l=(char *)strchr(outfile,'.');
		while(strchr(l+1,'.') != NULL){ /* path ̾��ʣ��"."�� */
			l=(char *)strchr(l+1,'.');  /* ������ν��� */
		}
		*l='\0';
		strcat(outfile,".mid");
	}else{
		strcpy(outfile, file2);
	}
	if((fp3 = fopen2(outfile, "wb")) == NULL ){
		wsprintf(Msg,"ERROR! File open error '%s'\n",outfile);
		strcat(text,Msg);
		fclose2(fp0);
		owari();
	}
	/* <<<<<<<<<< -------- mid file open */

	strcpy(file2,outfile);

	analyze();
	InvalidateRect(hWnd3,NULL,TRUE);
	UpdateWindow(hWnd3);
	ffree(fp1);
	return 0; /* ���ｪλ */
}
#endif /* WINDOWS */

#ifndef WINDOWS
static void sw(void)   /* Usage */
{
	fprintf(STDERR, 
		"\nUsage: mml2mid [switches] filename[.mml] [filename.mid]\n"
		"  (switches) -f  : Output format 0 file\n"
		"             -tn : Transpose\n"
		"             -x  : Swap >,<\n"
		"             -v  : Swap ),(\n"
#ifndef DEFAULT_MSCODE
		"             -m  : Assume MS-kanji code in strings (toggle)\n"
#else
		"             -m  : Not assume MS-kanji code in strings (toggle)\n"
#endif
		"             -w  : Only warning on unrecognized symbols\n"
		"             -c  : Classic undocumented behavior\n"
#if !defined(MSG_TO_STDOUT)
		"\tInput and output filenames can be \"-\" (which means stdin/stdout).\n"
#elif !defined(BCC)
		"\tInput filename can be \"-\" (which means stdin).\n"
#endif
	);
	owari();
}
#endif /* !WINDOWS */

/* ���٤ƤΥե�����򥪡��ץ󤷤���ǡ����顼���Ф����ˡ�
   ���δؿ����ƤФ�롥
   ���δؿ��ϡ�mmlproc.c��mml_err()�ʤɤ���ƤФ�롥 */
void remove_file_and_owari(void)
{
	free_all_macros();
	remove_file_and_owari2();
}
static void remove_file_and_owari2(void)
{
	ffree(fp2);
	remove_file_and_owari1();
}
static void remove_file_and_owari1(void)
{
	ffree(fp1);
	remove_file_and_owari0();
}
static void remove_file_and_owari0(void)
{
	fclose2(fp3);
	remove(outfile);
	owari();
}

void owari(void)
{
	InvalidateRect(hWnd3,NULL,FALSE);
	UpdateWindow(hWnd3);
#ifdef WINDOWS
	longjmp(env,1);
#else
	exit(1);
#endif
}

static void fputs2(char *s, fileptr fp)
 /* �����file.c/file.asm�Ǽ¸�����٤� */
{
	for(; *s; s++) putc2(*s, fp);
}

static void smfheader(fileptr fp, int i) /* SMF�Υإå�����Ϥ��� */
{
	fputs2("MThd", fp); /* SMF Header */
	putc2(0, fp);   /* header length */
	putc2(0, fp);
	putc2(0, fp);
	putc2(6, fp);
	putc2(0, fp);
	putc2(i, fp);   /* format 0 or 1 */
	putc2(0, fp);
	fgetpos2(fp, &trkno);
	putc2(1, fp);  /* �ȥ�å���(�ǽ��1��񤤤Ƥ���) */
	putc2((char)(timebase >> 8), fp); /* timebase */
	putc2((char)timebase, fp);
}

/* SMF�γƥȥ�å��Υإå�����Ϥ��� */
void smftrkheader(fileptr fp, Fpos_t *trsize)
{
	fputs2("MTrk", fp); /* Track Header */
	fgetpos2(fp, trsize);
	fseek2(fp, 4L, SEEK_CUR); /* �ȥ�å���������񤭹����� */
	fgetpos2(fp, &lastlenpos);
	lastlen = 0;
	putc2(0, fp); /* �ǽ�Υ��ƥåץ����� */
}

/* SMF�γƥȥ�å��ν�λ���٥�Ȥ���Ϥ��� */
static void smftrkend(fileptr fp, Fpos_t *trsize)
{
	long j;
	putc2(0xff, fp);
	putc2(0x2f, fp);
	putc2(0, fp);
	fgetpos2(fp, &trkend);
	j = (long)trkend - (long)(*trsize) - 4; /* ���֤� -4 �Ǥ����Ȼפ� */
	fsetpos2(fp, trsize);
	putc2((char)(j >> 24),fp); /* ����ΰ��֤˥ȥ�å���������񤭹��� */
	putc2((char)(j >> 16),fp);
	putc2((char)(j >> 8), fp);
	putc2((char)j,		  fp);
	fsetpos2(fp,&trkend);
}

static void puttitle(char *title, char *copyright)
{
	putc2(0xff, fp3);
	putc2(3, fp3);
	if(title == NULL) title = "";
	putc2(strlen(title), fp3);
	fputs2(title, fp3);

	putc2(0, fp3);
	putc2(0xff, fp3);
	putc2(2, fp3);
	if(copyright == NULL) copyright = "";
	putc2(strlen(copyright), fp3);
	fputs2(copyright, fp3);
}

extern int scan_flag; /* used in  long gettrack(void) */
extern int ichi; /* =1 �����ä����ɤ����Υե饰 */
extern int tempo_master;

extern int alloc_tmap(void), alloc_master_step(void),
	   alloc_mmlproc(void), alloc_keyproc(void);

/*
  mml��mid���Ѵ����롥
  �ե�����μ�갷����SMF�إå��ν��ϡ��ʤɤ��˹Ԥ���

  fp2�����̤Υȥ�å��Υǡ�����񤭡�fp3�˥ƥ�ݥޥåפ�񤯡�
  ��ǡ�fp3�θ���fp2�򤯤äĤ���(close3()���Ԥʤ�)��
  fp2�ϡ�fmalloc()�ǥ���������ݤ����(�ե�����ϥ����ץ󤵤�ʤ���)
  �ʤ���fp1, fp2�ϥե�����ϥ����ץ󤵤�Ƥʤ���ΤΡ�fgetc2,fputc2�ʤɤ�
  fp0,fp3��Ʊ�ͤ˻Ȥ��롥fp1, fp2�ϡ�ffree(fp2)�ʤɤȤ�äơ���ʬ�ǥ����
  �������ʤ��Ȥ����ʤ���
  */
static void analyze(void)
{
	int i;
	timebase = 48;		/* ������١�������� */
	trknum = 0;		/* �ȥ�å��� */
	scan_flag = 0;
	ichi = 0;

	/* alloc data area */
	if(alloc_tmap() < 0 || alloc_master_step() < 0 ||
	   alloc_mmlproc() < 0 || alloc_keyproc() < 0){
		fprintf(STDERR, err_mem);
	 	fclose2(fp0);
		remove_file_and_owari0();
	}

	if((fp1 = fmalloc()) == NULL){
		wsprintf(Msg,"ERROR! Cannot allocate memory (for fp1)\n");
		strcat(text,Msg);
		fclose2(fp0);
		remove_file_and_owari0();
	}

	getsp(NULL, fp0, fp1);	/* #���ޥ�ɤμ��� fp0 -> fp1 �˽񤭹��ޤ�� */
	fclose2(fp0);		/* ����MML�ե�����(fp0)���Ĥ��� */

	/* ��ꤿ���ʤ���... */

	fseek2(fp1, 0L, SEEK_SET);	/* �ե��������Ƭ�˰�ư */
	if((fp2 = fmalloc()) == NULL ){
		wsprintf(Msg,"ERROR! Cannot allocate memory (for fp2)\n");
		strcat(text,Msg);
		remove_file_and_owari1();
	}

	smfheader(fp3, 1);		/* SMF�Υإå�����Ϥ��� */

	init_all_macros();		/* �ޥ���ν���� */
	scanmacro();			/* �ޤ����ޥ����������Ƥ��� */
	smftrkheader(fp3, &tposize);	/* �ƥ�ݥޥåפΥإå����� */
	puttitle(title, copyright);	/* �����ȥ���ϡ�
					  �Ǹ�˥��ƥåץ������񤭹��ޤʤ� */
	trktop[0] = tposize + 4;	/* �ƥ�ݥޥåפΥإå���ľ��ΰ��� */


	for(i = 0; i < numberof(track_map); i++) track_map[i]=0;

	for(talf = 0; talf < 26; talf++){
		if (talf>0 && track_map[talf]==0 && track_map[26]==0) continue;
		tnum=0; /* ��ȥ�å�0[A��Z]��ޤ������å����� */
		clear_master_step();
		fseek2(fp1, 0L, SEEK_SET); /* �ե��������Ƭ�˰�ư */
		if(converttrk() != -1){ /* ��ȥ�å�0[A��Z]��¸�ߤ����� */
			smftrkend(fp2, &trksize); /* �ȥ�å��ν�λ���٥�Ƚ��� */
			trktop[trknum] = trksize + 4; /* �إå���ľ��ΰ��� */
			for (tnum=1;tnum < 10 ; tnum++) {
				if(track_map[tnum*32+talf]==0 && track_map[tnum*32+26]==0)
					continue; /* ��°�ȥ�å���¸�ߤ��ʤ� */
				fseek2(fp1, 0L, SEEK_SET); /* �ե��������Ƭ�˰�ư */
				if(converttrk() != -1){
					smftrkend(fp2, &trksize); /* �ȥ�å��ν�λ���٥�Ƚ��� */
					trktop[trknum] = trksize + 4; /* �إå���ľ��ΰ��� */
				}
			}
		}
	}

	write_tmap();
	smftrkend(fp3, &tposize); /* �ƥ�ݥȥ�å��ν�λ���٥�Ƚ��� */
	fsetpos2(fp3, &trkno);	  /* �ȥ�å�����񤭹�����˰�ư���� */
	trknum++;		  /* �ȥ�å����˥ƥ�ݥޥåפ�ʬ��­�� */
	if(trknum >= MAXTRKNUM){
		fprintf(STDERR, "ERROR! Too many tracks\n");
		remove_file_and_owari();
	}
	putc2(trknum, fp3);		/* �ȥ�å�����񤭹��� */
	/* fseek2(fp3, 2L, SEEK_CUR); */ /* ������١�����ʬ�򥹥��åפ��� */
	if(fmat == 0){
		format1to0();
		fclose4(fp3, fp4);	/* fp3�ξ���fp4��񤯡�fp3�򥯥��� */
		ffree(fp4);		/* fp4�Υ����������� */
	}else{
		fclose3(fp3, fp2);	/* fp3�θ��fp2�򤯤äĤ��롣fp3�򥯥��� */
	}
	ffree(fp2);			/* fp2�Υ����������롥 */
	free_all_macros();		/* �ޥ����ѥ����������� */
}

 /* �ե����뤫��1�Լ�������������\n�������֤���^Z�ʸ��\r�Ϥ����ʳ��ǽ��
    ���顼(������­)��-1��EOF��0�����ｪλ��1�֤� */
static int getLine(prepro_linebuf *lbuf, int i)
{
	int c, iorg = i;

	for(;;){
		if(i >= lbuf->amount){
			lbuf->amount = i + 80;
			lbuf->buf = realloc(lbuf->buf, lbuf->amount);
			if(lbuf->buf == NULL) return -1;
		}
		switch(c = getc2(lbuf->fpi)){
		case '\r':
			continue; /* �����ʳ��Ǽ��ʧ�ä��㤨! */
		case '\n':
			lbuf->buf[i] = '\0';
			lbuf->lineno++;
			return 1;
		case '\032': /* ^Z */
#ifdef BCC
			while(EOF != getc2(lbuf->fpi));
			 /* file.asm�ˤ�fseek2(fp, pos, SEEK_END)���¸�����Ƥʤ� */
#else
			fseek2(lbuf->fpi, 0L, SEEK_END);
#endif
			/* FALLTHROUGH */
		case EOF:
			lbuf->buf[i] = '\0';
			return i!=iorg ? (lbuf->lineno++, 1) : 0;
		default:
			lbuf->buf[i++] = c;
		}
	}
}

static char *geteos(prepro_linebuf *lbuf, char *p, int flg)
/* ��" "�פǰϤޤ��ʸ����κǸ�Ρ�"�פΰ��֤��֤�
    p�ˤϺǽ�Ρ�"�פμ��ΰ��֤�Ϳ���Ƥ���
    ��³�Ԥξ�硢��³�ԥޡ������񤭤��Ƽ��ιԤ��ɤ�lbuf->pending_eols��
    ��"�פ������ʤ�����\�פ��Ѥʤɥ��顼�ʤ�NULL
    flg����0�ʤ��\�פβ���Ԥä�ʸ�����p���񤭤�����������'\0'��
    ���롣���ξ���³�Ԥΰ����Ϥ��ʤ�
    lbuf��NULL�ξ����³�Ԥΰ����Ϥ��ʤ� */
{
	char *q; /* flg��0�ξ���ư���ʤ� */
	char code;
	int i;

	for(q = p;;){
		switch(*p){
		case '\0':
			return NULL;
		case '"':
			if(flg) *q = '\0';
			return p;
		case '\\':
			++p;
			if(!flg){
				if(!*p){
					if(lbuf == NULL) return NULL;

					i = --p - lbuf->buf;
					switch(getLine(lbuf, i)){
					case -1:
						prepro_nomem(); /* ������­�� */
					case 0:
						prepro_illcont(lbuf);
					}
					p = lbuf->buf + i;
					lbuf->pending_eols++;
				} else p++;
			} else {
				if(*p == 'x'){
					for(++p, code = 0, i = 0; i < 2; i++, p++){
						if(!is_xdigit(*p)) break;
						code = (code << 4) | xtoi(*p);
					}
					if(!i) return NULL;
				} else
				if(is_octal(*p)){
					for(code = 0, i = 0; i < 3; i++, p++){
						if(!is_octal(*p)) break;
						code = (code << 3) | dtoi(*p);
					}
				} else {
					code = escchr(*p);
					p++;
				}
				*q++ = code;
			}
			break;
		default:
			if(mskanji && ismskanji1(*p)){
				if(flg) *q++ = *p;
				p++;
				if(!*p) return NULL;
			}
			if(flg) *q++ = *p;
			p++;
		}
	}
}

 /* �ɤ߹�����Ԥ��������Ƭ����������֤�
    ��³�Ԥ�����з�³�ԥޡ���������1�ԤˤޤȤ�lbuf->pending_eols������ */
static int getLine_cooked(prepro_linebuf *lbuf)
{
	int i, j, flg;
	char *p, *q;

	if(lbuf->pending_eols){ /* lbuf->buf��1�ʾ���ΰ����ݺѤߤΤϤ� */
		lbuf->pending_eols--;
		*lbuf->buf = '\0';
		return 1;
	}
	switch(getLine(lbuf, 0)){
	case -1:
		return -1;
	case 0:
		if(lbuf->inside_comment && !classic_behavior){ /* ��᤬�Ĥ��Ƥʤ� */
			lbuf->lineno = lbuf->inside_comment;
			prepro_error("ERROR! comment not closed", lbuf);
		} else {
			return 0;
		}
	default:
		p = q = lbuf->buf, flg = 1;
		for(;;){
			if(lbuf->inside_comment){
				if(*p == '*' && p[1] == '/'){
					p += 2;
					lbuf->inside_comment = 0;
					continue;
				} else
				if(*p){
					p++;
					continue;
				}
			} else {
				if(flg && is_space(*p)){ /* ��Ƭ���� */
					if(classic_behavior){
						*p = '\0'; /* ��᰷�� */
					} else {
						p++; continue; /* ̵�� */
					}
				} else if(*p == ';'){
					*p = '\0';
				} else if(*p == '/' && p[1] == '*'){
					p += 2;
					lbuf->inside_comment = lbuf->lineno;
					continue;
				} else if(*p == '\\' && !p[1]){
					i = p - lbuf->buf, j = q - lbuf->buf;
					switch(getLine(lbuf, i)){
					case -1:
						return -1;
					case 0:
						prepro_illcont(lbuf);
					default:
						p = lbuf->buf + i, q = lbuf->buf + j;
						break;
					}
					lbuf->pending_eols++;
					continue;
				} else if(*p == '"'){
					char *r;

					i = p - lbuf->buf, j = q - lbuf->buf;
					if(NULL == (r = geteos(lbuf, p+1, 0)))
						prepro_illstring(lbuf);
					 /* geteos()����³�Ԥ��ɤ��ǽ��������ΤϤ������� */
					p = lbuf->buf + i, q = lbuf->buf + j;
					while(p < r) *q++ = *p++;
				}
			}
			flg = 0;
			if(!(*q++ = *p++)) break;
		}
		return 1;
	}
}

static void prepro_error(char *s, prepro_linebuf *lbuf)
{
	char *curfile;

	strcat(text, s);
	curfile = lbuf->virt_curfile != NULL ? lbuf->virt_curfile : lbuf->curfile;
	if(curfile != NULL){
		wsprintf(Msg, " in line %d. <%s>\n", lbuf->lineno, curfile);
	} else {
		wsprintf(Msg, " in line %d.\n", lbuf->lineno);
	}
	strcat(text, Msg);
	remove_file_and_owari1();
}
static void prepro_illdirective(prepro_linebuf *lbuf)
{
	prepro_error("ERROR! illegal #-directive", lbuf);
}
static void prepro_illstring(prepro_linebuf *lbuf)
{
	prepro_error("ERROR! string not closed or illegal '\\' escape,", lbuf);
}
static void prepro_illcont(prepro_linebuf *lbuf)
{
	prepro_error("ERROR! continued line not exist",	lbuf);
}
static void prepro_nomem() /* �ץ�ץ�����˥�����­ */
{
	strcat(text, "ERROR! Cannot allocate memory while reading input lines\n");
	remove_file_and_owari1();
}

static void
	out_ppinfo(fileptr fpo, int lineno, char *curfile, char *virt_curfile)
{
	char numbuf[32];

	sprintf(numbuf, "# %d \"", lineno);
	fputs2(numbuf, fpo);
	if(virt_curfile != NULL) fputs2(virt_curfile, fpo);
	 else if(curfile != NULL) fputs2(curfile, fpo);
	fputs2("\"\n", fpo);
}

static char *next_nonsp(char *p)
{
	while(is_space(*p)) p++;
	return p;
}

 /* �ץ�ץ�������#�פν����ȡ��Ĥ��Ǥ��������ʸ����Ρ�"�פ��б������å�
    ���äƤ��ޤ���curfile�ϳ����ץ�ץ����Ȥζ�Ĵ�Ѥ˽��Ϥ���ե�����̾�ǡ�
    ��\�פ����������פ���Ƥ����ꤹ��Τǡ������fopen2()���Ƥ�ɬ������
    �������ʤ� */
static void getsp(char *curfile, fileptr fpi, fileptr fpo)
{
	int getstat;
	prepro_linebuf lbuf;
	/*
	prepro_linebuf lbuf = {
		NULL, 0,
		fpi, curfile, NULL,
		0, 0, 0
	};
	*/
	char *p, *q, *r;

	lbuf.buf = NULL;
	lbuf.amount = 0;
	lbuf.fpi = fpi;
	lbuf.curfile = curfile;
	lbuf.virt_curfile = NULL;
	lbuf.lineno = 0;
	lbuf.pending_eols = 0;
	lbuf.inside_comment = 0;

	while((getstat = getLine_cooked(&lbuf)) > 0){
		p = lbuf.buf;
		if(!lbuf.inside_comment && *p == '#'){ /* ��#�ġ׹� */
			p = next_nonsp(p+1);
			if(is_digit(*p)){ /* # ���ֹ� "�ե�����̾" */
				getppinfo(p, &lbuf, fpo);
			} else { /* #include "�ե�����̾" �ʤ� */
				int len;

				for(q = p; is_alnum(*q); q++);
				len = q - p;
				q = next_nonsp(q);

				if(!strncmp(p, "include", len)){
					getinclude(q, &lbuf, fpo);
					continue;
				} else
				if(!strncmp(p, r = "title", len) ||
				   !strncmp(p, r = "copyright", len)){
					gettitle(r, q, &lbuf);
					putc2('\n', fpo);
				} else
				if(!strncmp(p, "timebase", len)){
					timebase =
					 getintdirective(q, "timebase", &lbuf);
					putc2('\n', fpo);
				} else
				if(!strncmp(p, "swap", len)){
					getswap(q, &lbuf);
					putc2('\n', fpo);
				} else
				if(!strncmp(p, "german", len)){
					if(*q) prepro_illdirective(&lbuf);
					german_scale ^= 1;
					putc2('\n', fpo);
				} else
				if(!strncmp(p, "bc525", len)){
					if(*q) prepro_illdirective(&lbuf);
					backcompati = 1;
					putc2('\n', fpo);
				} else
				if(!strncmp(p, "xtempo", len)){
					tempo_master =
					  getintdirective(q, "xtempo", &lbuf);
					putc2('\n', fpo);
				} else {
					prepro_illdirective(&lbuf);
				}
			}
		} else { /* ��#�ġװʳ��ι� */
			fputs2(p, fpo);
			putc2('\n', fpo);
		}
	}
	if(getstat < 0) prepro_nomem();
/*
	if(lbuf.inside_comment){
		lbuf.lineno = lbuf.inside_comment;
		prepro_error("ERROR! comment not closed", &lbuf);
	}
*/
	free(lbuf.buf), free(lbuf.virt_curfile);
}

static void getppinfo(char *p, prepro_linebuf *lbuf, fileptr fpo)
{
	char *q;

	lbuf->lineno = atoi(p)-1;
	for(q = p; is_digit(*q); q++);
	q = next_nonsp(q);
	switch(*q++){
	case '\0':
		free(lbuf->virt_curfile), lbuf->virt_curfile = NULL;
		break;
	case '"':
		if((p = geteos(NULL, q, 0)) == NULL) prepro_illstring(lbuf);
		if(*next_nonsp(p+1)) prepro_illdirective(lbuf);

		free(lbuf->virt_curfile);
		if(!*q) lbuf->virt_curfile = NULL;
		 else if(NULL== (lbuf->virt_curfile = strnDup(q, p-q))) prepro_nomem();
		break;
	default:
		prepro_illdirective(lbuf);
	}
	out_ppinfo(fpo,	lbuf->lineno + 1, lbuf->virt_curfile, lbuf->curfile);
}

static void getinclude(char *p, prepro_linebuf *lbuf, fileptr fpo)
{
	fileptr fpinc;	/* include mml�ե����� */
	char *inclfile, *q;

	if(*p != '"') prepro_illdirective(lbuf);
	if((q = geteos(NULL, ++p, 0)) == NULL) prepro_illstring(lbuf);
	if(*next_nonsp(q+1)) prepro_illdirective(lbuf);

	if(NULL == (inclfile = strnDup(p, q-p))) prepro_nomem();
	out_ppinfo(fpo, 1, inclfile, NULL);

	(void)geteos(NULL, p, 1);
	if((fpinc = fopen2(p, "rb")) == NULL){
		wsprintf(Msg, "ERROR! cannot find include file '%s'", p);
		prepro_error(Msg, lbuf);
	}
	getsp(inclfile, fpinc, fpo);
	free(inclfile);

	out_ppinfo(fpo,	lbuf->lineno + 1, lbuf->virt_curfile, lbuf->curfile);
}

static void gettitle(char *kind, char *p, prepro_linebuf *lbuf)
{
	char *q;

	if(*p != '"') prepro_illdirective(lbuf);
	if((q = geteos(NULL, ++p, 1)) == NULL) prepro_illstring(lbuf);
	if(*next_nonsp(q+1)) prepro_illdirective(lbuf);

	if(strlen(p) > 256){
		wsprintf(Msg, "ERROR! %s too long", kind);
		prepro_error(Msg, lbuf);
	}
	if((p = strdup(p)) == NULL) prepro_nomem();
	if(*kind == 't'){ /* title */
		free(title), title = p;
	} else { /* copyright */
		free(copyright), copyright = p;
	}

	wsprintf(Msg, "%s: \"%s\"\n", kind, p);
	strcat(text, Msg);
#ifndef WINDOWS
	InvalidateRect(hWnd3, NULL, TRUE);
	UpdateWindow(hWnd3);
#endif
}

static int getintdirective(char *p, char *directivename, prepro_linebuf *lbuf)
{
	int ret;
	char *q;

	for(q = p; is_digit(*q); q++);
	if(p == q || *next_nonsp(q)) prepro_illdirective(lbuf);
	ret = atoi(p);

	wsprintf(Msg, "%s: %d\n", directivename, ret);
	strcat(text, Msg);
#ifndef WINDOWS
	InvalidateRect(hWnd3, NULL, TRUE);
	UpdateWindow(hWnd3);
#endif
	return ret;
}

static void getswap(char *p, prepro_linebuf *lbuf)
{
	for(;;){
		if(!strncmp(p, "()", 2)){
			x68k2 ^= 1;
			p = next_nonsp(p+2);
		} else
		if(!strncmp(p, "<>", 2)){
			x68k ^= 1;
			p = next_nonsp(p+2);
		} else
		if(!*p){
			break;
		} else {
			prepro_illdirective(lbuf);
		}
	}
}

static void format1to0(void)
{
	int i;
	int tr; /* �����Ѵ���Υȥ�å� */
	long ctrk[MAXTRKNUM]; /* �ƥȥ�å��ν�����Υ��٥�ȤΥ��ƥåץ����� */
	int endsw[MAXTRKNUM];
	long j; /* ���� */
	int code;
	int hend = 0; /* �Ѵ��Ѥߥȥ�å��θĿ� */
	long plus = 0;

	if((fp4 = fmalloc()) == NULL ){
		wsprintf(Msg,"ERROR! Cannot allocate memory (for fp4)\n");
		strcat(text,Msg);
		remove_file_and_owari2();
	}

	smfheader(fp4, 0);
	smftrkheader(fp4, &trksize);
	fsetpos2(fp4, &lastlenpos);

	for(i = 0; i < trknum; i++){
		if(i == 0 ){
			fsetpos2(fp3, &trktop[0]);
		}else{
			fsetpos2(fp2, &trktop[i]);
		}
		ctrk[i] = getlength(i); /* ������Υ��٥�ȤΥ��ƥåץ����� */
		if(i == 0 ){
			fgetpos2(fp3, &trktop[0]);
		}else{
			fgetpos2(fp2, &trktop[i]);
		}
		/*printf("length[%d] = %ld\n", i, ctrk[i]);*/
		endsw[i] = 0; /* �Ѵ�����λ�������ɤ��� */
		rstatus[i] = 0; /* ���˥󥰥��ơ����� */
	}

	for(;;){
		tr = 0;
		j = 99999999;
		for(i = 0; i < trknum; i++){
			if(endsw[i] != 0) continue;
			if(ctrk[i] < j){
				j = ctrk[i];
				tr = i;
			}
		}
		if(tr == 0 ){
			fsetpos2(fp3, &trktop[0]);
		}else{
			fsetpos2(fp2, &trktop[tr]);
		}
		/*printf("%ld(%d)", ctrk[tr], tr);*/
		write_length(ctrk[tr] + plus, fp4);
		plus = 0;
		for(i = 0; i < trknum; i++){
			if(endsw[i] == 0 && i != tr) ctrk[i] -= ctrk[tr];
		}
		code = get1(tr);
		/*printf("[%x]", code);*/

		if(code == EOF){
			wsprintf(Msg, "error occured while converting format 1 -> 0.\n");
			strcat(Msg, "There's no track_end event.\n");
			strcat(text,Msg);
			remove_file_and_owari2();
		}else if(code < 0xf0){
			MIDI_event(tr, code);
		}else if(code == 0xf0 || code == 0xf7){
			putc2(code, fp4);
			write_code(tr);
		}else if(code == 0xff){
			if(META_event(tr) == -1){
				/*printf("end\n");*/
				plus = lastlen;
				fsetpos2(fp4, &lastlenpos); /* ��©�ʼ��� */
				if(++hend == trknum) break;
				endsw[tr] = 1;
				continue;
			}
		}else{
			wsprintf(Msg, "error occured while converting format 1 -> 0.\n"
					 "abnormal code '%x'\n",code);
			strcat(text,Msg);
			remove_file_and_owari2();
		}
		ctrk[tr] = getlength(tr);
		/*printf("new get length for %d = %ld\n", tr, ctrk[tr]);*/
		if(tr == 0 ){
			fgetpos2(fp3, &trktop[0]);
		}else{
			fgetpos2(fp2, &trktop[tr]);
		}
	}
	putc2(0, fp4);
	smftrkend(fp4, &trksize);
}

static void MIDI_event(int x, int code)
{
	if(code < 0x80){
		putc2(rstatus[x], fp4);
	}else{
		rstatus[x] = code;
		putc2(rstatus[x], fp4);
		code = get1(x);
	}
	putc2(code, fp4);
	/*printf("<%x>",code);*/

	switch(rstatus[x] / 16){
	case 8: case 9: case 10: case 11: case 14:
		putc2(get1(x), fp4);
	}
}

static int META_event(int x)
{
	int code;

	code = get1(x);
	if(code == 0x2f){ /* track end */
		code = get1(x);
		if(code != 0){
			wsprintf(Msg, "error occured while converting format 1 -> 0.\n"
					 "abnormal code 'FF 2F %x'\n",code);
			strcat(text,Msg);
			remove_file_and_owari2();
		}
		return -1;
   	}
	putc2(0xff, fp4);
	putc2(code, fp4);
	write_code(x);
	return 0;
}

static void write_code(int x)
{
	int i;
	int length;

	length = get1(x);
	putc2(length, fp4);
	for(i = 0; i < length; i++) putc2(get1(x), fp4);
}

static int get1(int i)
{
	return i ? getc2(fp2) : getc2(fp3);
}

static long getlength(int x) /* get length from SMF */
{
	long i1, i2, i3, i4;
	long num;

#define j 0x80L
	i1 = get1(x);
	if(i1 >= j){
		i2 = get1(x);
		if(i2 >= j){
			i3 = get1(x);
			if(i3 >= j){
				i4 = get1(x);
				num= (i1 - j) * j * j * j + (i2 - j) *
					j * j + (i3 - j) * j + i4;
			}else num = (i1 - j) * j * j + (i2 - j) *
				j + i3;
		}else num = (i1 - j) * j + i2;
	}else num = i1;
	return num;
#undef j
}
