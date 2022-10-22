
/*
 *      file    name            mmlproc.h
 */

int converttrk(void);
void write_tmap();
#define MAXTRKNUM 64

struct keyproc {
	short onkai;  /* 音階 */
	short velo;  /* キーオフベロシティ */
};
struct keyproc2 {
	struct keyproc keyproc;
#define kp_onkai keyproc.onkai
		/* gatetimeがマイナスなので、まだkeyoffしてない音 */
#define kp_velo keyproc.velo
		/* キーオフ・ベロシティ */
	long st;  /* ゲートタイム */
};
struct mmlproc {
	int kind;  /* 未処理イベントの種類 */
#define MMLPROC_KEYOFF	(-1)
#define MMLPROC_EXPR	0
#define MMLPROC_PANPOT	1
#define MMLPROC_VOL	2
#define MMLPROC_BEND	3
#define MMLPROC_CUTOFF	4
#define MMLPROC_RESO	5
#define MMLPROC_MOD_ON	6
#define MMLPROC_CPRES	7
#define MMLPROC_DTEMPO	8
	union {
		struct keyproc keyoff; /* キーオフの場合: 音階とキーオフベロシティ */
		int ctrl; /* control changeの場合: パラメタ */
	} param;
#define param_onkai param.keyoff.onkai
#define param_velo param.keyoff.velo
#define param_ctrl param.ctrl
	long st; /* 未処理イベントのステップタイム */
};
#define KIND_MAX 9 /* MMLPROC_*の最大+1 */

struct tmap {
	int index; /* ソート時に同じstep timeの指定同士の順番を保存するため */
	int map;
#define TMAP_BEAT (-1)	/* 拍子 */
#define TMAP_POP (-2)	/* テンポスタックのポップ */
#define TMAP_DIFF (-3)	/* 一時的な変位 */
 /* 上記以外はビット指定 */
#define TMAP_RELATIVE 01	/* 相対指定 */
#define TMAP_PUSH 02		/* プッシュも行う */
#define TMAP_NORMAL 00		/* 普通のテンポ指定 */
	int p1; /* テンポの値 */
	long st;  /* ゲートタイム */
};

/* f,c,g,d,a,e,b → -1〜5 */
#define onpu2chogo(onpu) (((onpu)-'a'+2)*2 % 7 - 1)

#define ALIGN(x) do{ \
	if((x) < 0) (x) = 0; else if((x) > 127) (x) = 127; \
} while(0)
#define ALIGN2(x) do{ \
	if((x) < 0) (x) = 0; else if((x) > 16383) (x) = 16383; \
} while(0)

#define kind2param(kind) param_tbl[kind]
 /* mmlprocに記録される未処理イベントへの第1パラメータ */

#define check_expr()	check_proc(MMLPROC_EXPR)   /* contx:11 */
#define check_panpot()	check_proc(MMLPROC_PANPOT) /* contx:10 */
#define check_vol()	check_proc(MMLPROC_VOL)	   /* contx:7 */
#define check_bendx()	check_proc(MMLPROC_BEND)
#define check_cutoff()	check_proc(MMLPROC_CUTOFF) /* nrpn:0x20 */
#define check_reso()	check_proc(MMLPROC_RESO)   /* nrpn:0x21 */
#define check_mod()	check_proc(MMLPROC_MOD_ON) /* contx:1 */
#define check_aftx()	check_proc(MMLPROC_CPRES)
#define check_dtempo()	check_proc(MMLPROC_DTEMPO)	/* not used */

#define expression	ctl_values[MMLPROC_EXPR]
#define panpot		ctl_values[MMLPROC_PANPOT]
#define volume		ctl_values[MMLPROC_VOL]
#define bend		ctl_values[MMLPROC_BEND]
#define cutoff		ctl_values[MMLPROC_CUTOFF]
#define resonance	ctl_values[MMLPROC_RESO]
#define mod_on		ctl_values[MMLPROC_MOD_ON]
#define cpres		ctl_values[MMLPROC_CPRES] /* after touch */
#define tempo_diff	ctl_values[MMLPROC_DTEMPO]
/* テンポは他と違いチャンネルローカルではないので、tempo_diffには
   FTなどによるチャンネルローカルなテンポと全体のテンポとの差を入れる */

#define def_expression	def_values[MMLPROC_EXPR]
#define def_panpot	def_values[MMLPROC_PANPOT]
#define def_volume	def_values[MMLPROC_VOL]
#define def_bend	def_values[MMLPROC_BEND]
#define def_cutoff	def_values[MMLPROC_CUTOFF]
#define def_resonance	def_values[MMLPROC_RESO]
#define def_mod_on	def_values[MMLPROC_MOD_ON]
#define def_cpres	def_values[MMLPROC_CPRES]
#define def_tempo_diff	def_values[MMLPROC_DTEMPO]

#define x_expression	x_values[MMLPROC_EXPR]
#define x_panpot	x_values[MMLPROC_PANPOT]
#define x_volume	x_values[MMLPROC_VOL]
#define x_bend		x_values[MMLPROC_BEND]
#define x_cutoff	x_values[MMLPROC_CUTOFF]
#define x_resonance	x_values[MMLPROC_RESO]
#define x_mod_on	x_values[MMLPROC_MOD_ON]
#define x_cpres		x_values[MMLPROC_CPRES]
#define x_tempo_diff	x_values[MMLPROC_DTEMPO]

#define put_expr(para)	 do_mmlproc(MMLPROC_EXPR, para)   /* 11 */
#define put_panpot(para) do_mmlproc(MMLPROC_PANPOT, para) /* 10 */
#define put_vol(para)	 do_mmlproc(MMLPROC_VOL, para)	  /* 7 */
#define put_cutoff(para) do_mmlproc(MMLPROC_CUTOFF, para) /* 0x20 */
#define put_reso(para)	 do_mmlproc(MMLPROC_RESO, para)	  /* 0x21 */
#define put_mod(para)	 do_mmlproc(MMLPROC_MOD_ON, para) /* 1 */
#define put_dtempo(para) do_mmlproc(MMLPROC_DTEMPO, para)	/* not used */
 /* bendとcpresに対しては生のものがある */

extern int def_values[], each_factor[];
extern void do_mmlproc(int, int);
extern void do_mmlproc0(int, int, long);
extern int param_tbl[];

#define MAX_F_ARG 64
#define MAX_U_ARG 64
