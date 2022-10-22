
/*
 *      file    name            charproc.c
 */

/*
   このファイルでは、
   トラックから１文字取ってくる処理、および、マクロの処理を行う。
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include "win.h"
#include "file.h"
#include "charproc.h"

extern void mml_err(int); /* mml2mid.c にある */

extern fileptr fp2;         /* 出力ファイル(mid) */
extern void write_length(long, fileptr);

extern fileptr fp1;

extern int cur_line;           /* 現在のMMLソース上の行番号 */
extern int tnum; /* トラックを表す 0A 1B 2C などの 0,1,2 */
extern int talf; /* トラックを表す 0A 1B 2C などの A,B,C */
extern int kloop_ptr;          /* ループネスト数 */
/* --- 以下５行ＭＫＲの追加 */
extern Fpos_t lastlenpos; /* 最後に音長を書き込んだ場所へのポインタ */
extern long lastlen;      /* 最後に書き込んだ音長の値 */
extern long tstep;
struct master_step *master_step;	/* MKR追加 */ /* Mod Nide */
int master_step_amount;
int master_count;			/* MKR追加 */
int master_count_temp, master_count_temp_prev; /* Mod Nide */
#define master_step_temp (master_step[master_count_temp].step)
extern int mskanji;

extern int head; /* 従属パートにヘッダを書いたかどうかのフラグ */
extern void write_header(void);

static int cur_macro;          /* 現在読み込み中のマクロ */
static int cur_macronum;       /* マクロの現在読み込み中の文字 */
/* static unsigned char **macros; */ /* マクロ保存用 */
typedef struct macrostruct {
	int alcsiz, uselen;	/* allocサイズと使用中長さ */
	unsigned char *mac;	/* 内容 */
} macrostruct;
static macrostruct *mcrstr;	/* マクロ保存用 */
#define macros(i) mcrstr[i].mac

int scan_flag; /* used in  int gettrack(void) */

struct {
	int line, actual_line;
	char *fname;
} ppinfo = {0, 0, NULL};


static int nexttrack(void);
static void macroloop(unsigned char *, macrostruct *);
static void getmacro(void);
static int macro(int, int);
static int track(int);
static int gettrack(void);
static int skipts(void);
static int getnum(void);

#ifdef BCC
 /* BCC以外ではfile.hで実現している 本来はBCCでもfile.asmで実現して欲しい */
static long ftell2(long fp)
{
	Fpos_t p;

	fgetpos2(fp, &p);
	return (long)p;
}
#endif

void reallocmacro(macrostruct *m, int i)
{
	if(NULL == (m->mac = (unsigned char *)realloc(m->mac, m->alcsiz = i)))
		mml_err(61);
}
#define freemacro(m) /* 指定したマクロのメモリ解放 */ \
	free((m)->mac)
#define clearmacro(m) /* 指定したマクロのクリア */ do{ \
	reallocmacro((m), 200); \
	(m)->uselen = 0; \
} while(0)
#define initmacro(m) /* 指定したマクロの初期化 */ do{ \
	(m)->mac = 0; \
	clearmacro(m); \
} while(0)
 /* (m)がcallocしたものである場合はclearmacro()で十分。自動変数である場合など
    は、最初に(m)->macをNULLにする必要があるため、initmacro()の方を使う */
#define macrosizchk(m, i) /* サイズiに足りなければ再割り当て */ do{ \
	if((i) >= (m)->alcsiz) reallocmacro((m), (i) + 100); \
} while(0)
#define macroaddchar(m, c) /* マクロに文字cを追加。必要なら再割り当て */ do{ \
	macrosizchk((m), (m)->uselen); \
	(m)->mac[(m)->uselen++] = (c); /* cは1回しか評価されない */ \
} while(0)
#define macroNaddchar(n, c) /* 番号nのマクロに文字cを追加 */ \
	macroaddchar(&mcrstr[n], (c))

void macrocat(macrostruct *to, macrostruct *from){
	 /* マクロtoの後ろにfromを追加 但しtoの末尾にNULLの自動追加はしない */
	int needed = to->uselen + from->uselen - 1;
	macrosizchk(to, needed);
	memcpy(&to->mac[to->uselen], from->mac, from->uselen - 1);
	to->uselen += from->uselen - 1;
}

/* 全マクロの解放 */
void free_all_macros(void)
{
	int i;

	for(i = 0; i < 260; i++) free(macros(i));
	free(mcrstr);
}

/* 全マクロを初期化する */
void init_all_macros(void)
{
	mcrstr = (macrostruct *)calloc(10 * 26,	sizeof(macrostruct));
	if(NULL == mcrstr) mml_err(61);
}

/* マクロ内のループを展開する */
#define MAX_MACROLOOP_NEST_LEVEL 16
static void macroloop(unsigned char *mcr_src, macrostruct *mcr_dst)
{
	int inner_string_p = 0;
	int count;
	struct {
		unsigned char *start_pos;	/* mcr_src中のループ開始位置 */
		int loop_count;			/* ループを回った回数 */
	} mloop[MAX_MACROLOOP_NEST_LEVEL];
	int mloop_ptr = 0;
	int x, xx;

	for(;;){
		xx = *mcr_src++;
		if(inner_string_p){ /* 文字列内では「[ ]」の展開はしない(Nide) */
			 /* マクロ内で文字列が閉じてないエラーのチェックは
			    getmacro()内でmacroloop()を呼ぶ前に済んでるはず */
			switch(xx){
			case '"':
				break;		/* 一般と同じ扱い */
			case '\\': /* 「\n」などの処理はあとでやる ここでは
				 「\」の次を特殊文字扱いしない処理だけで十分 */
				goto esc;
			default:
				if(mskanji && ismskanji1(xx)){
				esc:
					macroaddchar(mcr_dst, xx);
					xx = *mcr_src++;
				}
				macroaddchar(mcr_dst, xx);
				continue;
			}
		}
		switch(xx){
		case '[':
			if(mloop_ptr >= MAX_MACROLOOP_NEST_LEVEL) mml_err(46);

			mloop[mloop_ptr].start_pos = mcr_src; /* ループ開始地点のセット */
			mloop[mloop_ptr].loop_count = -1;
				/* ループを回る回数.最初は-1にしておく */
			mloop_ptr++;
			break;
		case '/':
		case ':':	/* ループアウト */
			if(!mloop_ptr) mml_err(66); /* ループ外だった */
			if(mloop[mloop_ptr - 1].loop_count == 1){
				int loop_inner_string_p = 0;

				if(xx == '/') mml_warn(2); /* obsoleteの警告 */

				count = 0;
				for(;;){
					x = *mcr_src++;	
					if(x == '\0')
						mml_err(48); /* ]がない */
					if(x == '"'){
						loop_inner_string_p ^= 1; continue;
					}
					if(loop_inner_string_p){
						if(x == '\\' || (mskanji && ismskanji1(x))){
							if(!*mcr_src++) mml_err(48);
						}
						continue;
					}
					else if(x == '[') count++;
					else if(x == ']'){
						for(;;){ /* 数値を飛ばす */
							x = *mcr_src++;
							if(!is_digit(x)){
								mcr_src--;
								break;
							}
						}
						if(count == 0) break;
						count--;
					}
				}
				mloop_ptr--;	/* ループを抜けたので、ネスト数を減らす */
			}
			break;
		case ']':	/* 繰り返しの終わり。 */
			if(mloop_ptr == 0) mml_err(47); /* マクロ内のループが変 */
			count = 0;
			for(;;){
				x = *mcr_src++;
				if(!is_digit(x)){
					mcr_src--;
					break;
				}
				count = count * 10 + dtoi(x);
			}
			if(count == 0) count = 2;
			 /* まだループを回ってない場合、回る回数をセット */
			if(mloop[mloop_ptr - 1].loop_count == -1)
				mloop[mloop_ptr - 1].loop_count = count;
			if(--mloop[mloop_ptr - 1].loop_count == 0){
				mloop_ptr--;
				break;
			}
			mcr_src = mloop[mloop_ptr - 1].start_pos; /* ループの先頭に飛ぶ */
			break;
		case '\0':
			macroaddchar(mcr_dst, '\0');
			if(mloop_ptr != 0) mml_err(48); /* ]がない */
			return;
		case '"':
			inner_string_p ^= 1; /* FALLTHROUGH */
		default:
			macroaddchar(mcr_dst, xx);
			break;
		}
	}
}

/* マクロのスキャン */
void scanmacro(void)
{
	int code;

	 /* マクロ内の行番号保存にもcur_lineを使うことにし、別な変数を
	    用意するのはやめた */
	for(cur_line = 0;;){
		if(!cur_line || (code = getc2(fp1)) == '\n'){
			cur_line++;
			code = getc2(fp1);
			if(code == EOF){
				break;
			} else
			if(code == '$'){
				getmacro();
			} else {
				ungetc2(fp1);
			}
		} else
		if(code == EOF) break;
	}
}

/* scanmacro()から呼ばれる マクロの定義内容を保存 */
static void getmacro(void)
{
	int code, inner_string_p;
	int cm; /* 処理中のマクロ */
	macrostruct mcr, mcr2;
	unsigned char *mcrptr;

	 /* マクロの内部番号を求めておく */
	code = getc2(fp1);
	cm = 0;
	if(is_digit(code)){
		cm = dtoi(code) * 26;
		code = getc2(fp1);
	}
	if(!is_lower(code)) mml_err(5);
	cm += code - 'a';

	 /* マクロの定義内容をmcrに入れる */
	initmacro(&mcr); /* 最後には解放すること */
	for(inner_string_p = 0;;){
		code = getc2(fp1);
		if(code == EOF){
			break;
		} else
		if(code == '\n'){
			ungetc2(fp1); /* これがないとダメ */
			 /* ここで既読ポインタを1つ戻し、改めてscanmacro()の
			    最外forループ冒頭で'\n'を読んでcur_lineを増やす */
			break;
		} else
		if(code == '"'){
			inner_string_p ^= 1;
		} else
		if(inner_string_p &&
		   (code == '\\' || (mskanji && ismskanji1(code)))){
			macroaddchar(&mcr, code); /*「\n」などの処理は後 */
			code = getc2(fp1);
			if(code == '\n' || code == EOF) mml_err(62);
			/* マクロ内で文字列が閉じてない…起こらないはずだけど*/
		}
		macroaddchar(&mcr, code);
	} /* この関数ではfp1を読むのはここまで */
	macroaddchar(&mcr, '\0');

	 /* マクロ内のループを展開する mcr2は最後には解放すること */
	initmacro(&mcr2);
	macroloop(mcr.mac, &mcr2);
	freemacro(&mcr);
				
	 /* 2重定義でも(つまり既にmacros(cm)!=NULLでも)エラーとはしない */
	clearmacro(&mcrstr[cm]);

	 /* mcr2内のマクロを展開しつつmcrstr[cm]へコピー
	    但し文字列内ではマクロ展開はしない(Nide) */
	for(inner_string_p = 0, mcrptr = mcr2.mac;;){
		code = *mcrptr++;
		if(code == '\0') break;
		if(code == '"'){
			inner_string_p ^= 1;
		} else
		if(inner_string_p &&
		   (code == '\\' || (mskanji && ismskanji1(code)))){
			macroNaddchar(cm, code); /* 「\n」などの処理は後 */
			macroNaddchar(cm, *mcrptr++);
			continue;
		} else
		if(code == '$' && !inner_string_p){
			int cm2 = 0;

			code = *mcrptr++;
			if(is_digit(code)){
				cm2 = dtoi(code) * 26;
				code = *mcrptr++;
			}
			if(!is_lower(code))
				mml_err(44);
			cm2 += code - 'a';
			if(macros(cm2) == NULL)
				mml_err(45); /* 未定義マクロを呼んだ */
			if(cm == cm2)
				mml_err(63); /* マクロが再帰している */

			macrocat(&mcrstr[cm], &mcrstr[cm2]);
			continue;
		}
		macroNaddchar(cm, code);
	}
	macroNaddchar(cm, '\0'); /* マクロ文字列の終端コードはNULL */
	freemacro(&mcr2);

	/* printf("macro number %d = '%s'\n", cm, macros(cm)); */
}

/*
   tnum, talfに対応するトラックから1文字取ってくる．
   取ってこれない場合(トラックの最後に来た場合)は -1 を返す．
   スペース、タブ、!などの処理もここでやる。
   マクロの処理もここでやる。

   x == 0 １文字取った後にポインタ移動。
   x == 1 １文字取った後にポインタは移動しない。
   x == 2 ポインタ移動だけ。ただし、直前でgetbyte(1)が呼ばれている必要がある。

   inner_string_pが非0ならスペース無視やマクロ処理などは一切やらない(Nide)
   */
int Getbyte(int x, int inner_string_p)
{
	static int f = 0;	/* 次の行を調べに行くかどうかのフラグ 0〜2 */
	int i, code;		/* 汎用 */

	if(x == -1){f = 0; return 0;}

	for(;;){
		switch(f){
		case 0:		/* 次のトラックの先頭に移動する場合 */
			i = nexttrack();
			if(i == -1) return -1;
			f = 1;
			 /* FALLTHROUGH */
		case 1:		/* トラックの中から１文字取ってくる場合 */
			if(x == 2){
				fseek2(fp1, 1L, SEEK_CUR);
				return 0;
			}
			switch(code = track(inner_string_p)){
			case -1: /* EOFがあった */
				f = 0;	/* 次のトラック名のために f = 0 に戻しておく */
				return -1;
			case -2: /* 行末に来た */
				f = 0;
				continue;
			case -3: /* マクロに来た */
				f = 2;
				continue;
			}
			/* トラック内から１文字取ってきた */
			if(x == 1) ungetc2(fp1);
			break;
		default:	/* case 2: マクロ内から１文字取ってくる場合 */
			if(x == 2){
				cur_macronum++;
				return 0;
			}
			code = macro(x, inner_string_p);
			if(code == -1){	/* マクロ内の文字が尽きた */
				f = 1;
				continue;
			}
			break;
		}

		 /* トラックあるいはマクロから1文字取ってきたあとポインタを
		    移動したい場合の共通処理 */
		if(x == 0){
			if (tnum==0) { /* 主トラックの場合 */
				/* 従属トラックのステップ合わせ */
				for (i=master_count-1;i>=0;i--) {
					if (master_step[i].step == -2) {
						master_step[i].step = tstep;
						 /* master_step[i].linenumは設定済みのはず */
					} else break;
				}
			} else { /* 従属トラックの場合 */
				if (master_count_temp>=0) { /* 従属トラックの先頭 */
				/* tstep は1個前の(?)ステップ
				   master_step_tempは従属トラックの先頭のステップ
				   (master_step_temp == master_step[master_count_temp].step) */

					/* if(master_step_temp<tstep) mml_err(37); */
					if(head == 0){
						head = 1;
						write_header(); /* 従属トラックのヘッダを書く */
					}
					/* 主トラックの次に同じ従属トラックが何度も続いていても
					   そのたびに位置合わせはしない
					   master_count_temp_prevは前回位置合わせしたときの
					   master_count_tempの値 */
					if(master_count_temp_prev < master_count_temp){
						master_count_temp_prev = master_count_temp;
						tstep -= lastlen;
						fsetpos2(fp2, &lastlenpos);
						write_length(master_step_temp-tstep, fp2);
					}
					master_count_temp = -1;
				}
			}
		}
		return code; /* トラック内あるいはマクロ内から1文字取ってこれた */
	}
}

/*
   次のトラックの先頭に移動する。
   次のトラックがない場合は、-1を返す。
   */
static int nexttrack()
{
	int li;

	for(;;){
		li=gettrack();
		switch(li){
		case -1:		/* EOF */
			return -1;
		case -2:		/* 主トラック */
			break;
		case -3:		/* 従属トラックだが対応する主トラックなし */
			mml_err(36);
		default:		/* 従属トラックを主トラックに合わせよう
					   としている master_step[li].stepが
					   合わせるべきステップ */
			master_count_temp=li;	/*ここでは変数に格納するだけ。*/
			break;
		}

		if(skipts() != -1) return 0; /* その行に何かあった */
		/* その行に何もなければ続行 */
	}
}

/*
   マクロから1文字取ってくる。
   マクロ内の文字が尽きたとき、-1を返す。

   inner_string_pが非0なら空白などの無視扱いはしない(Nide)
   */
static int macro(int x, int inner_string_p)
{
	int code;

	for(;;){
		code = macros(cur_macro)[cur_macronum];
		if(code == '\0') return -1;

		if(!inner_string_p && (code == '\t' || code == ' ' || code == '|')){
			cur_macronum++;
			continue;
		}
		if(x == 0){
			cur_macronum++;
		}
		return code;
	}
}

/*
   トラック内から１文字取ってくる。
   取ってこれない(EOF)場合は、-1を返す。
   行末に来た場合は、-2を返す。
   マクロに入った場合は、-3を返す。

   inner_string_pが非0なら空白無視やマクロ処理などは一切やらない(Nide)
   */
#define MAX_LOOP_NEST_LEVEL 16
static int track(int inner_string_p)
{
	static struct {
		Fpos_t start_pos;		/* ループ開始位置 */
		int loop_count;			/* ループを回った回数 */
		int start_line;			/* ループ開始地点の行番号 */
	} kloop[MAX_LOOP_NEST_LEVEL];
	int i, code, count;		/* 汎用 */

	for(;;){
		code = getc2(fp1);	/* 現在の場所から読んでくる */
		if(inner_string_p){ /* Add Nide */
			switch(code){
			case EOF: case '\n':
				break; /* この2つについては文字列外と同じ処理 */
			default:
				return code;
			}
		}
		switch(code){
		case EOF:
		/* case '!': */ /* '!'以降は全て無視する */
			return -1;
		case '\n':
			ungetc2(fp1);
			return -2;
		/* case '\r': */ /* 0x0d は無視する */
		case '\t':	/* タブは無視する */
		case ' ':	/* スペースは無視する */
		case '|':	/* 小節線は無視する */
			break;
		case '[':	/* 繰り返しの開始 */
			if(kloop_ptr >= MAX_LOOP_NEST_LEVEL) mml_err(43);

			fgetpos2(fp1, &kloop[kloop_ptr].start_pos);
			 /* ループ開始地点のセット */
			kloop[kloop_ptr].loop_count = -1;
			 /* ループを回る回数。最初は-1にしておく */
			kloop[kloop_ptr].start_line = cur_line; /* 行番号の保存 */
			kloop_ptr++;
			break;
		case '/':
		case ':':	/* ループアウト */
			if(!kloop_ptr) mml_err(66);
			if(kloop[kloop_ptr - 1].loop_count == 1){
				int loop_inner_string_p = 0;
				if(code == '/') mml_warn(2); /* obsoleteの警告 */

				count = 0;
				i = cur_line;	/* 行番号の保存（エラーを出すときのために） */
				for(;;){
					code = getc2(fp1);
					if(code == '"'){
						loop_inner_string_p ^= 1; continue;
					}
					if(loop_inner_string_p){
						if(code == '\\' || (mskanji && ismskanji1(code))){
							if(EOF == getc2(fp1)){
								cur_line = i;
								mml_err(4);
							}
						}
						continue;
					}
					else if(code == '[') count++;
					else if(code == ']'){
						(void)getnum();
						if(count == 0) break;
						count--;
					}else if(code == '\n'){
						ungetc2(fp1);
						if(nexttrack() == -1){
							cur_line = i; /* ':'があった行番号に戻す */
							mml_err(4);
						}
					}else if(code == EOF){
						cur_line = i; /* ':'があった行番号に戻す */
						mml_err(4);
					}
				}
				kloop_ptr--;	/* ループを抜けたので、ネスト数を減らす */
			}
			break;
		case ']':				/* 繰り返しの終わり。 */
			if(kloop_ptr == 0) mml_err(3);
			count = getnum();
			if(count == 0) count = 2;
			 /* まだループを回ってない場合、回る回数をセット */
			if(kloop[kloop_ptr - 1].loop_count == -1)
				kloop[kloop_ptr - 1].loop_count = count;
			if(--kloop[kloop_ptr - 1].loop_count == 0){
				kloop_ptr--;
				break;
			}
			fsetpos2(fp1, &kloop[kloop_ptr - 1].start_pos);
			 /* ループの先頭に飛ぶ */
			cur_line = kloop[kloop_ptr - 1].start_line; /* 行番号を戻す */
			break;
		case '$':	/* マクロ */
			code = getc2(fp1);
			cur_macro = 0;
			if(is_digit(code)){
				cur_macro = dtoi(code) * 26;
				code = getc2(fp1);
			}
			if(!is_lower(code)){
				mml_err(6);
			}
			cur_macro += code - 'a';
			cur_macronum = 0;
			/* if(macros(cur_macro)[0] == '\0') */
			if(macros(cur_macro) == NULL){
				int sub = cur_macro - (code - 'a');
				char *p = Msg;

				 /* Msgにマクロ番号を入れる */
				if(sub) *p++ = '0' + sub / 26;
				*p++ = code, *p = '\0';
				mml_err(65);
			}
			return -3;
		default:
			return code;
		}
	}
}

/*
   tnum, talf に対応するトラックを1行だけ取ってくる．
   （トラックの先頭にポインタを移す）

   0AはAと同じ．?はAからZのワイルドカード．並列表記可能．

   トラック名記述例:
   A       == 0A
   0A      == 0A
   1A      == 1A
   ?       == 0A - 0Z
   2?      == 2A - 2Z
   1A2B    == 1A, 2B
   0?2?    == 0A - 0Z, 2A - 2Z
   0A1BC2D == 0A, 1B, 0C, 2D
   ABCD1?  == 0A, 0B, 0C, 0D, 1A - 1Z

   1BC などとやっても，1B, 1C にはならない点に注意．(1B,0Cになる)
   1B,1Cを指定したい場合には，1B1Cと書かないといけない．
   行頭に空白やタブがあってはいけない．
   トラック名の直後には1個以上の空白がないといけない．

   トラック名と見なせない場合は、無視する．（エラーは出さない）

   現在の行番号が常に cur_line に入る．

   */

static int gettrack2(int);
extern char track_map[];

 /* Add Nide */
int alloc_master_step(void)
{
	master_step = malloc((master_step_amount=1024) * sizeof(*master_step));
	return master_step != NULL ? 0 : -1;
}
void clear_master_step(void)
{
	int	i;

	for(i = 0; i < master_step_amount; i++) master_step[i].step = -1;
}
static void check_master_step_amount()
{
	int i;
	int prev_amount;

	 /* master_step_amount[master_count]までの存在を保証 */
	if(master_count >= master_step_amount){
		prev_amount = master_step_amount;
		master_step_amount = master_count + 1024;
		master_step = realloc(master_step,
			master_step_amount * sizeof(*master_step));
		if(master_step == NULL) mml_err(61);
		for(i = prev_amount; i < master_step_amount; i++)
			master_step[i].step = -1;
	}
}

static void read_ppinfo(void)/* Add Nide, 外部プリプロセッサからの情報の処理 */
{
	int c, i, amount = 0, escape;
	long l;
	char *alc = NULL;

	while((c = getc2(fp1)) == ' ' || c == '\t');
	if(c == EOF) return;
	ungetc2(fp1);
	if(!is_digit(c)) return;

	for(l = 0; c = getc2(fp1), is_digit(c); ) l = l*10 + dtoi(c);
	if(c == EOF) return;
	ungetc2(fp1);
	if(!(c == ' ' || c == '\t')) return;

	while((c = getc2(fp1)) == ' ' || c == '\t');
	if(c == EOF) return;
	if(c != '"'){
		ungetc2(fp1);
		return;
	}

	for(escape = i = 0; ; i++){
		if(amount <= i){
			amount = i + 100;
			if((alc = realloc(alc, amount)) == NULL) mml_err(61);
		}
		switch(c = getc2(fp1)){
		case '\n':
			ungetc2(fp1); /* FALLTHROUGH */
		case EOF:
			free(alc);
			return;
		}

		if(escape){
			escape = 0;
		} else
		switch(c){
		case '"':
			alc[i] = '\0';

			free(ppinfo . fname);
			ppinfo . line = l;
			ppinfo . fname = alc;
			ppinfo . actual_line = cur_line;
			return;
		case '\\': /* 他と揃える 但し「\n」を0x0aにするなどの処理は省いた */
			escape++;
			break;
		default:
			if(mskanji && ismskanji1(c)) escape++;
			break;
		}
		alc[i] = c;
	}
}

static int gettrack(void)
 /* 返り値は、現在のステップ数が格納されているmaster_step[]の添字(従属
    トラックを主トラックに合わせようとしている場合)、あるいは-1(EOF), -2
    (主トラックの場合), -3(従属トラックだが対応する主トラックがない) */
{
	int code;
	int i1, i2;

	if(talf == 0 && tnum == 0 && scan_flag == 0){ /* トラックマップの作成 */
		int talf2;

		scan_flag = 1;
		for(talf2 = 0; talf2 < 26; talf2++){
			if(gettrack2(talf2) != -1) track_map[talf2] = 1;

			fseek2(fp1, 0L, SEEK_SET); /* ファイルの先頭に移動 */
			cur_line = 1; /* add Nide */
		}
	}

	for(;;){
		int track_found = 0;

		if(ftell2(fp1)){
			while((code = getc2(fp1)) != '\n'){
				if(code == EOF) return -1;
			}
			cur_line++;
		}

		for(;;){
			i1 = getc2(fp1);
			if(i1 == EOF){
				return -1;
			} else
			if(i1 == '\n'){
				cur_line++; continue;
			} else
			if(i1 == '#'){ /* 外部プリプロセッサからの情報。Nide */
				read_ppinfo();
				break;
			} else
			if(i1 == '$'){ /* マクロ定義行 */
				break;
			} else
			if(i1 == '='){ /* 行頭で =n を指定した場合 */
				ungetc2(fp1);
				return tnum ? master_count : -2;
			}

			if(is_upper(i1) || i1 == '?'){ /*0〜9が省略されてる場合*/
				i2 = i1;
				i1 = '0';
				track_found = 1;
			} else if(is_digit(i1)){
				i2 = getc2(fp1);
				if(!(is_upper(i2) || i2 == '?')) mml_err(67);
				track_found = 1;
			} else {
				if(!track_found) mml_err(67);
				i2 = i1;
			}

			if(i2 == EOF){
				return -1;
			} else
			if(i2 == '\n'){
				cur_line++;
			} else
			if(i2 == talf + 'A' || i2 == '?'){
				if(tnum == 0){ /* 主トラック検査 */
					if(i1 == '0'){ /* 主トラック発見 */
						master_count++;
						check_master_step_amount();
						master_step[master_count-1].step = -2;
						master_step[master_count-1].linenum = cur_line;
						master_step[master_count].step = -1;
						return -2;
					} else { /* 従属トラック発見 */
						track_map[dtoi(i1) * 32 + talf] = 1;
						/* 従属トラックマップ作成 */
					}
				} else { /* 従属トラック検査 */
					if(i1 == '0'){ /* 主トラック発見 */
						master_count++;
						check_master_step_amount();
					   /* master_step[]への格納は主トラック
					      検査時に終わっているから不要? */
					} else if(dtoi(i1) == tnum){ /* 従属トラック発見 */
						if(master_count == -1){
							return -3; /* 主トラックがない */
						}
						if(!kloop_ptr){ /* 「[ ]」によるループ内でない場合
						  は直前の主トラックの先頭ステップ(そこがループ内
						  ならその最初の時のステップ)に合わせちゃおう */
							int tmp1, tmp2;

							tmp1 = tmp2 = master_count;
							/* 主トラックが既に見つかっているから
							   master_count >= 0 */
							while(master_step[tmp1+1].step >= 0 &&
							      master_step[tmp1+1].linenum <= cur_line){
							 /* master_stepのデータは突然終わることはなく
							    最後に-1が必ずある(check_master_step_amount
							    を呼ぶなどしても)はずなので、master_count
							    のオーバフローチェックはしなくていい */
								tmp1++;
								if(master_step[tmp1].linenum >
								   master_step[tmp2].linenum) tmp2 = tmp1;
							}
							master_count = tmp2;
						} /* Add Nide */
						return master_count;
					}
				}
			} else
			if(!is_upper(i2)){
				break;
			}
		}
	}
}

/* トラックマップ作成時に呼ばれる EOF時-1, そうでなければ0返す */
static int gettrack2(int talf2)
{
	int code;
	int i1, i2;

	for(;;){
		if(ftell2(fp1)){
			while((code = getc2(fp1)) != '\n'){
				if(code == EOF) return -1;
			}
			cur_line++;
		}

		for(;;){
			i1 = getc2(fp1);
			if(i1 == EOF){
				return -1;
			} else
			if(i1 == '\n'){
				cur_line++; continue;
			}

			if(!is_digit(i1)){ /*0〜?が省略されてる場合*/
				i2 = i1;
				i1 = '0';
			} else {
				i2 = getc2(fp1);
			}

			if(i2 == EOF){
				return -1;
			} else
			if(i2 == '\n'){
				cur_line++;
			} else
			if(i2 == talf2 + 'A' || i2 == '?'){
				if(i1 == '0') return 0;
			} else
			if(!is_upper(i2)){
				break;
			}
		}
	}
}

/*
   トラックの最初のスペースかタブか小節線まで移動する．
   スペースまでに変な文字があったらエラー。スペースかタブか小節線が
   見つかる以前にEOFか行末に行き当たったら、-1を返す  */
static int skipts(void)
{
	int code;

	for(;;){
		code = getc2(fp1);
		if(code == '='){
			ungetc2(fp1);
			return 0;
		}
		if(code == EOF) return -1;
		if(code == '\n'){
			ungetc2(fp1);
			return -1;
		}
		if(code == ' ' || code == '\t' || code == '|') return 0;
		if(is_upper(code) || is_digit(code) || code == '?') continue;

		mml_err(67);
	}
}

/*
   数値の取得
   */
static int getnum(void)
{
	int num = 0;
	int code;

	for(;;){
		code = getc2(fp1);
		if(!is_digit(code)){
			ungetc2(fp1);
			break;
		}
		num = num * 10 + dtoi(code);
	}
	return num;
}

int escchr(int c) /* Add Nide */
{
	switch(c){
	case 'n':
		return '\n';
	case 't':
		return '\t';
	case 'b':
		return '\b';
	case 'r':
		return '\r';
	case 'f':
		return '\f';
	case 'a':
		return '\007';
	case 'v':
		return '\013';
	case 'e':
		return '\033';
	default:
		return c;
	}
}
