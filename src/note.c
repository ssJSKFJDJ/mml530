
/*
 *      file    name            note.c
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include <string.h>
#include <time.h>
#include "win.h"
#include "file.h"
#include "charproc.h"
#include "mmlproc.h"

struct local_note_vars {
	int onkai;
	int oncho; /* 音長 */
	int oto; /* r(休符):1 \:3 K:4 A:8 どれでもなければ0 */
#define OTO_KYUFU 001
#define OTO_BS_CMD 002 /* \の場合OTO_KYUFUビットも立つ */
#define OTO_K_CMD 004
#define OTO_A_CMD 010
	int velocity;	/* localなvelocity */
	int gatetime;	/* localなgatetime */
	int koff;	/* localなキーオフベロシティ */
};

/* mml2mid.c からの extern */
extern fileptr fp2;		/* 出力ファイル(mid) */
extern void write_length(long, fileptr);
extern Fpos_t lastlenpos;	/* 最後に音長を書き込んだ場所へのポインタ */
extern long lastlen;		/* 最後に書き込んだ音長の値 */
extern int length(void);
extern int xget(int *);
extern int trans;
extern int german_scale, backcompati;

/* mmlproc.c からの extern */
extern long tstep;	/* トラックの現在のステップタイム */
extern int cur_ch;	/* 現在のチャンネル */
extern int run;		/* ランニングステータス */
extern int octave;	/* オクターブ */
extern int under;	/* 転調 */
extern int transcale;	/* 調号 */
extern int velocity, rvel1, rvel2; /* ベロシティー */
extern int randvel1, randvel2; /* ランダムベロシティー */
extern int ktencho;	/* Jコマンドの値 */
extern int gatetimeQ;	/* Qコマンドの値 */
extern int gatetime;	/* qコマンドの値 */
extern int gatetime2;
extern int ctl_values[]; /* expression, panpot, …を全部まとめた配列 */
extern int rpan1, rpan2;
extern int mod_delay, mod_after;
extern struct keyproc2 *keyproc2;
extern int keyproc2_ptr /* 個数 */, keyproc2_amount;
extern int koff;	/* キーオフ・ベロシティ */
extern int codef[KIND_MAX][MAX_F_ARG+1];
extern int codeu[KIND_MAX][MAX_U_ARG+1];
extern int codei[KIND_MAX][5];
extern int check_cntchange(int, int);
extern int check_bendchange(int);
extern void mml_err(int);
extern int psw;

/* 98.3.6 追加 */
/* U指定時に last=first=0
   F,I開始時にU,F,Iが無いならlast=first=0
   F,I開始時にU,F,Iがあるならx_* -= last-first,first=z
   F,I毎回last設定
*/
long first_F[KIND_MAX];
long last_F[KIND_MAX];
long first_I[KIND_MAX];
long last_I[KIND_MAX];

/*
	98/3/4追加。
	F,Iの同時使用を可能にする。
	F,Iコマンド系の値をmmlprocに相対値で書くために、ベースとなる
	値をx_*に保存する。
*/
int x_values[KIND_MAX]; /* x_expression, x_panpot…を全部まとめた配列 */

int andflag;	/* &があったかどうかのフラグ */
/* キーオフ時に処理すべきキーオフ */
struct keyproc *keyproc;
int keyproc_ptr /* 個数 */, keyproc_amount;

struct mmlproc *mmlproc;
int mmlproc_ptr /* 個数 */, mmlproc_amount;

void note(int); /* 音符、休符処理*/
void setcode_U(int, int);
void setcode_F(int, int, int);
void setcode_I(int, int, int);
static void set_keyproc(int, int);
static void set_keyproc2(int, int, int);
static void set_mmlproc(int, int, int);
static void set_rpan(void);
static void set_rvel(void);
static int set_randvel(int);
static void set_keyoff(void);
static void write_step0_change(void);
static void write_mmlproc(int, long *, long *);
void noteoff(int, int);
static int get_hanon(int);
int soutai0(int);
static int soutai(int);
static void putnote(int, int);
static void putnoteoff(int, int);

extern int check_proc(int);

#ifdef NO_MEMMOVE
#define shift_array(a, i, j) do { int c_tmp; \
	for(c_tmp = (i); c_tmp < (j)-1; c_tmp++) (a)[c_tmp] = (a)[c_tmp + 1]; \
} while(0)
#else
#define shift_array(a, i, j) \
	memmove(&(a)[i], &(a)[(i)+1], ((j)-(i)-1)*sizeof(*(a)))
#endif

int param_tbl[] = {
	11, 10, 7, -1, 0x20, 0x21, 1, -1, -1
}; /* mmlprocに記録される未処理イベントへの第1パラメータ
	順にexpressiom, panpot, …に対するもの。-1は使われていないもの */

/* 相対指定は可能だが、値の省略は不可能な値を取得するための関数 */
/* 値が省略されていた場合は、-1 を返す */
int soutai0(int base)
{
	int rt;
	int i;
	int num = xget(&i);

	if(i == 0){
		rt = num;
	}else if(i == -1 || i == 1){ /* 相対指定の場合 */
		rt = base + num;
	}else{
		return -1; /*値が省略された場合 */
	}
	ALIGN(rt); /* 相対指定以外でも強制的に0〜127の範囲になるようにした */
	return rt;
}

/* 相対指定が可能で、かつ、値の省略も可能な値を取得するための関数 */
static int soutai(int base)
{
	int rt = soutai0(base);

	if(rt == -1) rt = base;
	return rt;
}

/* キーオン（またはvelocity0でキーオフ）するだけの関数 */
static void putnote(int p1, int p2)
{
	if(run != 9){
		putc2(0x90 + cur_ch, fp2);
		run = 9;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

/* 80hでキーオフするだけの関数 */
static void putnoteoff(int p1, int p2)
{
	if(run != 8){
		putc2(0x80 + cur_ch, fp2);
		run = 8;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

/* a0hでポリプレッシャーを与えるだけの関数 */
static void ppres(int p1, int p2)
{
	if(run != 0xa){
		putc2(0xa0 + cur_ch, fp2);
		run = 0xa;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

 /* mmlproc[i].stが最小となるiを返す */
static int find_minst(void)
{
	int i, minst_ptr;
	long minst;

	minst = mmlproc[0].st;	
	minst_ptr = 0;
	for(i = 1; i < mmlproc_ptr; i++){
		if(mmlproc[i].st < minst){
			minst = mmlproc[i].st;
			minst_ptr = i;
		}
	}
	return minst_ptr;
}

/*

  処理の区別．

  発音前にしないといけない処理．
    [例] Fコマンドで指定される最初のexpressionの値．
  発音中にしないといけない処理．
    [例] ・直前に発音された音のゲートタイムがマイナスだった場合に
           消音しないといけない．
	 ・Mコマンド
	 ・Fコマンド
    ステップタイムを考える必要がある．
  発音後にしないといけない処理
    [例] ・Mコマンドを使ってた場合，modulationの値の初期化．
         ・Fコマンドを使ってた場合，expressionの値の初期化．
         と思ったけど、直後はマズイ・・・

  考えるべき状況：

ゲートタイムがマイナス、および、＆の処理の兼ね合い。
例：q-4 e4&a2c4
    cがキーオンしてから、eとaがキーオフされないといけない。

ゲートタイムがマイナスで、かつ、&が後ろにある場合。
  ゲートタイムがマイナス、の方でキーオフ位置を記憶する。
直前に&が無い音符が来たときになって初めて、今まで
ゲートタイムマイナスで記憶した音の処理をする。

＆とイベントの兼ね合い。
例：M48,127 c2&e4
MON127は1度しか書かれない。
しかも、キーオフ後に、MON0 が必要。

休符と、＆、ゲートタイムとの兼ね合い。
例えば、
a2.^4. を、 a2.&r4. と全く同じにする。つまり、ゲートタイム処理は、
&r4.のところでも行われる。
このあたりのことを考えると、音符と休符の処理は似てる。
例２：
q-4 c2r4 だと、どう処理するか。

ゲートタイムがマイナスの時。
直前の音がゲートタイムマイナスで発音されていて、現在発音したい音の音長が
ものすごく短い場合。


qn0,n1
    n0 == gatetime  マイナスでも良い。
    n1 == gatetime2
今までのqのマイナスコマンドは無くなった。
代わりに、q100,4 のようにすれば良い。つまり、第一パラメータの値を大きくする。

この情報は古い．
↓
・音符、休符 (a-g,r)n0,n1,n2,n3[&]
       n0 == note length
       n1 == local velocity
       n2 == local gatetime
       n3 == local keyoff velocity (use 80h event)
  - preandflag = andflag
  - andflag 処理
  - localgatetime が無ければ、 localgatetime = gatetime;
  - keyoff velocity が無ければ、 localkoff = koff;
  - localgatetime < 0 ならば、gtflag = 1; else gtflag = 0;
  - その音階をキーオンする。休符ならばしない。
  - どの位置までイベントを処理するかを検討する。
    andflag == 0 && gtflag == 0ならば、
      音長 - localgatetime < gatetime2 ならば、localgatetime = 音長
    else
      localgatetime = 音長
  - mmlprocの処理
    * ＜イベントの処理＞
      イベントをmmlprocに順番に書いていく。
      この時に、preandflag != 0 ならば、つまり、直前に&があった場合は、
      途中からイベントを処理する。それは、イベントを書く手続きにおいて
      うまく考慮する。例えば、Mコマンドでは、途中からの処理ではモジュレーション
      を書き込まない。
      どこまでイベントを書くかは、localgatetime まで書く。
      必要に応じて、keyprocにも書く。
    * イベントの処理が行われない場合もある。それは、
      休符で、しかも、preandflag == 0 の場合。
    * ＜マイナスゲートタイムの処理＞
      preandflag == 0 ならば、つまり、直前に&が無い場合は、
      keyproc2 から mmlproc に書く。
      ただし、keyproc2_st > localgatetime の場合は、
      keyproc2_st = localgatetimeとする。
      最後に、keyproc2_ptr = 0 にする。
    * 休符の場合も、マイナスゲートタイム処理を行う。
  - mmlprocを実際に書き込む処理をする。
    結果的に、mmlproc_ptr = 0 となる。
  - keyoff処理をする。
    （音符 || 休符 && keyproc_ptr != 0 の場合だけ）
    localkoffを考えてキーオフする。
    * gtflag == 1 の場合は、キーオフしない。
      ・最後のステップタイム = 0 を書く。
      ・keyproc2に保存する。（音符の場合だけ）
    * else andflag == 1 の場合は、キーオフしない。
      ・最後のステップタイム = 0 を書く。
      ・keyprocに保存する。（音符の場合だけ）
    * else
      ・keyprocに含まれる音をキーオフする。
      ・自分自身をキーオフする。（音符の場合だけ）
      ・最後のステップタイムをちゃんと考えて書く。
*/

 /* note()の下請け *varsに6つの値を入れて戻る
    LSI-Cでコンパイル可能にするためにnote()から分離した(そうしないとLSI-Cでは
    メモリ不足でコンパイルできない) */
static void get_note_values(int code, struct local_note_vars *vars)
{
	int i, gatetimetmp;

	vars->oto = 0;
#ifdef __GNUC__
	vars->onkai = vars->oncho = 0; /* 警告を抑える目的 */
#endif
	for(;;){
		switch(code){
		case 'a':
			vars->onkai = 9;
			break;
		case 'b':
			vars->onkai = german_scale ? 10 : 11;
			break;
		case 'c':
			vars->onkai = 0;
			break;
		case 'd':
			vars->onkai = 2;
			break;
		case 'e':
			vars->onkai = 4;
			break;
		case 'f':
			vars->onkai = 5;
			break;
		case 'g':
			vars->onkai = 7;
			break;
		case 'h':
			vars->onkai = 11; /* german_scale only */
			break;
		case 'r':
			if(vars->oto) mml_err(38);
			vars->oto = OTO_KYUFU;
			break;
		case '\\':
			if(vars->oto) mml_err(38);
			vars->oto = OTO_KYUFU | OTO_BS_CMD;
			break;
		case 'K':
			if(vars->oto) mml_err(38);
			vars->oto = OTO_K_CMD;
			code = getbyte(0);
			continue;
		case 'A':
			if(vars->oto) mml_err(38);
			vars->oto = OTO_A_CMD;
			code = getbyte(0);
			continue;
		default:
			mml_err(38);
		}
		break;
	} /* ここでvars->otoは0, OTO_KYUFU, OTO_KYUFU | OTO_BS_CMD, OTO_K_CMD,
	     OTO_A_CMDのいずれか vars->onkaiが不定値のままここに来るのは
	     vars->otoがOTO_KYUFUかOTO_KYUFU | OTO_BS_CMDの場合だけ */

	if(vars->oto == 0 && psw == 0){
		if(rpan1 != 0) set_rpan(); /* running panpot の処理 */
		if(rvel1 != 0) set_rvel(); /* running velocity の処理 */
	}

	if(!(vars->oto & OTO_KYUFU)){
		 /* このときcodeは'a'〜'h'のいずれか */
		vars->onkai += 12 + get_hanon(code) + octave * 12 + under;
		if(gatetime2 != -1) vars->onkai += trans;
		if(ktencho != 0) vars->onkai = ktencho; /* 強制転調処理 */
		ALIGN(vars->onkai); /* 範囲外チェック */
	}
	if(!(vars->oto & (OTO_K_CMD | OTO_A_CMD)))
		vars->oncho = length();

   	gatetimetmp = gatetime + vars->oncho * (8 - gatetimeQ) / 8;

	 /* ここまででvars->{onkai,oncho,oto}が決まる(但しvars->otoが
	    OTO_KYUFUかOTO_KYUFU | OTO_BS_CMDの場合はvars->onkaiは不定
	    OTO_K_CMDかOTO_A_CMDの場合はvars->onchoが不定)
	    次はvars->{velocity,gatetime,koff}の決定 */

	if(getbyte(1) == ','){
		(void)getbyte(2);
		vars->velocity = soutai(velocity);
		if(getbyte(1) == ','){
			int num;

			(void)getbyte(2);
			num = xget(&i);
			if(i != -2) vars->gatetime = num;
			else{
				vars->gatetime = gatetimetmp; /* 値の省略 */
			}
			if(getbyte(1) == ','){
				(void)getbyte(2);
				vars->koff = soutai(koff);
			}else{
				vars->koff = koff;
			}
		}else{
			vars->gatetime = gatetimetmp;
			vars->koff = koff;
		}
	}else{
		vars->velocity = velocity;
		vars->gatetime = gatetimetmp;
		vars->koff = koff;
	}
	if(vars->oto == 0 && psw == 0){
		if(randvel1 != 0) vars->velocity = set_randvel(vars->velocity);
		 /* random velocity */
	}
}

/* 音符、休符，\の処理 */
void note(int code)
{
	int i;
	long j;
	int gt;
	long p = 0;
	int preandflag, minst_ptr;
	struct local_note_vars local;

	/* mmlproc_ptr = 0; */ /* ここで 0 にするとやばい */

	get_note_values(code, &local);
	 /* localに6つの値を入れて帰ってくる 但しlocal.otoが
	    OTO_KYUFUかOTO_KYUFU | OTO_BS_CMDの場合はlocal.onkaiは不定
    	    OTO_K_CMDかOTO_A_CMDの場合はlocal.onchoが不定 */

	if((local.oto & OTO_KYUFU) && local.oncho == 0)
		return; /* 音長ゼロの休符は無視 */

	if(local.oto & OTO_K_CMD){ /* Kコマンドの場合 */
		if(psw != 0) return;
		putnote(local.onkai, local.velocity);
		write_length(0, fp2);
		return;
	}
	if(local.oto & OTO_A_CMD){ /* Aコマンドの場合 */
		if(psw != 0) return;
		ppres(local.onkai, local.velocity);
		write_length(0, fp2);
		return;
	} /* ここに来ればlocal.onchoは不定ではない */

	preandflag = andflag;
	if(local.oto & OTO_BS_CMD){
		andflag = 1;
	} else if(getbyte(1) == '&'){
		(void)getbyte(2);
		andflag = 1;
	} else {
		andflag = 0;
	}

	if(psw != 0) return;

	/* gatetime2 == -1 となるのは RT の時 */
	if((local.oto & OTO_KYUFU) || gatetime2 == -1) local.gatetime = 0;
	gt = local.gatetime;

	/* local.gatetime は、どのsteptimeまでイベントを処理するか */
	if(gatetime2 != -1){
		if(andflag == 0 && gt >= 0){
			if(local.oncho - local.gatetime < gatetime2){
				if(local.oncho < gatetime2)
					local.gatetime = local.oncho;
				else
					local.gatetime = gatetime2;
			}else{
				local.gatetime = local.oncho - local.gatetime;
			}
		}else{
			local.gatetime = local.oncho;
		}
	}

	/* mmlproc の処理がここに入る．*/
	/* mmlproc の処理というのは、キーオン中にやらないといけない処理の事 */
	/* ステップ0〜local.gatetime まで処理する */
	/* 発音前に先行して処理しないといけない場合もあるのだ！ */
	/* 発音前に処理するのは、キーオン／オフ以外でステップ0のイベント */
	if(local.oto == 0 && preandflag == 0 && mod_delay != -1){
		x_mod_on = 0;
		set_mmlproc(6, 0, 0); /* 入れておく必要あり (6:modulation) */
		if(mod_delay < local.gatetime){
			if(mod_delay && mod_after)
				set_mmlproc(6, mod_after, mod_delay);
		}
	}
	for(i = 0; i < KIND_MAX; i++){
		if(local.oto == 0 || preandflag == 1){
			if(codef[i][0] != 0){
				setcode_F(local.gatetime, preandflag, i);
			}
			if(codei[i][0] != 0){
				setcode_I(local.gatetime, preandflag, i);
			}
		}
	}
	if(preandflag == 0){
		set_keyoff();
	}

	/* 発音前に処理しないといけないイベントを書き込む */
	/* 具体的には、ステップタイム0のキーオン／オフ以外のイベントすべて */
	write_step0_change();

	/* ようやく発音する */
	if(!(local.oto & OTO_KYUFU)){
		putnote(local.onkai, local.velocity);
		p = 0; /* 音符と休符を同等に扱うために p を導入した。苦肉の策 */
	}else{ /* 休符の場合 */
		tstep -= lastlen;
		fsetpos2(fp2, &lastlenpos); /* わざわざ戻す。苦肉の策 */
		p = lastlen; /* pというのは、最初の1回だけ効いてくるステップタイム */
	}
	if(local.oncho == 0){ /* 音長ゼロの音符の場合 */
	 /* この場合(local.oto & OTO_KYUFU) != 0なので、local.onkaiは
	    不定ではない */
		andflag = 1; /* 後ろに&があるのと同じ扱いにする */
		write_length(0, fp2);
		set_keyproc(local.onkai, local.koff); /* 後でキーオフが必要ということ */
		return;
	}

	/* 以下は、mmlproc を書き込む処理 */
	j = local.gatetime; /* jは、キーオフまでのステップタイム */
	while(mmlproc_ptr > 0){
		minst_ptr = find_minst();
		if(mmlproc[minst_ptr].st > j){
		 /* Mod Nide; Uコマンドのバグ対策。ここで既にmmlproc[i].stから
		    j引くため、後にmmlproc[i].stから引く数は従来よりj減らす */
			for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= j;
			break; /* ここのbreakはこのままでいいだろうか? */
		}
		write_mmlproc(minst_ptr, &p, &j);
	}

	/* キーオフ処理 */
	if(!(local.oto & OTO_KYUFU) || keyproc_ptr != 0){
		long xx;

		if(gt < 0){ /* ゲートタイムが負 */
			xx = p + local.oncho - local.gatetime;
			write_length(xx + j, fp2);
			if(!(local.oto & OTO_KYUFU))
				set_keyproc2(local.onkai, local.koff, -gt);
			for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= xx;
		}else if(andflag == 1){
			xx = p + local.oncho - local.gatetime;
			write_length(xx + j, fp2);
			if(!(local.oto & OTO_KYUFU))
				set_keyproc(local.onkai, local.koff);
			for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= xx;
		}else{
			write_length(p + j, fp2);
			/* p = 0; ←入れなくても良い */
			if(!(local.oto & OTO_KYUFU)){ /* 休符の時 */
				noteoff(local.onkai, local.koff);
				write_length(0, fp2);
			}
			for(i = 0; i < keyproc_ptr; i++){
				noteoff(keyproc[i].onkai, keyproc[i].velo);
				write_length(0, fp2);
			}
			keyproc_ptr = 0;
			tstep -= lastlen;
			fsetpos2(fp2, &lastlenpos);

			/* ノートオフ中にもmmlprocの処理をする 98/3/4 */
			xx = lastlen + local.oncho - local.gatetime;
			while(mmlproc_ptr > 0){
				minst_ptr = find_minst();
				if(mmlproc[minst_ptr].st > xx){
				 /* Mod Nide; Uコマンドのバグ対策 */
					for(i = 0; i < mmlproc_ptr; i++)
						mmlproc[i].st -= xx;
					break; /* このbreakはこのままでOKか? */
				}
				write_mmlproc(minst_ptr, NULL, &xx);
			}
			write_length(xx, fp2); /* これはあやしい。可能性がある。 */
		}
	}else{ /* 単なる休符 */
		long xx;

		xx = p + j + local.oncho - local.gatetime;
		while(mmlproc_ptr > 0){
			minst_ptr = find_minst();
			if(mmlproc[minst_ptr].st > 0) break;
			 /* Mod Nide; Uコマンドのバグ対策。xxでなく0と比較 */
			write_mmlproc(minst_ptr, NULL, &xx);
		}
		write_length(xx, fp2); /* これはあやしい。可能性がある。 */
		/* p = 0; ←入れなくても良い */
	}

	/* 不要なはずなのだが。98.3.4
	for(i = 0; i < mmlproc_ptr; i++){
		mmlproc[i].st -= p + j + local.oncho - local.gatetime;
	}
	*/
}

static void set_keyproc(int onkai, int velo)
{
	keyproc[keyproc_ptr].onkai = (short)onkai;
	keyproc[keyproc_ptr].velo = (short)velo;
	if(++keyproc_ptr >= keyproc_amount){
		keyproc_amount = keyproc_ptr + 256;
		if((keyproc = realloc(keyproc,
			    keyproc_amount * sizeof(*keyproc))) == NULL){
			mml_err(61);
		}
	}
}

static void set_keyproc2(int onkai, int velo, int st)
{
	keyproc2[keyproc2_ptr].kp_onkai = (short)onkai;
	keyproc2[keyproc2_ptr].kp_velo = (short)velo;
	keyproc2[keyproc2_ptr].st = st;
	if(++keyproc2_ptr >= keyproc2_amount){
		keyproc2_amount = keyproc2_ptr + 256;
		if((keyproc2 = realloc(keyproc2,
			    keyproc2_amount * sizeof(*keyproc2))) == NULL){
			mml_err(61);
		}
	}
}

 /* Add Nide */
int alloc_mmlproc(void)
{
	mmlproc = malloc((mmlproc_amount = 1024) * sizeof(*mmlproc));
	return mmlproc != NULL ? 0 : -1;
}
static void realloc_mmlproc(void)
{
	if(mmlproc_ptr >= mmlproc_amount){
		mmlproc_amount = mmlproc_ptr + 512;
		if((mmlproc = realloc(mmlproc,
				mmlproc_amount * sizeof(*mmlproc))) == NULL){
			mml_err(61);
		}
	}
}

static void set_mmlproc(int kind, int p, int st)
{
	mmlproc[mmlproc_ptr].kind = kind;
	mmlproc[mmlproc_ptr].param_ctrl = p;
	mmlproc[mmlproc_ptr].st = st;
	mmlproc_ptr++;
	realloc_mmlproc();
}

 /* expression, panpot, …のデフォルト値 */
int def_values[] = {
	127, 64, 100, 64*128, 64, 64, 0, 0, 0
};
 /* code{f,u,i}[x][i]にexpression, panpot, …の何倍を書くか */
int each_factor[] = {
	128, 128, 128, 1, 128, 128, 128, 128, 16
};

/* Uコマンドを書いた時に１回だけ呼ばれる。 */
void setcode_U(int localgatetime, int x)
{
	long real_value;
	int num; /* 配列じゃなくてよい */
	int step; /*配列じゃなくてよい */
	int step3;
	int z, factor;

	first_F[x] = last_F[x] = first_I[x] = last_I[x] = 0;

	factor = each_factor[x];
	if(ctl_values[x] == -1) ctl_values[x] = def_values[x];
	z = ctl_values[x] * factor;
	x_values[x] = ctl_values[x];

	num = z + codeu[x][1];
	if(x != MMLPROC_DTEMPO) ALIGN2(num); /* テンポ以外は範囲内に */
	step = 0;
	set_mmlproc(x, num/factor - z/factor, 0);
	real_value = num/factor; /* 実際に作用する値 */

	step3 = step;
	for(;;){
		long k11, k12, k13;
		int k;

		k12 = k13 = 0;

		step += codeu[x][0];
		if(step - step3 > localgatetime){
			step -= codeu[x][0];
			break;
		}

		for(k = 0; k < codeu[x][2]; k++){
			k11 = k12;
			k12 = k11 + codeu[x][k * 2 + 3];
			if(k != 0) k13 += codeu[x][k * 2 + 2];
			if(k != codeu[x][2] - 1){
				if(k11 <= step && step < k12){
					if(codeu[x][k * 2 + 3] == 0) continue;
					num = z + codeu[x][1] + k13 +
						(step - k11) * codeu[x][k * 2 + 4] /
						codeu[x][k * 2 + 3];
					break;
				}
			}else{
				if(k11 <= step && step <= k12){
					if(codeu[x][k * 2 + 3] == 0) continue;
					num = z + codeu[x][1] + k13 +
						(step - k11) * codeu[x][k * 2 + 4] /
						codeu[x][k * 2 + 3];
					break;
				}else{
					return;
				}
			}
		}

		if(x != MMLPROC_DTEMPO) ALIGN2(num);
		if(num / factor == real_value) continue;
		set_mmlproc(x, num/factor - real_value, step-step3);
		real_value = num/factor; /* 実際に作用する値 */
	}
}

void setcode_F(int localgatetime, int preandflag, int x)
{
	static int num[KIND_MAX];
	static int step[KIND_MAX];
	static long real_value[KIND_MAX];
	int step3;
	int i, z, factor;

	if(x < 0){
		for(i = 0; i < KIND_MAX; i++){
			num[i] = 0;
			step[i] = 0;
			real_value[i] = 0;
		}
		return;
	}

	factor = each_factor[x];
	if(ctl_values[x] == -1) ctl_values[x] = def_values[x];
	z = ctl_values[x] * factor;

	if(preandflag == 0){
		if(check_proc(x) == 0){
			x_values[x] = ctl_values[x];
			first_F[x] = last_F[x] = first_I[x] = last_I[x] = 0;
		} else {
			x_values[x] -= last_F[x]/factor - first_F[x]/factor;
			first_F[x] = z;
		}

		num[x] = z + codef[x][1];
		if(x != MMLPROC_DTEMPO) ALIGN2(num[x]); /* テンポ以外は範囲内に */
		step[x] = 0;
		set_mmlproc(x, num[x]/factor - z/factor, 0);
		real_value[x] = num[x]/factor; /* 実際に作用する値 */
	} else {
		/* real_value[x] = ?? */
	}
	step3 = step[x];

	for(;;){
		int k;
		long k11, k12, k13;

		i = num[x];
		/* このあたりは、すべて相対値で計算するように変更する
		   必要がある。98/3/4 */
		k12 = k13 = 0;
	
		step[x] += codef[x][0];
		if(step[x] - step3 > localgatetime){
			step[x] -= codef[x][0];
			last_F[x] = i;
			break;
		}

		for(k = 0; k < codef[x][2]; k++){
			k11 = k12;
			k12 = k11 + codef[x][k * 2 + 3];
			if(k != 0) k13 += codef[x][k * 2 + 2];
			if(k != codef[x][2] - 1){
				if(k11 <= step[x] && step[x] < k12){
					if(codef[x][k * 2 + 3] == 0) continue;
					num[x] = z + codef[x][1] + k13 +
						(step[x] - k11) * codef[x][k * 2 + 4] /
						codef[x][k * 2 + 3];
					break;
				}
			}else{
				if(k11 <= step[x] && step[x] <= k12){ /*上のifと<=が違う*/
					if(codef[x][k * 2 + 3] == 0) continue;
					num[x] = z + codef[x][1] + k13 +
						(step[x] - k11) * codef[x][k * 2 + 4] /
						codef[x][k * 2 + 3];
					break;
				}else{
					last_F[x] = i;
					return;
				}
			}
		}

		/* 後では全部書くので、ここで範囲はみ出しチェックをする。*/
		if(x != MMLPROC_DTEMPO) ALIGN2(num[x]);
		if(num[x] / factor == i / factor) continue;
		set_mmlproc(x, num[x]/factor - real_value[x], step[x] - step3);
		real_value[x] = num[x]/factor; /* 実際に作用する値 */
	}
}

/* I コマンドを発効するためにmmlprocに書き込む */
void setcode_I(int localgatetime, int preandflag, int x)
{
	static long num[KIND_MAX];
	static int step[KIND_MAX];
	static int dir[KIND_MAX];
	static long real_value[KIND_MAX];
	int step3;
	int factor;
	long z;

	if(x < 0){
		int i;

		for(i = 0; i < KIND_MAX; i++){
			num[i] = 0;
			step[i] = 0;
			dir[i] = 0;
			real_value[i] = 0;
		}
		return;
	}

	factor = each_factor[x];
	if(ctl_values[x] == -1) ctl_values[x] = def_values[x];
	z = ctl_values[x] * factor;

	if(preandflag == 0){
		if(check_proc(x) == 0){
			x_values[x] = ctl_values[x];
			first_F[x] = last_F[x] = first_I[x] = last_I[x] = 0;
		} else {
		     /*	x_values[x] -= last_F[x]/factor - first_F[x]/factor; */
			x_values[x] -= last_I[x]/factor - first_I[x]/factor;
			first_I[x] = z;
		}

		/* gosa = 0; */
		num[x] = z;
		step[x] = 0;
		dir[x] = 1;
		set_mmlproc(x, 0, 0);
		real_value[x] = num[x]/factor; /* 実際に作用する値 */
	} else {
		/* real_value[x] = ?? */
	}
	step3 = step[x];

	for(;;){
		long i = num[x];

		/* codei[x][0〜4]: 振幅 頻度 ディレイ 波長 最大振幅まで */
		if(step[x] >= codei[x][2]){ /* completely rewritten. Nide */
			long num_diff; /* longで計算する必要あり */
			int dt = step[x] - codei[x][2] + codei[x][1];
			int max_timing = codei[x][4] + codei[x][1];
			 /* step[x]が0になるタイミングが最初からcodei[x][1]だけ
			    遅れているらしく、比率をはかるにはdtとmax_timingの
			    それぞれにcodei[x][1]を足しておく必要がある */

			num_diff = (long)codei[x][0]*4 * (dt%codei[x][3])
					/ codei[x][3];
			if(labs(num_diff) > (long)abs(codei[x][0])*3)
				num_diff -= (long)codei[x][0]*4;
			else if(labs(num_diff) > (long)abs(codei[x][0]))
				num_diff = (long)codei[x][0]*2 - num_diff;
			/* num_diff: ノコギリ波の基準値との差 */

			if(dt < max_timing) num_diff = num_diff * dt/max_timing;
			 /* 最大振幅になるまでは振幅だけ縮める */

			num[x] = (int)((long)z + num_diff);
			if(x != MMLPROC_DTEMPO) ALIGN2(num[x]); /* テンポ以外は範囲内に */
		}

		step[x] += codei[x][1];
		if(step[x] - step3 >= localgatetime){
			step[x] -= codei[x][1];
			last_I[x] = z;
			break;
		}
		if(num[x]/factor == i/factor) continue;	/* 書き込む値が変わってない場合*/
		set_mmlproc(x, num[x]/factor - real_value[x], step[x] - step3);
		real_value[x] = num[x]/factor; /* 実際に作用する値 */
	}
}

static void set_rpan(void)
{
	if(panpot == -1) panpot = 64; /* Add Nide; 必要か?? */
	panpot += rpan1;
	if(panpot < 0){
		panpot -= 2*rpan1;
		if(rpan2 == 0) rpan1= 0;
		else rpan1 = -rpan1;
	}else if(panpot > 127){
		panpot -= 2*rpan1;
		if(rpan2 == 0) rpan1= 0;
		else rpan1 = -rpan1;
	}
	put_panpot(panpot);
}

static void set_rvel(void)
{
	velocity += rvel1;
	if(velocity < 0){
		velocity -= 2*rvel1;
		if(rvel2 == 0) rvel1= 0;
		else rvel1 = -rvel1;
	}else if(velocity > 127){
		velocity -= 2*rvel1;
		if(rvel2 == 0) rvel1= 0;
		else rvel1 = -rvel1;
	}
}

static int set_randvel(int velocity)
{
	int x, y, s;
	static int seed_set = 0;

	if(!seed_set){ /* 一度だけseedをセット */
		long t;

		t = (long)time(NULL);
#ifdef UNIX
		t ^= ((long)getpid() * 0x8001);
#endif
		srand((int)t);
		seed_set = 1;
	}

 /* rand()の実装によっては下位のビットは上位ほどランダムでないものもあるので
    できれば下位ビットだけを乱数に使うのは避ける */
#ifdef RAND_MAX
# define irand(n) ((int)((double)(n) * rand() / (RAND_MAX+1.0)))
#else
# define irand(n) (rand() % (n))
#endif
	x = irand(randvel1);
	s = randvel1 * randvel2 / 2;

	y = x * x / s + x - x * randvel1 / s;
	if(irand(2) == 0) y = -y;
	velocity += y;
	ALIGN(velocity);
	return velocity;
}

static void set_keyoff(void)
{
	int i;

	for(i = 0; i < keyproc2_ptr; i++){
		mmlproc[mmlproc_ptr].kind = -1; /* 種別＝キーオフ */
		mmlproc[mmlproc_ptr].param.keyoff = keyproc2[i].keyproc;
		 /* 音階&キーオフvelocity */

		/*if(keyproc2[mmlproc_ptr].st > localgatetime){
			mmlproc[mmlproc_ptr].st = localgatetime;
		}else{
			mmlproc[mmlproc_ptr].st = keyproc2[i].st;
		}*/
		mmlproc[mmlproc_ptr].st = keyproc2[i].st;
		mmlproc_ptr++;
		realloc_mmlproc();
	}
	keyproc2_ptr = 0;
}

static void write_step0_change(void)
{
	int i, x, kind;

	for(i = 0; i < mmlproc_ptr;){
		if(mmlproc[i].st != 0){
			i++; continue;
		}
		switch(kind = mmlproc[i].kind){
		case MMLPROC_KEYOFF:
			/* ここには来ないはず */
			i++; continue;
		default:
			x = x_values[kind] + mmlproc[i].param_ctrl;
			switch(kind){
			case MMLPROC_BEND: /* bend */
				ALIGN2(x); break;
			case MMLPROC_DTEMPO: /* tempo */
				break; /* ここにくることはありうるか? */
			default:
				ALIGN(x); break;
			}
			x_values[kind] = x;
			do_mmlproc(kind, x);
			break;
		}
		shift_array(mmlproc, i, mmlproc_ptr);

		mmlproc_ptr--;
	}
}

static void write_mmlproc(int minst_ptr, long *add_steptime, long *gatetime)
{
	 /* minst_ptrはmmlproc[minst_ptr].stが最小となるような添字。minstは
	    そこでのst。
	    *add_steptimeは最初の1回だけ効いてくるステップタイム。DTEMPO以外の
	    イベントが書き出される場合は、write_lengthで書き出される長さに
	    これを加え、事後*add_steptimeを0にする。
	    *gatetimeはキーオフまでのステップタイム。DTEMPO以外のイベントが
	    書き出される場合はこれをminstだけ減らす */
	int i, x, kind;
	long minst, actual_length;

	actual_length = minst = mmlproc[minst_ptr].st;
	if(add_steptime != NULL) actual_length += *add_steptime;

	kind = mmlproc[minst_ptr].kind;
	if(kind != MMLPROC_DTEMPO){
		*gatetime -= minst;
		if(add_steptime != NULL) *add_steptime = 0;
		write_length(actual_length, fp2);
	} /* DTEMPOの場合テンポマップに書かれて後で処理されるので
	     ここではwrite_lengthしない よってtstepも進まない */

	switch(kind){
	case MMLPROC_KEYOFF: /* キーオフの時 */
		noteoff(mmlproc[minst_ptr].param_onkai,
			mmlproc[minst_ptr].param_velo);
		break;
	default: /* control changeとtempoの時 */
		x = x_values[kind] + mmlproc[minst_ptr].param_ctrl;
		switch(kind){
		case MMLPROC_BEND: /* bend */
			ALIGN2(x); break;
		case MMLPROC_DTEMPO: /* tempo */
			break;
		default:
			ALIGN(x); break;
		}
		x_values[kind] = x;

		switch(kind){
		case MMLPROC_EXPR:
		case MMLPROC_PANPOT:
		case MMLPROC_VOL:
		case MMLPROC_MOD_ON:
			(void)check_cntchange(kind2param(kind), x);
			/* ダミーのチェック。必要*/
			break;
		case MMLPROC_BEND:
			(void)check_bendchange(x); /*ダミーのチェック。必要 */
			break;
		/* case MMLPROC_CPRES:
			(void)check_aft(kind2param(kind), x);
		ダミーのチェックは不要．もともと連続値チェックしてないから */
		}
		do_mmlproc0(kind, x, actual_length);
		 /* do_mmlproc0()はkindがDTEMPOの場合以外第3引数不使用 */
		break;
	}

	 /* 書き込んだmmlprocを除去してそれ以後を1つずらし、stを減らす */
	shift_array(mmlproc, minst_ptr, mmlproc_ptr);
	mmlproc_ptr--;
	if(kind != MMLPROC_DTEMPO){
		for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= minst;
	}
}

/* キーオフベロシティーを考慮してノートオフする関数 */
void noteoff(int onkai, int localkoff)
{
	if(localkoff == 0){
		putnote(onkai, 0);
	}else{
		putnoteoff(onkai, localkoff);
	}
}

static int get_hanon(int onpu)
{
	int code, hanon = 0;

	switch(code = getbyte(1)){
	case '+':
		hanon++;
		(void)getbyte(2);
		if(getbyte(1) == code){
			hanon++;
			(void)getbyte(2);
		}
		break;
	case '-':
		hanon--;
		(void)getbyte(2);
		if(!backcompati && getbyte(1) == code){
			hanon--;
			(void)getbyte(2);
		}
		break;
	case '*':
		if(!backcompati) (void)getbyte(2);
		break;
	default: /* 調号を見る */
		if(german_scale){
			if(onpu == 'b') break; /* 調号は不扱い */
			if(onpu == 'h') onpu = 'b';
		}
		hanon = (transcale - onpu2chogo(onpu) + 705) / 7 - 100;
		break;
	}
	return hanon;
}

int alloc_keyproc(void)
{
	keyproc = malloc((keyproc_amount = 256)* sizeof(*keyproc));
	keyproc2 = malloc((keyproc2_amount = 256) * sizeof(*keyproc2));
	return keyproc != NULL && keyproc2 != NULL ? 0 : -1;
}
