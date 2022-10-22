
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
static jmp_buf env;	/* エラー処理用 */
#endif

static fileptr fp0;	/* 入力ファイル(mml) */
fileptr fp1;	/* プリプロセス後ファイル(mml)を書き込む */
fileptr fp2;	/* テンポマップ以外のSMFを書き込む場所 */
fileptr fp3;	/* 出力ファイル(mid) 最初はテンポ・マップが書かれる */
#ifdef WINDOWS
static char outfile[128]; /* 出力ファイル名(変換失敗時のファイル削除用) */
#else
static char *outfile = NULL;
#endif
int timebase;           /* タイムベース 48,60,80,96,120,160,240,480 */
static Fpos_t trkno;    /* トラック数を書き込む場所 */
int trknum;             /* トラック数 */
Fpos_t trksize;         /* トラックサイズを書き込む場所 */
static Fpos_t trkend;   /* トラックの終了位置 */
static Fpos_t tposize;  /* テンポトラックのトラックサイズを書き込む場所 */
static char *title = NULL;	/* 曲のタイトル */
static char *copyright = NULL;	/* 曲の著作権情報 */
Fpos_t lastlenpos;       /* 最後に音長を書き込んだ場所へのポインタ */
long lastlen;            /* 最後に書き込んだ音長の値 */
int tnum; /* パート名．  トラックを表す 0A 1B 2C などの 0,1,2 */
int talf; /* トラック名．トラックを表す 0A 1B 2C などの A,B,C */
int trans = 0; /* オプションスイッチTによる転調値 */
int x68k = 0;  /* <, > を逆にするスイッチ */
int x68k2 = 0; /* (, ) を逆にするスイッチ */
int german_scale = 0; /* 音名をドイツ流のCDEFGAHにするスイッチ */
char track_map[320];	/* MKR追加 */
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

static void smfheader(fileptr, int); /* SMFのヘッダを出力する */
void smftrkheader(fileptr, Fpos_t *); /* SMFトラックヘッダ出力 */
static void smftrkend(fileptr, Fpos_t *);
				/* SMFトラック終了イベント出力 */
extern void write_length(long, fileptr);
static void puttitle(char *, char *);
static void analyze(void);
extern void clear_master_step(void);
extern int alloc_master_step(void);

 /* プリプロセス時に使う構造体 */
typedef struct {
	char *buf;
	int amount;

	fileptr fpi;
	char *curfile, *virt_curfile;
	 /* 外部プリプロセサとの協調用に出力するファイル名。「\」がエスケープ
	    されていたりするので、fopen2()しても必ずしも成功はしない */
	int lineno, pending_eols;
	int inside_comment; /* 「／*」〜「*／」の中ならその開始行番号、でないと0 */
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
	 /* nargvに (*argvp)[0], envの分割, (*argvp)[1〜(*argcp-1)], NULL
	    を順に入れる nargvのサイズは(*argcp+1 + envの分割個数)だけ必要 */
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
 /* Add Nide; オプション文字列optを解析し、*argcpと*argvpを進める */
{
	int c;

	while((c = *opt++) != '\0'){
/* CASE_INSENSITIVE_OPTSがdefineされていると、オプションの大文字を小文字と
   同一視 */
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
	 /* completely rewritten by Nide; まず先頭の「-」で始まる引数を
		オプションと見なして解析 */
	for(--argc, ++argv; argc && **argv == '-' && argv[0][1];
		options(*argv + 1, &argc, &argv));

	 /* その次の引数がMMLファイル名 */
	if(!argc--) sw();
	srcfile = *argv++;

/* USR_NONMINUS_OPTSが定義されていれば、MMLファイル名の次の引数が「-」で
   始まらない2文字以下の場合にそこ以降を無条件にオプション引数扱いする */
#ifdef USE_NONMINUS_OPTS
	if(!(argc && **argv != '-' && strlen(*argv) <= 2))
#endif
	{
		 /* その後に「-」で始まる引数がある間はオプションとして解析 */
		for(; argc && **argv == '-' && argv[0][1];
			options(*argv + 1, &argc, &argv));
		 /* その次に引数があればそれがMIDファイル名。outfileはfree()しないし
		    argvに使われている文字列も書き換えないので、outfileとして*argvを
		    直接使って支障ない */
		if(argc) argc--, outfile = *argv++;
	}

	 /* 残る引数はオプション。但しUSE_NONMINUS_OPTSが定義されていない場合は
	    「-」で始まらないオプションを許さない */
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
	_fmode = O_BINARY; /* for BCC グローバル変数 _fmode に値を代入 */
#endif

	/* ディレクトリ名中の「.」を拡張子の始まりと誤認するのを防止のため */
#if !defined(MSDOS) && !defined(EGCSW32)
	if((base = strrchr(srcfile, '/')) == NULL) base = srcfile;
#else
	{ /* '/'も'\\'もパス区切り。さらにファイル名の漢字も考慮 */
		char *p;
		for(base = p = srcfile; *p; p++){
			if(*p == '/' || *p == '\\' || *p == ':') base = p;
			else if(ismskanji1(*p) && p[1]) p++;
		}
	}
#endif

	/* mml file open -------- >>>>>>>>>> */
#ifndef BCC /* BCCではファイル関連の関数がfile.asmで実現されているため
	       fdopen2()が作れない */
	if(!strcmp(srcfile, stdio_name)){ /* read stdin */
		fp0 = fdopen2(0, "rb");
		if(fp0 == NULL){
			fprintf(STDERR, err_mem);
			owari();
		}
	} else /* 拡張子なしでもそのファイル名を先に試すように変更した(Nide) */
#endif
	if((fp0 = fopen2(srcfile, "rb")) == NULL) do {
		if(strchr(base, '.') == NULL){ /* 拡張子なしなら「.mml」を付け再try */
			char *file1;

			if((file1 = malloc(strlen(srcfile) + sizeof(MML_EXT))) == NULL){
				fprintf(STDERR, err_mem);
				owari();
			}
			sprintf(file1, "%s%s", srcfile, MML_EXT);
			fp0 = fopen2(file1, "rb");
			free(file1);
			if(fp0 != NULL) break; /* 成功。エラーをスキップ */
		}
		fprintf(STDERR, "ERROR! Unable to open %s\n", srcfile);
		owari();
	} while(0);
	/* <<<<<<<<< -------- mml file open */

	/* mid file open -------- >>>>>>>>>> */
	if(outfile == NULL){ /* まだ出力ファイル名が決まっていなければ */
#ifndef BCC
		if(!strcmp(srcfile, stdio_name)){
# ifdef MSG_TO_STDOUT
			fprintf(STDERR, "ERROR! No output file specified " \
				"while input is stdin.\n");
			fclose2(fp0);
			owari();
# else
			outfile = srcfile; /* 出力先もstdout */
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
		 /* Windows2000には、エクスプローラから.midファイルを簡易演奏
		    するとそのファイルをwrite openできなくなるバグがある??
		    対策として一度削除して再試行してみるが、有効だろうか */
/* #ifdef EGCSW32 */ /* 有効ではなかったのでとりやめ */
#if 0
		if(access(outfile, W_OK) == 0 && unlink(outfile) == 0 &&
		   (fp3 = fopen2(outfile, "wb")) != NULL){
			break; /* 成功。エラーをスキップ */
		}
		 /* Symbolic linkのあるOSではこの措置をしてはいけない(Symbolic
		    linkを削除してしまう)。また、まともなファイルアクセス権限の
		    管理をしているOSでもこの措置はしない方がいいと思う(ファイル
		    の権限が元と同じでなくなる)。*/
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
	return 0; /* 正常終了 */
}
#endif

#ifdef WINDOWS
/* . */
/* file1 は 入力 mml ファイルの名前 */
/* file2 は 出力 mid ファイルの名前 */
/* file2[0] == '\0' ならば，出力ファイル名を自動生成する */
/* 自動生成の例 foo.mml が入力ならば foo.mid  を出力ファイルとする */
/* . */
/* staticをつけると Windowsプログラムから呼び出せなくなる */
int mml_smf(char *file1,char *file2,int xx,int ff,int oo,int tt)
{
	int i;
	char *l;

	if(setjmp(env)){
		return 1; /* longjmp から戻ったとき実行される */
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

	trans=tt;   /* トランスポーズ */
	fmat=ff;    /* フォーマット */
	x68k = xx;  /* >と<を入れ換えるオプション */
	x68k2 = oo; /* )と(を入れ換えるオプション */

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
		while(strchr(l+1,'.') != NULL){ /* path 名に複数"."が */
			l=(char *)strchr(l+1,'.');  /* ある場合の処理 */
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
	return 0; /* 正常終了 */
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

/* すべてのファイルをオープンした後で，エラーが出た場合に，
   この関数が呼ばれる．
   この関数は，mmlproc.cのmml_err()などから呼ばれる． */
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
 /* 本来はfile.c/file.asmで実現するべき */
{
	for(; *s; s++) putc2(*s, fp);
}

static void smfheader(fileptr fp, int i) /* SMFのヘッダを出力する */
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
	putc2(1, fp);  /* トラック数(最初は1を書いておく) */
	putc2((char)(timebase >> 8), fp); /* timebase */
	putc2((char)timebase, fp);
}

/* SMFの各トラックのヘッダを出力する */
void smftrkheader(fileptr fp, Fpos_t *trsize)
{
	fputs2("MTrk", fp); /* Track Header */
	fgetpos2(fp, trsize);
	fseek2(fp, 4L, SEEK_CUR); /* トラックサイズを書き込む場所 */
	fgetpos2(fp, &lastlenpos);
	lastlen = 0;
	putc2(0, fp); /* 最初のステップタイム */
}

/* SMFの各トラックの終了イベントを出力する */
static void smftrkend(fileptr fp, Fpos_t *trsize)
{
	long j;
	putc2(0xff, fp);
	putc2(0x2f, fp);
	putc2(0, fp);
	fgetpos2(fp, &trkend);
	j = (long)trkend - (long)(*trsize) - 4; /* たぶん -4 でいいと思う */
	fsetpos2(fp, trsize);
	putc2((char)(j >> 24),fp); /* 所定の位置にトラックサイズを書き込む */
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
extern int ichi; /* =1 があったかどうかのフラグ */
extern int tempo_master;

extern int alloc_tmap(void), alloc_master_step(void),
	   alloc_mmlproc(void), alloc_keyproc(void);

/*
  mmlをmidに変換する．
  ファイルの取り扱い，SMFヘッダの出力，などを主に行う．

  fp2に普通のトラックのデータを書き、fp3にテンポマップを書く。
  後で、fp3の後ろにfp2をくっつける(close3()が行なう)．
  fp2は，fmalloc()でメモリだけ確保される(ファイルはオープンされない．)
  なお，fp1, fp2はファイルはオープンされてないものの，fgetc2,fputc2などが
  fp0,fp3と同様に使える．fp1, fp2は，ffree(fp2)などとやって，自分でメモリを
  解放しないといけない．
  */
static void analyze(void)
{
	int i;
	timebase = 48;		/* タイムベース初期値 */
	trknum = 0;		/* トラック数 */
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

	getsp(NULL, fp0, fp1);	/* #コマンドの取得 fp0 -> fp1 に書き込まれる */
	fclose2(fp0);		/* 元のMMLファイル(fp0)は閉じる */

	/* やりたくないが... */

	fseek2(fp1, 0L, SEEK_SET);	/* ファイルの先頭に移動 */
	if((fp2 = fmalloc()) == NULL ){
		wsprintf(Msg,"ERROR! Cannot allocate memory (for fp2)\n");
		strcat(text,Msg);
		remove_file_and_owari1();
	}

	smfheader(fp3, 1);		/* SMFのヘッダを出力する */

	init_all_macros();		/* マクロの初期化 */
	scanmacro();			/* まず、マクロを取得しておく */
	smftrkheader(fp3, &tposize);	/* テンポマップのヘッダ出力 */
	puttitle(title, copyright);	/* タイトル出力．
					  最後にステップタイムを書き込まない */
	trktop[0] = tposize + 4;	/* テンポマップのヘッダの直後の位置 */


	for(i = 0; i < numberof(track_map); i++) track_map[i]=0;

	for(talf = 0; talf < 26; talf++){
		if (talf>0 && track_map[talf]==0 && track_map[26]==0) continue;
		tnum=0; /* 主トラック0[A〜Z]をまずチェックする */
		clear_master_step();
		fseek2(fp1, 0L, SEEK_SET); /* ファイルの先頭に移動 */
		if(converttrk() != -1){ /* 主トラック0[A〜Z]が存在する場合 */
			smftrkend(fp2, &trksize); /* トラックの終了イベント出力 */
			trktop[trknum] = trksize + 4; /* ヘッダの直後の位置 */
			for (tnum=1;tnum < 10 ; tnum++) {
				if(track_map[tnum*32+talf]==0 && track_map[tnum*32+26]==0)
					continue; /* 従属トラックが存在しない */
				fseek2(fp1, 0L, SEEK_SET); /* ファイルの先頭に移動 */
				if(converttrk() != -1){
					smftrkend(fp2, &trksize); /* トラックの終了イベント出力 */
					trktop[trknum] = trksize + 4; /* ヘッダの直後の位置 */
				}
			}
		}
	}

	write_tmap();
	smftrkend(fp3, &tposize); /* テンポトラックの終了イベント出力 */
	fsetpos2(fp3, &trkno);	  /* トラック数を書き込む場所に移動する */
	trknum++;		  /* トラック数にテンポマップの分を足す */
	if(trknum >= MAXTRKNUM){
		fprintf(STDERR, "ERROR! Too many tracks\n");
		remove_file_and_owari();
	}
	putc2(trknum, fp3);		/* トラック数を書き込む */
	/* fseek2(fp3, 2L, SEEK_CUR); */ /* タイムベースの分をスキップする */
	if(fmat == 0){
		format1to0();
		fclose4(fp3, fp4);	/* fp3の場所にfp4を書く。fp3をクローズ */
		ffree(fp4);		/* fp4のメモリを解放する */
	}else{
		fclose3(fp3, fp2);	/* fp3の後にfp2をくっつける。fp3をクローズ */
	}
	ffree(fp2);			/* fp2のメモリを解放する． */
	free_all_macros();		/* マクロ用メモリを解放する */
}

 /* ファイルから1行取得し、行末の\nを除去して返す。^Z以後や\rはこの段階で除去。
    エラー(メモリ不足)は-1、EOFは0、正常終了は1返す */
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
			continue; /* この段階で取っ払っちゃえ! */
		case '\n':
			lbuf->buf[i] = '\0';
			lbuf->lineno++;
			return 1;
		case '\032': /* ^Z */
#ifdef BCC
			while(EOF != getc2(lbuf->fpi));
			 /* file.asmにはfseek2(fp, pos, SEEK_END)が実現されてない */
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
/* 「" "」で囲まれる文字列の最後の「"」の位置を返す
    pには最初の「"」の次の位置を与えておく
    継続行の場合、継続行マークを上書きして次の行も読みlbuf->pending_eols増
    「"」が釣り合わない、「\」が変などエラーならNULL
    flgが非0なら「\」の解釈を行った文字列でpを上書きしその末尾を'\0'に
    する。この場合継続行の扱いはしない
    lbufがNULLの場合も継続行の扱いはしない */
{
	char *q; /* flgが0の場合は動かない */
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
						prepro_nomem(); /* メモリ不足… */
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

 /* 読み込んだ行の注釈内や行頭空白を除去して返す
    継続行があれば継続行マークを除去して1行にまとめlbuf->pending_eolsを増す */
static int getLine_cooked(prepro_linebuf *lbuf)
{
	int i, j, flg;
	char *p, *q;

	if(lbuf->pending_eols){ /* lbuf->bufに1以上の領域を確保済みのはず */
		lbuf->pending_eols--;
		*lbuf->buf = '\0';
		return 1;
	}
	switch(getLine(lbuf, 0)){
	case -1:
		return -1;
	case 0:
		if(lbuf->inside_comment && !classic_behavior){ /* 注釈が閉じてない */
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
				if(flg && is_space(*p)){ /* 行頭空白 */
					if(classic_behavior){
						*p = '\0'; /* 注釈扱い */
					} else {
						p++; continue; /* 無視 */
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
					 /* geteos()が継続行を読む可能性があるのはここだけ */
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
static void prepro_nomem() /* プリプロセス中にメモリ不足 */
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

 /* プリプロセス。「#」の処理と、ついでに注釈除去や文字列の「"」の対応チェック
    もやってしまう。curfileは外部プリプロセサとの協調用に出力するファイル名で、
    「\」がエスケープされていたりするので、これをfopen2()しても必ずしも
    成功しない */
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
		if(!lbuf.inside_comment && *p == '#'){ /* 「#…」行 */
			p = next_nonsp(p+1);
			if(is_digit(*p)){ /* # 行番号 "ファイル名" */
				getppinfo(p, &lbuf, fpo);
			} else { /* #include "ファイル名" など */
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
		} else { /* 「#…」以外の行 */
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
	fileptr fpinc;	/* include mmlファイル */
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
	int tr; /* 現在変換中のトラック */
	long ctrk[MAXTRKNUM]; /* 各トラックの処理中のイベントのステップタイム */
	int endsw[MAXTRKNUM];
	long j; /* 汎用 */
	int code;
	int hend = 0; /* 変換済みトラックの個数 */
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
		ctrk[i] = getlength(i); /* 処理中のイベントのステップタイム */
		if(i == 0 ){
			fgetpos2(fp3, &trktop[0]);
		}else{
			fgetpos2(fp2, &trktop[i]);
		}
		/*printf("length[%d] = %ld\n", i, ctrk[i]);*/
		endsw[i] = 0; /* 変換が終了したかどうか */
		rstatus[i] = 0; /* ランニングステータス */
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
				fsetpos2(fp4, &lastlenpos); /* 姑息な手段 */
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
