
/*
 *      file    name            mmlproc.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win.h"
#include "file.h"
#include "charproc.h"
#include "mmlproc.h"

#define MAX_TEXT_STR 256

#define MAX_TRACK_NAME_STR 256

static char trackname[MAX_TRACK_NAME_STR + 1];

/* mml2mid.c ����� extern */
extern fileptr fp2;         /* ���ϥե�����(mid) */
extern fileptr fp3;         /* �ƥ�ݡ��ޥå��� */
extern void smftrkheader(fileptr, Fpos_t *);
extern int timebase;      /* ������١��� 48,60,80,96,120,160,240,480 */
extern int trknum;        /* �ȥ�å��� */
extern Fpos_t trksize;    /* �ȥ�å���������񤭹����� */
extern int tnum;          /* �ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� 0,1,2 */
extern int talf;          /* �ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� A,B,C */
extern Fpos_t lastlenpos; /* �Ǹ�˲�Ĺ��񤭹�������ؤΥݥ��� */
extern long lastlen;      /* �Ǹ�˽񤭹������Ĺ���� */
extern int x68k;
extern int x68k2;
extern void remove_file_and_owari(void);
extern int german_scale, backcompati, mskanji, warnmode;

/* note.c ����� extern */
extern int andflag;     /* &�����ä����ɤ����Υե饰 */
extern int koff;	/* �������ա��٥��ƥ� */
extern int keyproc_ptr, keyproc_amount;
			/* �������ջ��˽������٤����٥�ȤθĿ� */
extern int mmlproc_ptr, mmlproc_amount;
extern struct mmlproc *mmlproc;
extern void note(int); /* ���䡢�������*/
extern void noteoff(int, int);
extern int soutai0(int);
extern void setcode_F(int, int, int);
extern void setcode_I(int, int, int);
extern void setcode_U(int, int);
int check_proc(int);

/* charproc.c */
extern struct {
	int line, actual_line;
	char *fname;
} ppinfo;
extern int master_count; /* MKR�ɲ� */
extern int master_count_temp, master_count_temp_prev; /* Modified Nide */
extern int escchr(int);

int head;		/* ��°�ѡ��Ȥǥإå���񤤤����ɤ����Υե饰 */
long tstep;		/* �ȥ�å��θ��ߤΥ��ƥåץ����� */
int kloop_ptr;		/* �롼�ץͥ��ȿ� */
int cur_line;		/* ���ߤ�MML��������ι��ֹ� */
int cur_ch;		/* ���ߤΥ����ͥ� */
int run;		/* ���˥󥰥��ơ����� */
int octave;		/* ���������� */
int under;		/* žĴ */
int transcale;		/* Ĵ�� */
static int clength;     /* l���ޥ�ɤΥ���å��� */
int velocity, rvel1, rvel2; /* �٥��ƥ��� */
int randvel1, randvel2; /* ������٥��ƥ��� */
int ktencho;		/* ����žĴ���ޥ�ɤ��� 0����̵�뤵��� */
int gatetimeQ;		/* Q���ޥ�ɤ��� �ǥ��ե���Ȥ�8 */
int gatetime, gatetime2; /* q���ޥ�ɤ��� */
int ctl_values[KIND_MAX]; /* expression, panpot, �Ĥ�ޤȤ᤿���� */
static struct {
	Fpos_t pos; /* ���Ĺ����񤤤����� */
	int lineno; /* ���θ��������Ϲ��ֹ� */
} last_oddlen; /* ��°�ȥ�å����ֹ�碌��ǽ���顼�Υ����å��� */
static int kakko_def;
int rpan1, rpan2;
int mod_delay, mod_after;
int koff;		/* �������ա��٥��ƥ� */
static struct tmap *tmap;
static int tmap_ptr, tmap_amount;
int tempo_master = 100;
struct keyproc2 *keyproc2;
int keyproc2_ptr, keyproc2_amount;  /* �Ŀ� */
int codef[KIND_MAX][MAX_F_ARG+1]; /* F���ޥ����¸�� */
int codeu[KIND_MAX][MAX_U_ARG+1]; /* U���ޥ����¸�� */
int codei[KIND_MAX][5];  /* I���ޥ����¸�� */
static int rpn_para;
static int nrpn_para;
int psw; /* '='�����å� */
static int prog; /* �ץ��������� */
static int bs1; /* bank select */
static int bs2; /* bank select */
static int var[256]; /* �ѿ� z0 ... z255 */
int base; /* @+-n */

void write_length(long, fileptr); /* ����Ĺ->����Ĺ���Ѵ����ƽ񤭤��� */
static void write_keyproc2(void);
			/* 1�ȥ�å��Ѵ���λľ��ˡ�ȯ����β��򥭡����� */
static void write_restofmmlproc(void);
static void code_y(void); /* y���ޥ�ɽ��� */
static void oct(void); /* ���������ֽ��� */
int length(void);
int xget(int *i);
static void tempo(void); /* t���ޥ�ɽ��� */
static void code_B(void); /* B���褿�Ȥ��ν��� (BS, BB, BR, BT) */
static void add_tmap(int, int, long);
static void beat(void);		/* BT���ޥ�ɽ��� */
static void bendrange(void);	/* BR���ޥ�ɤν��� */
static void bendset(int);	/* BS, BW, BB���ޥ�ɤν��� */
static int log2i(int i);	/* log(2,i) */
static void getgatetime(void);
static void getgatetimeQ(void);
static void getktencho(void);
static void getvelocity(void);
static void get_randvel(void);
static void getvolume(void);
static void getexpression(void);
static void getexclusive(int);
static void variables(void);
static void code_E(void);
static void code__(void);
static void code___(void);
static void kakko1(void);
static void kakko2(void);
static void bankselect(void);
static void progchange(void);
static void getpanpot(void);
static void setRT(void);
static void keyoffvelocity(void);
static void code_M(void);
void mml_err(int i); /* ���顼���� */
static void code_F(void);
static void code_U(void);
static void code_I(void);
int check_cntchange(int, int);
static void put_cntchange(int, int, long);
static void put_cntchange0(int, int);
int check_bendchange(int);
static void put_bend(int, long);
static void put_bend0(int);
static void write_rpn(int, int, int); /* RPN�񤭹��� */
static void write_nrpn(int, int, int); /* NRPN�񤭹��� */
static void write_nrpn0(int, int, int); /* NRPN�񤭹��� */
static void detune(void);
static void getnrpn(void);
static void setpsw(void); /* #�����å����� */
void write_header(void);
static void cpres0(void);
static void put_cpres(int);
static void put_cpres0(int);
static void tvf(void);
static void tvf_c(void);
static void tvf_r(void);
static void set_transcale(void);
static void get_trackname(void);
static void write_trackname(void);
static void texts(void);
static void put_midistring(int);
static int mml_getstring(char *, int);
static void do_command(int);

extern int x_values[]; /* x_expression, x_panpot�Ĥ������ޤȤ᤿���� */

extern long first_F[];
extern long last_F[];
extern long first_I[];
extern long last_I[];

int alloc_tmap(void)
{
	tmap_ptr = 0;
	tmap = malloc((tmap_amount = 256) * sizeof(*tmap));
	return tmap != NULL ? 0 : -1;
}

 /* mmlproc�γƼ��̤��Ф����ͤ��
    (ɬ�פʤ�Τ�)���write_length(0,fp2)�⤷�Ƥ��� */
void do_mmlproc(int kind, int para)
{
	switch(kind){
	case MMLPROC_EXPR:
	case MMLPROC_PANPOT:
	case MMLPROC_VOL:
	case MMLPROC_MOD_ON:
		put_cntchange(kind2param(kind), para, 0);
		break;
	case MMLPROC_BEND:
		put_bend(para, 0);
		break;
	case MMLPROC_CUTOFF:
	case MMLPROC_RESO:
		write_nrpn(1, kind2param(kind), para);
		break;
	case MMLPROC_CPRES:
		put_cpres(para);
		break;
	default: /* case MMLPROC_DTEMPO: */
		add_tmap(TMAP_DIFF, para, tstep);
		break;
	}
}

void do_mmlproc0(int kind, int para, long st)
 /* �����å��ʤ��ǽ� �ޤ���ɬ�פʤ�����write_length()�򤹤뤳��
    st��DTEMPO�ξ������Ȥ��� */
{
	switch(kind){
	case MMLPROC_EXPR:
	case MMLPROC_PANPOT:
	case MMLPROC_VOL:
	case MMLPROC_MOD_ON:
		put_cntchange0(kind2param(kind), para);
		break;
	case MMLPROC_BEND:
		put_bend0(para);
		break;
	case MMLPROC_CUTOFF:
	case MMLPROC_RESO:
		write_nrpn0(1, kind2param(kind), para);
		break;
	case MMLPROC_CPRES:
		put_cpres0(para);
		break;
	default: /* case MMLPROC_DTEMPO: */
		add_tmap(TMAP_DIFF, para, tstep+st);
		break;
	} /* write_length()�ϸƤФ�ʤ� */
}

/* control change ��񤯤��ɤ���������å�����ؿ� */
/* �񤯾���0�򡢽񤫤ʤ����� -1 ���֤��� */
/* p1 == -1 �λ��ϡ�static �ѿ��ν���� */
/* p2 == -1 �λ��ϡ����⤻���˥꥿���� */
int check_cntchange(int p1, int p2)
{
	static int last_modulation = -1;
	static int last_volume = -1;
	static int last_panpot = -1;
	static int last_expression = -1;

	if(p2 == -1) return -1;
	if(p1 == -1){ /* �����ʥȥ�å��Υ���ѥ��롢�ޤ��ϡ�s���ޥ��ȯ�� */
		last_modulation = -1;
		last_volume = -1;
		last_panpot = -1;
		last_expression = -1;
		return -1;
	}

	switch(p1){
	case 1:
		if(p2 == last_modulation) return -1;
		last_modulation = p2;
		break;
	case 7:
		if(p2 == last_volume) return -1;
		last_volume = p2;
		break;
	case 10:
		if(p2 == last_panpot) return -1;
		last_panpot = p2;
		break;
	case 11:
		if(p2 == last_expression) return -1;
		last_expression = p2;
		break;
	default:
		break;
	}
	return 0;
}

/* control change ��񤯴ؿ����񤯤��ɤ��������å�������롣 */
/* �񤤤���˥��ƥåץ������� */
static void put_cntchange(int p1, int p2, long l)
{
	if(psw == 0){
		if(check_cntchange(p1, p2) == -1) return;
		put_cntchange0(p1, p2);
		write_length(l, fp2);
	}
}

/* control change ��񤯴ؿ��������å�̵���ǽ񤯡� */
static void put_cntchange0(int p1, int p2)
{
	if(run != 0xb){
		putc2(0xb0 + cur_ch, fp2);
		run = 0xb;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

/* bend ��񤯤��ɤ���������å�����ؿ� */
/* �񤯾���0�򡢽񤫤ʤ����� -1 ���֤��� */
/* p == -1 �λ��ϡ�static �ѿ��ν���� */
int check_bendchange(int p)
{
	static int last_bend = -1;

	if(p == -1){ /* �����ʥȥ�å��Υ���ѥ��롢�ޤ��ϡ�s���ޥ��ȯ�� */
		last_bend = -1;
		return -1;
	}

	if(p == last_bend) return -1;
	last_bend = p;
	return 0;
}

/* bend ��񤯴ؿ����񤯤��ɤ��������å�������롣 */
/* �񤤤���˥��ƥåץ������� */
static void put_bend(int p, long l)
{
	if(psw == 0){
		if(check_bendchange(p) == -1) return;
		put_bend0(p);
		write_length(l, fp2);
	}
}

/* bend ��񤯴ؿ��������å�̵���ǽ񤯡� */
static void put_bend0(int p)
{
	if(run != 0xe){
		putc2(0xe0 + cur_ch, fp2);
		run = 0xe;
	}
	putc2(p % 128, fp2);
	putc2(p / 128, fp2);
}

/* RPN�񤭹��� */
static void write_rpn(int tmsb, int tlsb, int tdat)
{
	int i;

	if(psw == 0){
		i = tmsb * 256 + tlsb;
		if(i != rpn_para) {
			put_cntchange0(101, tmsb);
			write_length(0, fp2);
			put_cntchange0(100, tlsb);
			write_length(0, fp2);
			rpn_para = i;
		}
		nrpn_para = -1;
		put_cntchange0(6, tdat);
		write_length(0, fp2);
	}
}

/* NRPN�񤭹��� */
static void write_nrpn(int tmsb, int tlsb, int tdat)
{
	if(psw == 0){
		write_nrpn0(tmsb, tlsb, tdat);
		write_length(0, fp2);
	}
}

/* length��񤫤ʤ�NRPN�񤭹���(psw����̵��) */
static void write_nrpn0(int tmsb, int tlsb, int tdat)
{
	int i;

	i = tmsb * 256 + tlsb;
	if(i != nrpn_para){
		put_cntchange0(99, tmsb);
		write_length(0, fp2);
		put_cntchange0(98, tlsb);
		write_length(0, fp2);
		nrpn_para = i;
	}
	rpn_para = -1;
	put_cntchange0(6, tdat);
}

/*
  tnum, talf���б�����ȥ�å����Ѵ����롥
  �ȥ�å��ˣ�ʸ����ʤ���硢-1���֤���
  */
int converttrk(void)
{
	int i;
	int code;

	/* �����Х��ѿ��ν���� */
	head = 0; /* (��°�ȥ�å���?)�ޤ��إå���񤤤Ƥ��ʤ� */
	master_count_temp = master_count_temp_prev = -1;
	 /* ��°�ȥ�å��ξ�硢�б���ȥ�å��򸫤Ĥ����餽����Ƭ��
	    ���ƥå׾�����Ǽ����master_step[]��ź����master_count_temp�� */
	master_count = (!tnum ? 0 : -1);
	 /* ��ȥ�å��ξ���gettrack()���master_step[master_count]�˥��ƥå�
	    �����񤤤Ƥ�master_count��ʤ�뤿��master_count��0���鳫��
	    ��°�ȥ�å��Ǥ��б������ȥ�å��򸫤Ĥ�����master_count��ʤ��
	    �Τ�master_count�ν���ͤ�-1 */
	last_oddlen.pos = -1; /* ��°track���ֹ�碌���ԥ����å����ѿ������ */

	cur_line = 1; /* ���ֹ� = 1 �����ɤ߻Ϥ�� */
	kloop_ptr = 0; /* �롼�ץͥ��ȿ� = 0 */
	cur_ch = 0; /* ���߽������MIDI�����ͥ� */
	run = 0; /* ���˥󥰥��ơ���������� */
	octave = 4; /* ���������� */
	under = 0; /* žĴ */
	transcale = 0; /* Ĵ�� */
	clength = timebase; /* l���ޥ�ɤΥ���å��� */
	 /* converttrk()���ƤФ��Τ�analyze()����Ǥ��뤳�Ȥ� timebase��
	    ���ꤹ��Τ�analyze()��������Ǥ��뤳�Ȥ�����Ȥ��� ���äƤ����Ǥ�
	    timebase�Ϥ⤦����Ѥ� */
	velocity = 100; /* �٥��ƥ��� */
	tstep = 0; /* �ȥ�å��θ��ߤΥ��ƥåץ����� */
	kakko_def = 4;
	/*
	volume = def_volume;
	expression = def_expression;
	bend = def_bend;
	panpot = def_panpot;
	*/
	cutoff = -1;
	resonance = -1;
	volume = -1;
	expression = -1;
	bend = -1;
	panpot = -1;
	mod_on = -1;

	cpres = -1;

	rpan1 = rpan2 = 0;
	rvel1 = rvel2 = 0;
	randvel1 = randvel2 = 0;
	mod_delay = -1;
	koff = 0; /* �������ե٥��ƥ��� */
	rpn_para = -1;
	nrpn_para = -1;
	psw = 0; /* '='�����å��ϣƣ� */
	prog = -1;
	bs1 = bs2 = 0;
	for(i = 0; i < KIND_MAX; i++){
		codef[i][0] = 0;
		codeu[i][0] = 0;
		codei[i][0] = 0;
		first_F[i] = 0; /* �ʲ����Ĥ�ǰ�Τ�������Ƥ����� */
		last_F[i] = 0;
		first_I[i] = 0;
		last_I[i] = 0;
	}
	setcode_I(0,0,-1); /* static�ѿ��ν���� */
	setcode_F(0,0,-1);
	base = 0; /* �ȥ�å���˻��ꤹ�� */
	for(i = 0; i < 256; i++){ /* �ѿ��ν���� */
		var[i] = 0;
	}

	ktencho = 0;	/* ����žĴ�ᥪ�� */
	gatetimeQ = 8;	/* Q���ޥ�ɤΥ����ȥ����� */
	gatetime = 1;	/* ��1�����ȥ����� */
	gatetime2 = 0;	/* ��2�����ȥ����� */
	andflag = 0;	/* & �����ä����ɤ����Υե饰 */
	koff = 0;	/* �������ա��٥��ƥ� */
	keyproc_ptr = 0; /* �������ջ��˽������٤����٥�ȤθĿ� */
	mmlproc_ptr = 0;
	keyproc2_ptr = 0; /* �����ȥ����ब�ޥ��ʥ����ä����θĿ� */
	check_cntchange(-1, 0); /* static �ѿ��ν���� */
	check_bendchange(-1); /* static �ѿ��ν���� */

	trackname[0] = '\0'; /* should not be `NULL' */

	code = getbyte(1);
	if(head == 0){ /* ��°�ѡ��ȤǤޤ��إå���񤤤Ƥ��ʤ� */
		if(code != -1 && code != '!'){
			/* �ȥ�å��˺���1ʸ�����ä������ǥإå���� */
			write_header();
	 		head = 1;
		}else return -1;
	}
	for(;;){
		code = getbyte(0);
		if(code == -1 || code == '!'){
			getbyte(-1);
			wsprintf(Msg, " %6ld step%c \"%s\"\n",
				tstep, tstep > 1 ? 's' : ' ', trackname);
			strcat(text,Msg);
#ifndef WINDOWS
			InvalidateRect(hWnd3,NULL,TRUE);
			UpdateWindow(hWnd3);
#endif
			if(kloop_ptr != 0)
				mml_err(1); /* �롼�פ���λ���Ƥʤ� */
			if(last_oddlen.pos >= 0){ /* ��°�ȥ�å���碌���� */
				cur_line = last_oddlen.lineno;
				mml_err(70);
			}
			write_keyproc2();
			write_restofmmlproc();
			return 0; /* 1�ĤΥȥ�å����Ѵ���λ */
		}
		do_command(code);
	}
}
static void do_command(int code)
{
	int i;

	switch(code){
	case 'C':
		cur_ch = (xget(&i) - 1) % 16;
			/* cur_ch �ˤϥ����ͥ�-1 (0��15) ������ */
		get_trackname();
		check_cntchange(-1, 0);
		check_bendchange(-1);
		run = 0;
		break;
	case '_':
		code__();
		break;
	case '*':
		if(!backcompati) goto syn_err;
		code___();
		break;
	case 'q':
		getgatetime();
		break;
	case 'Q':
		getgatetimeQ();
		break;
	case 'J':
		getktencho();
		break;
	case 'k':
		switch(getbyte(1)){
		case 'r':
			get_randvel();
			break;
		default:
			getvelocity();
			break;
		}
		break;
	case 'w':
		keyoffvelocity();
		break;
	case 's':
		check_cntchange(-1, 0);
		check_bendchange(-1);
		break;
	case 'v':
		getvolume();
		break;
	case '@':
		progchange();
		break;
	case 'D':
		detune();
		break;
	case 'E':
		code_E();
		break;
	case 'F':
		code_F();
		break;
	case 'U':
		code_U();
		break;
	case 'H':
		bankselect();
		break;
	case 'I':
		code_I();
		break;
	case 'N':
		getnrpn();
		break;
	case '~':
		bendset('S');
		break;
	case 'V':
		kakko_def = xget(&i);
		break;
	case ')':
		kakko1();
		break;
	case '(':
		kakko2();
		break;
	case 'p':
		getpanpot();
		break;
	case 'R':
		setRT();
		break;
	case 'M':
		code_M();
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'r':
	case '\\':
	case 'A': /* pori press */
		note(code);
		break;
	case 'K': /* �Ρ��ȥ���Ĵ�� */
		switch(getbyte(1)){
		case 'I': case 'J':
			set_transcale();
			break;
		default:
			note(code);
			break;
		}
		break;
	case 'h':
		if(!german_scale) goto syn_err;
		note(code);
		break;
	case '^':
		mml_warn(3);
		note('r');
		break;
	case 'l':
		clength = length();
		break;
	case 'o':
		oct();
		break;
	case '>':
		if(x68k == 0) octave++;
		else octave--;
		break;
	case '<':
		if(x68k == 0) octave--;
		else octave++;
		break;
	case 'y':
		code_y();
		break;
	case 't':
		tempo();
		break;
	case 'B':
		code_B();
		break;
	case '=':
		setpsw();
		break;
	case 'P':
		put_cntchange(64, 127, 0);
		break;
	case 'X':
		put_cntchange(64, 0, 0);
		break;
	case 'G':
		cpres0();
		break;
	case 'S':
		wsprintf(Msg, "\n --- H%d,%d @%d o%d v%d k%d E%d p%d BS%d(BW%d) "
			 "q%d,%d Q%d l%s%d (%ld step%s)\n        ",
			bs1, bs2, prog, octave, volume, velocity, expression,
			panpot, bend/128, bend,
			gatetime, gatetime2, gatetimeQ,
			(timebase * 4 % clength) ? "%" : "",
			(timebase * 4 % clength) ? clength : timebase * 4 / clength,
			tstep, tstep > 1 ? "s" : "");
		strcat(text,Msg);
		break;
	case 'T':
		tvf();
		break;
	case 'Z':
		variables();
		break;
	case 'W':
		texts();
		break;
	default:
	syn_err:
		wsprintf(Msg, "%c", code);
		warnmode ? mml_warn(1) : mml_err(64);
	}
	 /* ��[�ס�]�פ�getbyte()����ƤФ��track()��Ÿ�����Ƥ��뤫��
	    �����Ǥν��������� */
}

int ichi; /* =1 �����ä����ɤ����Υե饰 */

/*
 * '='�����å��ν���
 */
static void setpsw(void)
{
	int i, num;

	num = xget(&i);
	if(i == -2){
		if(psw == 0) psw = 1;
		else psw = 0;
	}else{
		if(num != 0 && num != 1) mml_err(35);
		else psw = num;
	}
	if(psw == 1){
		ichi = 1;
		prog = -1;
		write_keyproc2();
		write_restofmmlproc();
		return;
	}else if(ichi == 1){
		ichi = 0;
		if(prog != -1){ /* ����ǥץ��������󥸤����İʾ夢�ä���� */
			put_cntchange0(0, bs1);
			write_length(0, fp2);
			put_cntchange0(32, bs2);
			write_length(0, fp2);
			/* if(run != 0xc){} ���˥󥰥��ơ�������̵�뤹�� */
			putc2(0xc0 + cur_ch, fp2);
			run= 0xc;
			putc2(prog, fp2);
			write_length(0, fp2);
		}
		if(mod_on != -1) put_mod(mod_on);
		if(volume != -1) put_vol(volume);
		if(panpot != -1) put_panpot(panpot);
		if(expression != -1) put_expr(expression);
		if(cpres != -1) put_cpres(cpres);
	}
}

/* mml��1�ȥ�å�ʬ�Ѵ���������ľ��ˡ�
   ���λ���ȯ����λĤ�β��򥭡����դ��롥

   �ʤ���������������ɬ�פ��ȸ����ȡ�
   �����ȥ����ब�ޥ��ʥ��ξ��ˡ����������������դ���ʤ�����
   �Ĥ��礬���뤫�顥
   */
static void write_keyproc2(void)
{
	int i;

	for(i = 0; i < keyproc2_ptr; i++){
		noteoff(keyproc2[i].kp_onkai, keyproc2[i].kp_velo);
		write_length(0, fp2);
	}
}

/* mml��1�ȥ�å�ʬ�Ѵ���������ľ��ˡ�
   ���λ���ȯ����λĤ�β��򥭡����դ��롥

   �����ȥ����ब�ޥ��ʥ��ǡ������⤽���ͤ�����ʻ���
   keyproc2�ǤϤʤ���mmlproc��ȯ����β����ĤäƤ����礬���롣
   */
static void write_restofmmlproc(void)
{
	int i;

	for(i = 0; i < mmlproc_ptr; i++){
		if(mmlproc[i].kind == -1){ /* ���̡ᥭ������ */
			noteoff(mmlproc[i].param_onkai, mmlproc[i].param_velo);
			write_length(0, fp2);
		}
	}
}

/* ) �ν��� */
static void kakko1(void)
{
	int num, i;

	num = xget(&i);
	if(x68k2 == 0){
		if(i == -2) velocity += kakko_def;
		else velocity += num;
	}else{
		if(i == -2) velocity -= kakko_def;
		else velocity -= num;
	}
	ALIGN(velocity);
}

/* ( �ν��� */
static void kakko2(void)
{
	int num, i;

	num = xget(&i);
	if(x68k2 == 0){
		if(i == -2) velocity -= kakko_def;
		else velocity -= num;
	}else{
		if(i == -2) velocity += kakko_def;
		else velocity += num;
	}
	ALIGN(velocity);
}

static int tmap_timing_comp(struct tmap *t1, struct tmap *t2)
{ /* tmap����������Ӵؿ� */
	if(t1->st < t2->st) return -1;
	if(t1->st > t2->st) return 1;
	if(t1->map == TMAP_DIFF){
		if(t2->map != TMAP_DIFF) return 1;
	} else {
		if(t2->map == TMAP_DIFF) return -1;
	} /* ��ʬ����Ϥ���ʳ����� */
	return t1->index < t2->index ? -1 : 1;
	 /* Ʊ����Ʊ�ΤǤν��֤���¸ */
}
static unsigned long tempo_conv(int tempo)
{
	unsigned long i4, i_mod;

	if(tempo <= 0) tempo = 1;

	/* i4 = 60000000UL * 100 / (tempo_master * tempo);
	   �ʤΤ����������̤�ˤ��ȥ����С��ե�����Τ� */
	tempo *= tempo_master;
	i4 = 3000000000UL / tempo * 2;
	i_mod = 3000000000UL % tempo * 4;
	if(i_mod >= tempo) i4++;
	if(i_mod >= 3*tempo) i4++;

	if(i4 >= 1UL<<24) i4 = (1UL<<24)-1;
	return i4;
}

/* �ƥ�ݥޥåפؤν񤭹��� */
void write_tmap(void)
{
	int i, cur_tempo = 0, cur_base_tempo = 0;
	long last_st = 0;
	struct {
		int *area, amount, ptr;
	} tempo_stack;

	tempo_stack.area = malloc(sizeof(int) * (tempo_stack.amount = 30));
	if(tempo_stack.area == NULL) mml_err(61);
	tempo_stack.ptr = 0;

	qsort(tmap, tmap_ptr, sizeof(*tmap), (int (*)())tmap_timing_comp);
	for(i = 0; i < tmap_ptr; i++){
		unsigned long i4;
		
		write_length(tmap[i].st - last_st, fp3);
		last_st = tmap[i].st;
		switch(tmap[i].map){
		case TMAP_BEAT: /* beat �������¾�Τ������̽��� */
			putc2(0xff, fp3);
			putc2(0x58, fp3);
			putc2(4, fp3);
			putc2(tmap[i].p1 / 100, fp3);
			putc2(log2i(tmap[i].p1 % 100), fp3);
			putc2(0x18, fp3);
			putc2(8, fp3);
			continue;

		case TMAP_DIFF: /* temporally add tempo */
			cur_tempo = cur_base_tempo + tmap[i].p1;
			break;
		case TMAP_POP: /* pop tempo */
			if(!tempo_stack.ptr){
				/* include�ե���������֤ʤɤ��θ�����
				   ���顼�ι��ֹ��Ф��Τϼ㴳���ݤʤΤ�
				   �Ȥꤢ�������ֹ�ʤ����顼��å������Ф� */
				fprintf(STDERR,	"ERROR!  't)' without 't('\n");
				remove_file_and_owari();
			} else {
				cur_tempo = cur_base_tempo =
					tempo_stack.area[--tempo_stack.ptr];
			}
			break;
		default:
			if(tmap[i].map & TMAP_PUSH){ /* push tempo */
				if(tempo_stack.ptr>=tempo_stack.amount){
					tempo_stack.amount = tempo_stack.ptr + 30;
					tempo_stack.area = realloc(tempo_stack.area,
						sizeof(int) * tempo_stack.amount);
					if(tempo_stack.area == NULL) mml_err(61);
				}
				tempo_stack.area[tempo_stack.ptr++] = cur_tempo;
			}
			if(tmap[i].map & TMAP_RELATIVE){ /* relative */
				cur_tempo = (cur_base_tempo += tmap[i].p1);
			} else {
				cur_tempo = cur_base_tempo = tmap[i].p1;
			}
			break;
		}

		/* beat�ξ��ʳ��ζ��̽��� */
		i4 = tempo_conv(cur_tempo);
		putc2(0xff, fp3);
		putc2(0x51, fp3);
		putc2(0x03, fp3);
		putc2((char)(i4>>16),fp3);
		putc2((char)(i4>>8), fp3);
		putc2((char)i4,	fp3);
	}
	write_length(0, fp3);
	if(tempo_stack.ptr != 0){
		/* �ٹ��å����������Ʊ����ͳ�ǹ��ֹ�Ф��ʤ� */
		strcpy(text, "Warning: 't(' not closed by 't)'\n");
		InvalidateRect(hWnd3, NULL, TRUE);
		UpdateWindow(hWnd3);
	}
	free(tempo_stack.area);
	tmap_ptr = 0; /* ���פ���? */
}

static void tmap_realloc(void) /* Add Nide */
{
	if(tmap_ptr >= tmap_amount){
		tmap_amount = tmap_ptr + 512;
		if((tmap = realloc(tmap, tmap_amount * sizeof(*tmap))) == NULL){
			mml_err(61);
		}
	}
}

static void add_tmap(int map, int num, long step)
{
	tmap[tmap_ptr].index = tmap_ptr;
	tmap[tmap_ptr].map = map;
	tmap[tmap_ptr].p1 = num;
	tmap[tmap_ptr].st = step;
	tmap_ptr++;
	tmap_realloc();
}

/*
  �ƥ�ݡ����٥�Ȥν�����
  �ƥ�ݡ��ޥåפ˽񤯡�
  �ǽ�˥��ƥåץ������񤤤Ƥ���ƥ�ݥ��٥�Ȥ�񤯡�
  */
static void tempo(void)
{
	int i, num, map;

	switch(getbyte(1)){
	case '(':
		(void)getbyte(2);
		num = xget(&i);
		map = TMAP_PUSH; /* push tempo */
		break;
	case ')':
		(void)getbyte(2);
		num = 0; i = 2;
		map = TMAP_POP; /* pop tempo */
		break;
	default:
		num = xget(&i);
		if(i == -2) mml_err(10); /* �ͤ��ʤ� */
		map = TMAP_NORMAL; /* normally specify tempo */
		break;
	}

	switch(i){
	case 2: /* pop */
		break;
	case 0: /* ľ�ܻ��� */
		if(!num) mml_err(10); /* �ͤ�0 */
		break;
	default: /* ���л��� */
		map |= TMAP_RELATIVE;
		break;
	}
	add_tmap(map, num, tstep);
}

/*
  B���褿�Ȥ��ν�����
  BS, BB, BR, BT, BW �Υ��ޥ�ɤ����롣
  */
static void code_B(void)
{
	int kind;

	switch(kind = getbyte(0)){
	case 'T':
		beat();
		break;
	case 'R':
		bendrange();
		break;
	case 'S':
	case 'B':
	case 'W':
		bendset(kind);
		break;
	default:
		mml_err(13);
	}
}

/* log(2,i), modified by Nide */
static int log2i(int i)
{
	int j = 0;

	while(i /= 2) j++;
	return j;
}

/*
  BT���ޥ�ɤν���
  �ƥ�ݡ��ޥåפ˽񤯡�
  �ǽ�˥��ƥåץ������񤤤Ƥ���ƥ�ݥ��٥�Ȥ�񤯡�
  */
static void beat(void)
{
	int i, num, num2;

	num = xget(&i);
	if(i != 0) mml_err(8);
	if(getbyte(1) != ',') mml_err(8); /* BT?�θ�˥���ޤ��ʤ� */
	(void)getbyte(2);
	num2 = xget(&i);
	if(i != 0) mml_err(8);

	add_tmap(TMAP_BEAT, num * 100 + num2, tstep);
}

/* channel press ���ޥ�ɤν��� */
static void cpres0(void)
{
	int old, first = 0;

	if(cpres == -1) first = 0, cpres = def_cpres; /* Add Nide */
	old = cpres;
	if((cpres = soutai0(cpres)) == -1) mml_err(40);
	first ? (x_cpres = 0) : (x_cpres += cpres - old); /* ¾��·���� */
	if(psw == 0){
		if(run != 0xd){
			putc2(0xd0 + cur_ch, fp2);
			run = 0xd;
		}
		putc2(cpres, fp2);
		write_length(0, fp2);
	}
}

/* channel press ���ޥ�ɤν����ѡ��ȣ� */
void put_cpres(int x)
{
	if(psw == 0){
		put_cpres0(x);
		write_length(0, fp2);
	}
}

/* channel press ���ޥ�ɤν����ѡ��ȣ� */
static void put_cpres0(int x)
{
	if(run != 0xd){
		putc2(0xd0 + cur_ch, fp2);
		run = 0xd;
	}
	putc2(x, fp2);
}

/* BR���ޥ�ɤν��� */
static void bendrange(void)
{
	int i, num;

	num = xget(&i);
	if(i != 0) mml_err(11);
	write_rpn (0,0,num);
}

/* BS, BW, BB���ޥ�ɤν��� */
static void bendset(int kind)
{
	int old, first = 0, bend_h, bend_l, num, i;

	if(bend == -1) first = 1, bend = def_bend; /* Add Nide */
	old = bend;

	num = xget(&i);
	switch(kind){
	case 'S': /* BS */
		bend_h = bend / 128;
		bend_l = bend % 128;

		switch(i){
		case -2:
			mml_err(17);
		case 0:
			bend_h = num;
			break;
		default: /* ���л���ξ�� */
			bend_h += num;
			break;
		}
		if(getbyte(1) != ','){
			bend_l = 0;
		} else {
			(void)getbyte(2);
			num = xget(&i);
			switch(i){
			case -2:
				mml_err(17);
			case 0:
				bend_l = num;
				break;
			default: /* ���л���ξ�� */
				bend_l += num;
				break;
			}
		}
		bend = bend_h * 128 + bend_l;
		break;
	case 'W': /* BW */
		switch(i){
		case -2:
			mml_err(34);
		case 0:
			bend = num;
			break;
		default: /* ���л���ξ�� */
			bend += num;
			break;
		}
		break;
	default: /* case 'B': */	/* BB���ޥ��; obsolete */
		switch(i){
		case -2:
			mml_err(18);
		default: /* 0�Ǥ����л���ȸ��ʤ� */
			bend += num * 128;
			break;
		}
	}
	ALIGN2(bend);

	first ? (x_bend = 0) : (x_bend += bend - old);
	if(check_bendx() == 0) put_bend(bend, 0);
}

/* y���ޥ�ɤν��� */
static void code_y(void)
{
	int num, num2, i;

	num = xget(&i);
	if(i != 0) mml_err(9); /* �ͤ��� */
	if(getbyte(0) != ',') mml_err(9); /* y?�θ�˥���ޤ��ʤ� */
	num2 = xget(&i);
	if(i != 0) mml_err(9); /* �ͤ��� */
	put_cntchange(num, num2, 0);
}

/* N���ޥ�ɤν��� */
static void getnrpn(void)
{
	int num, num2, num3, i;

	num = xget(&i);
	if(i != 0) mml_err(32);
	if(getbyte(0) != ',') mml_err(32);
	num2 = xget(&i);
	if(i != 0) mml_err(32);
	if(getbyte(0) != ',') mml_err(32);
	num3 = xget(&i);
	if(i != 0) mml_err(32);
	if(getbyte(1) == ',') mml_err(33);
	write_nrpn(num, num2, num3);
}

/* H���ޥ�ɤν��� */
static void bankselect(void)
{
	int i;

	bs1 = xget(&i);
	if(i != 0) mml_err(22); /* �ͤ��� */
	if(getbyte(1) != ',') mml_err(22); /* y?�θ�˥���ޤ��ʤ� */
	(void)getbyte(2);
	bs2 = xget(&i);
	if(i != 0) mml_err(22); /* �ͤ��� */
	if(psw == 0){
		put_cntchange0(0, bs1);
		write_length(0, fp2);
		put_cntchange0(32, bs2);
		write_length(0, fp2);
	}
}

/* v���ޥ�ɤν��� */
static void getvolume(void)
{
	int old, first = 0;

	if(volume == -1) first = 1, volume = def_volume; /* Add Nide */
	old = volume;
	if((volume = soutai0(volume)) == -1) mml_err(15);
	first ? (x_volume = 0) : (x_volume += volume - old);
	if(check_vol() == 0) put_vol(volume);
}

/* p���ޥ�ɤν��� */
static void getpanpot(void)
{
	int i;
	int old, first = 0;

	if(panpot == -1) first = 1, panpot = def_panpot; /* Add Nide */
	old = panpot;
	if((panpot = soutai0(panpot)) == -1) panpot = 2;
	 /* �ޥ˥奢��Ǥϥѥ�ݥåȤο��ͤ�ά�����2���ꤹ�뤳�ȤˤʤäƤ�*/
	first ? (x_panpot = 0) : (x_panpot += panpot - old);
	if(check_panpot() == 0)	put_panpot(panpot);
	if(getbyte(1) != ','){
		rpan1 = 0;
		return;
	}
	(void)getbyte(2);
	rpan1 = xget(&i);
	if(getbyte(1) == ','){
		(void)getbyte(2);
		rpan2 = xget(&i);
	}
}

/* @���ޥ�ɤν��� */
static void progchange(void)
{
	int num, i;

	num = xget(&i);
	if(i == -1 || i == 1){
		base = num;
		return;
	} else if(i == 0){
		prog = num + base;
	} else mml_err(23);
	/* prog &= ~0x80; ������? */
	if(psw == 0){
		if(run != 0xc){
			putc2(0xc0 + cur_ch, fp2);
			run= 0xc;
		}
		putc2(prog, fp2);
		write_length(0, fp2);
	}
}

/* E���ޥ�ɤν��� */
static void getexpression(void)
{
	int old, first = 0;

	if(expression == -1) first = 1, expression = def_expression;
	 /* Add Nide */
	old = expression;
	if((expression = soutai0(expression)) == -1) mml_err(16);
	first ? (x_expression = 0) : (x_expression += expression - old);
	if(check_expr() == 0) put_expr(expression);
}

/* q���ޥ�ɤν��� */
/*
  qn0,n1
	n0 == gatetime  �ޥ��ʥ��Ǥ��ɤ���
	n1 == gatetime2
  q-4 a4b4 ���ȡ�b�򥭡������4step�ФäƤ���a���������դ���롣
  ���ޤǤΤ褦��q�Υޥ��ʥ����ޥ�ɤ�̵���ʤä���
  ���ޤǤ� q-4 �Τ褦�ˤ�ꤿ����С�q100,4 �Τ褦�ˤ�����ɤ����Ĥޤꡢ
  ��1�ѥ�᡼�����ͤ������礭�����롣
*/
static void getgatetime(void)
{
	int num, i;

	num = xget(&i);
	if(i != -2) gatetime = num;
	if(getbyte(1) != ',') return; /* q?�θ�˥���ޤ��ʤ� */
	(void)getbyte(2);
	num = xget(&i);
	if(i == 0) gatetime2 = num;
}

/* Q���ޥ�ɤν��� */
static void getgatetimeQ(void)
{
	int num, i;

	num = xget(&i);
	if(num < 0 || num > 8) mml_err(41);
	if(i != -2) gatetimeQ = num;
}

/* J���ޥ�ɤν��� */
static void getktencho(void)
{
	int num, i;

	num = xget(&i);
	if(num < 0) mml_err(42);
	if(i != -2) ktencho = num; /* num!=0�ʤ�ж���žĴ��ͭ���ˤʤ� */
}

/* k���ޥ�ɤν��� */
static void getvelocity(void)
{
	int i;

	if((velocity = soutai0(velocity)) == -1) mml_err(14);
	if(getbyte(1) != ','){
		rvel1 = 0;
		return;
	}
	(void)getbyte(2);
	rvel1 = xget(&i);
	if(getbyte(1) == ','){
		(void)getbyte(2);
		rvel2 = xget(&i);
	}
}

/* kr���ޥ�ɤν��� */
static void get_randvel(void)
{
	int i;

	(void)getbyte(2);
	randvel1 = xget(&i);
	if(getbyte(1) == ','){
		(void)getbyte(2);
		randvel2 = xget(&i);
	}else{
		randvel2 = 1; /* ��2�ѥ�᡼����ά����default�� */
	}
}

/*
  T���褿�Ȥ��ν�����
  TC, TR���ޥ�ɤ����롣
  */
static void tvf(void)
{
	switch(getbyte(0)){
	case 'C':
		tvf_c();
		break;
	case 'R':
		tvf_r();
		break;
	default:
		mml_err(68);
	}
}

static void tvf_c(void)
{
	int old, first = 0;

	if(cutoff == -1) first = 1, cutoff = def_cutoff; /* Add Nide */
	old = cutoff;
	if((cutoff = soutai0(cutoff)) == -1) mml_err(54);
	first ? (x_cutoff = 0) : (x_cutoff += cutoff - old);
	if(check_cutoff() == 0)	put_cutoff(cutoff);
}

static void tvf_r(void)
{
	int old, first = 0;

	if(resonance == -1) first = 1, resonance = def_resonance;
	 /* Add Nide */
	old = resonance;
	if((resonance = soutai0(resonance)) == -1) mml_err(55);
	first ? (x_resonance = 0) : (x_resonance += resonance - old);
	if(check_reso() == 0) put_reso(resonance);
}

static void set_transcale(void) /* KJ, KI���ޥ�ɡ�KJa+�Τ褦�˻��� */
{
	int code, tr = 0, i, no_sharpflat = 0;

	if(getbyte(0) == 'I') tr = -3; /* else getbyte(1)=='J' */
	if(!('a' <= (code = getbyte(0)) && code <= 'h')) mml_err(69);
	if(german_scale){
		switch(code){
		case 'b':
			tr -= 7; no_sharpflat = 1; break;
		case 'h':
			code = 'b'; break;
		}
	} else {
		if(code == 'h') mml_err(69);
	}
	tr += onpu2chogo(code);
	if(!no_sharpflat){
		switch(code = getbyte(1)){
		case '+':
		case '-':
			i = 1;
			(void)getbyte(2);
			if(code == getbyte(1)){
				i++;
				(void)getbyte(2);
			}
			tr += (code == '+' ? 7 : -7) * i;
			break;
		}
	}
	transcale = tr;
}

/*
  E���褿�Ȥ��ν�����
  E, EX, EE �Υ��ޥ�ɤ����롣
  */
static void code_E(void)
{
	switch(getbyte(1)){
	case 'X':
		(void)getbyte(2);
		getexclusive(0);
		break;
	case 'E':
		(void)getbyte(2);
		getexclusive(1);
		break;
	default:
		getexpression();
	}
}

/* _���ޥ�ɤν��� */
static void code__(void)
{
	int i, num;

	if(getbyte(1) == '_'){
		(void)getbyte(2);
		code___();
	}else{
		num = xget(&i);
		if(i == -2) mml_err(21);
		under = num;
	}
}

/* __���ޥ�ɡ�����ӡ�*���ޥ�ɤν��� */
static void code___(void)
{
	int i, num;
	num = xget(&i);
	if(i == -2) mml_err(20);
	under += num;
}

/* EX���ޥ�ɤ�¿ʬ�����Ok�Ȼפ��� */
/* �㤨�� EXx41,x10,x42,x12,{x40,x00,x7f,x00},xf7 */
/* �Ȥ����{}���SUM��}��ʬ�˽񤫤��ʤϤ���*/
/* EX,EE ���ޥ�ɤν��� */
/* x == 0�ʤ��EX�� x == 1�ʤ��EE */
static void getexclusive(int x)
{
	int i,j,k,bulk_code;
	int exclusive[1024];
	int ex_pnt = 0;
	int sum_pnt,bulk_pnt,pntd,pnts;
	int ci,cj;
	int bit_size,bit_ptr;
	unsigned short bit_temp,b2;

	sum_pnt=-1;
	bulk_pnt=-1;
	bulk_code=0;
	bit_size=8;
	bit_ptr=0;
	bit_temp=0;

	for(;;){
		j=xget(&i);
		if (bulk_pnt==-1) {
			if (i!=0) mml_err(19);
		} else {
			if (i==-2) mml_err(19);		/*Bulk�������*/
		}
		if (bit_size==8&&bit_ptr==0) {
			exclusive[ex_pnt++] = j;
		} else {
			if (j>=(1<<bit_size)||j<-(1<<(bit_size-1))) mml_err(19);
			b2=(unsigned short)j;
			b2=b2<<(16-bit_size);	/*��ü�˵ͤ��*/
			b2=b2>>bit_ptr;		/*���Ѻ�ʬ����*/
			bit_temp|=b2;
			bit_ptr+=bit_size;
			if (bit_ptr>=8) {
				exclusive[ex_pnt++]=bit_temp>>8;
				bit_temp&=0xff;
				bit_temp<<=8;
				bit_ptr-=8;
			}
			bit_size=8;	/*����ϣ���Τ�ͭ���ʤΤ��᤹*/
		}
	aft:
		i = getbyte(1);
		if (i=='}'){
			if (bit_ptr!=0) {	/*ü���ӥåȤ����Ƥ��ޤ�*/
				exclusive[ex_pnt++]=bit_temp>>8;
				bit_ptr=0;
				bit_temp=0;
			}
			if (bulk_pnt!=-1) {	/*�ޤ��Х륯�������*/
				switch (bulk_code) {
				case 1:	/*TR-Rack*/
				  	j=ex_pnt-bulk_pnt;	/*�Ѵ���������*/
					k=((j-1)/7)+1;		/*�Ľ�����(=���ȥХ�����������뤫)*/
					pntd=ex_pnt+k-1;	/*ž����ݥ���*/
					pnts=ex_pnt-1;		/*ž�����ݥ���*/
					i=j%7;
					j=j/7;
					ci=0;
					if (i) {	/*���ž����ü��ʬ��*/
						while (i>0) {
							cj=exclusive[pnts];
							ci*=2;
							ci+=cj/128;
							exclusive[pntd]=cj&0x7f;
							pnts--;
							pntd--;
							i--;
						}
						exclusive[pntd]=ci;
						pntd--;
					}
					while (j>0) {	/*�����ܰʹ�ž��*/
						ci=0;
						for (i=0;i<7;i++) {
							cj=exclusive[pnts];
							ci*=2;
							ci+=cj/128;
							exclusive[pntd]=cj&0x7f;
							pnts--;
							pntd--;
						}
						exclusive[pntd]=ci;
						pntd--;
						j--;
					}
					ex_pnt+=k;	/*�������줿ʬ�û�*/
					break;

				case 2:	/* GS Niblize */
				  	j=ex_pnt-bulk_pnt;	/*�Ѵ���������*/
					pnts=ex_pnt-1;
					pntd=ex_pnt+j-1;
					ex_pnt+=j;
					while (j>0) {
						ci=exclusive[pnts--];
						exclusive[pntd--]=ci%16;
						exclusive[pntd--]=ci/16;
						j--;
					}
					break;

				default:
					mml_err(19);
				}
				bulk_code=0;
				bulk_pnt=-1;
			} else {	/*�Х륯��Ǥʤ���硢�����å�����*/
				if (sum_pnt!=-1) {	/*�Ф򸫤Ĥ��Ƥ�����Τ߽����򤹤�*/
					i=ex_pnt-sum_pnt;
					j=0;
					while (i>0) {
						j=(j+exclusive[sum_pnt++])&0x7f;
						i--;
					}
					exclusive[ex_pnt++] = (0x80 - j) & 0x7f;
					sum_pnt=-1;
				}
			}
			(void)getbyte(2);
			i = getbyte(1);
		}
		if(i != ',') break;
		(void)getbyte(2);

		i = getbyte(1);
		if (i=='{') {
			(void)getbyte(2);
			i=getbyte(1);
			if (i=='T'||i=='t') {
				bulk_pnt=ex_pnt;
				bulk_code=1;
				(void)getbyte(2);
			} else if (i=='N'||i=='n') {
				bulk_pnt=ex_pnt;
				bulk_code=2;
				(void)getbyte(2);
			} else {
				sum_pnt=ex_pnt;
				if ((sum_pnt>bulk_pnt)&&bulk_pnt!=-1) mml_err(19);
			}
		}
		i = getbyte(1);
		if(i == '"'){ /* Mod Nide */
			unsigned char text[1024]; /* ���ΤȤ���������Ϲͤ��Ƥʤ� */
			int got, j;

			got = mml_getstring((char *)text, 1024);
			for(j = 0; j < got; j++) exclusive[ex_pnt++] = text[j];
			goto aft;
		} else if (i=='B'||i=='b') {
			if (bulk_pnt==-1) mml_err(19);	/*�Х륯��ǤΤ�ͭ��*/
			(void)getbyte(2);
			bit_size=getbyte(0)-'0';
			if (bit_size<1||bit_size>8) mml_err(19);
			i=getbyte(0);
			if (i!='_') mml_err(19);
		}
	}
	if (bulk_pnt!=-1||sum_pnt!=-1) mml_err(19);	/*�Ф��Ĥ��Ƥ��ʤ�*/
	if (bit_ptr!=0) {	/*ü���ӥåȤ����Ƥ��ޤ�*/
		exclusive[ex_pnt++]=bit_temp>>8;
		bit_ptr=0;
		bit_temp=0;
	}
	if(x == 0){
		putc2(0xf0, fp2);
		if (ex_pnt>127) {
			putc2((ex_pnt/128)+128,fp2);
			putc2((ex_pnt%128) ,fp2);
		} else {
			putc2(ex_pnt, fp2);
		}
	}
	for(i = 0; i < ex_pnt; i++){
		putc2(exclusive[i], fp2);
	}
	write_length(0, fp2);
}

/* �ѿ��򥻥åȤ��륳�ޥ�ɡ��㡧 Z{z0 = 120, z1 = x3a} */
/* Z{z4 = z3 + 55, Z2 = C - 30} �ʤɤ�ϣˡ� */
static void variables(void)
{
	int code, i;
	int index;

	code = getbyte(1);
	if(code != '{') mml_err(50);
	(void)getbyte(2);

	for(;;){
		code = getbyte(1);
		if(code == '}'){
			(void)getbyte(2);
			break;
		}

		code = getbyte(1);
		if(code != 'z') mml_err(50); /* 'z'���ʤ���� */
		(void)getbyte(2);

		index = 0;
		for(;;){
			code = getbyte(1);
			if(is_digit(code)){
				index = index * 10 + dtoi(code);
				(void)getbyte(2);
			}else break;
		}
		if(index < 0 || 255 < index)
			mml_err(50); /* z0 �� z255 ���ϰϤˤʤ� */

		code = getbyte(1);
		if(code != '=') mml_err(50); /* =���ʤ���� */
		(void)getbyte(2);

		var[index] = xget(&i);
		/* if(i == -2) mml_err(50); */
		code = getbyte(1);

		if (code == ','){
			(void)getbyte(2);
			continue;
		}else if(code == '}'){
			(void)getbyte(2);
			break;
		}
		mml_err(50);
	}
}

/* D���ޥ�ɽ�����Detune��= RPN#1:FineTune����� */
/* Dn .... n�� -64��63 */
static void detune(void)
{
	int i, num;

	num = xget(&i);
	if(i == -2) mml_err(31);
	num += 0x40;
	ALIGN(num);
	write_rpn(0, 1, num);
}

/* RT���ޥ�ɤν��� */
static void setRT(void)
{
	if(getbyte(0) != 'T') mml_err(24);
	gatetime2 = -1;
	/* gatetime = 32767;*/ /* �����κ� */
	/*gatetime2 = 1;*/
}

/* o���ޥ��(����������)�ν��� */
static void oct(void)
{
	int num, i;

	num = xget(&i);
	if(i == -2) mml_err(7);
	if(i != 0){
		octave += num;
	}else{
		octave = num;
	}
}

static int FUIcmd2intl(int code, int *factor)
{
	int retval;

	switch(code){
	case 'E':
		retval = MMLPROC_EXPR; break;
	case 'P':
		retval = MMLPROC_PANPOT; break;
	case 'V':
		retval = MMLPROC_VOL; break;
	case 'W': case 'B':
		retval = MMLPROC_BEND; break;
	case 'C':
		retval = MMLPROC_CUTOFF; break;
	case 'R':
		retval = MMLPROC_RESO; break;
	case 'M':
		retval = MMLPROC_MOD_ON; break;
	case 'A':
		retval = MMLPROC_CPRES; break;
	case 'T':
		retval = MMLPROC_DTEMPO; break;
	 /* ����κ���+1��mmlproc.h��KIND_MAX */
	default:
		return -1;
	}

	 /* code{f,u,i}������뿶��������β��ܤˤ��뤫 */
	*factor = (code == 'B' ? 128 : each_factor[retval]);
	 /* B�ξ�������code{f,u,i}���ͤ�bend��1�ܤ���������128�� */

	return retval;
}

static void code_F(void)
{
	int y, i, num, j, factor;
	
	y = FUIcmd2intl(getbyte(0), &factor);
	if(y < 0) mml_err(29);

	num = xget(&i);
	if(i == -2 || num < 0) mml_err(25);
	codef[y][0] = num; /* ��1���� */
	if(num == 0){
		if(psw == 0 && check_proc(y) == 0)
			do_mmlproc(y, ctl_values[y]);
		return;
	}

	if(getbyte(0) != ',' || (num = xget(&i), i == -2) ||
	   getbyte(0) != ',') mml_err(25);
	codef[y][1] = num * factor; /* ��2���� */
	for(j = 3;;){
		codef[y][j++] = length(); /* ��3,5,�İ��� */
		if(getbyte(0) != ',' || (num = xget(&i), i == -2)) mml_err(25);
		codef[y][j++] = num * factor; /* ��4,6,�İ��� */
		if(getbyte(1) != ',') break;
		if(j == MAX_F_ARG+1) mml_err(25); /* too long */
		(void)getbyte(2);
	}
	codef[y][2] = (j - 3) / 2;
}

static void code_U(void)
{
	int y, i, num, j, factor;
	long tl = 0;

	y = FUIcmd2intl(getbyte(0), &factor);
	if(y < 0) mml_err(52);

	num = xget(&i);
	if(i == -2 || num <= 0) mml_err(53);
	codeu[y][0] = num; /* ��1���� */

	if(getbyte(0) != ',' || (num = xget(&i), i == -2) ||
	   getbyte(0) != ',') mml_err(53);
	codeu[y][1] = num * factor; /* ��2���� */
	for(j = 3;;){
		tl += (codeu[y][j++] = length()); /* ��3,5,�İ��� */
		if(getbyte(0) != ',' || (num = xget(&i), i == -2)) mml_err(53);
		codeu[y][j++] = num * factor; /* ��4,6,�İ��� */
		if(getbyte(1) != ',') break;
		if(j == MAX_U_ARG+1) mml_err(53); /* too long */
		(void)getbyte(2);
	}
	codeu[y][2] = (j - 3) / 2;
	if(psw == 0){
		setcode_U(tl, y); /* �����ʤ�mmlproc���ͤ�񤭹��� */
	}
}

static void code_I(void)
{
	int y, i, num, j, factor;

	y = FUIcmd2intl(getbyte(0), &factor);
	if(y < 0) mml_err(30);

	for(j = 0;;){
		num = xget(&i);
		if(i == -2) mml_err(30);
		codei[y][j++] = num;
		if(',' != getbyte(1)) break;
		if(j == 5) mml_err(30);
		(void)getbyte(2);
	} /* ���λ����� j:�����Ŀ� num:�Ǹ�ΰ��� */

	if(j == 1 && num == 0){ /* �ѥ�᡼����1�Ĥ����Ǥ��Ĥ��줬0�λ� */
		if(psw == 0 && check_proc(y) == 0)
			do_mmlproc(y, ctl_values[y]);
		return;
	}

	if(j < 4) mml_err(30); /* �ѥ�᡼����4�Ĥ�꾯�ʤ��ΤϤ��� */
	codei[y][0] *= factor;
	if(j == 4) codei[y][4] = 0; /* 5���ܤ�ά�����0 */
	if(codei[y][0] == 0 || codei[y][1] <= 0 || codei[y][2] < 0 ||
	   codei[y][3] <= 0 || codei[y][4] < 0) mml_err(59);
}

/* w���ޥ��(�������ե٥��ƥ���)�ν��� */
static void keyoffvelocity(void)
{
	int num, i;

	num = xget(&i);
	if(i == -2) mml_err(37);
	if(i != 0){
		koff += num;
	}else{
		koff = num;
	}
}

/* MON, MOF, M �ν��� */
static void code_M(void)
{
	int old, first = 0;
	int i;
	int code;

	if(mod_on == -1) first = 1, mod_on = def_mod_on; /* Add Nide */
	old = mod_on;	
	code = getbyte(1);
	if(code == 'O'){
		(void)getbyte(2);
		code = getbyte(0);
		if(code == 'N'){
			mod_on = xget(&i);
			/* mod_on &= ~0x80; ������? */
			if(i != 0) mml_err(26);
		} else if(code == 'F'){
			mod_on = 0;
		} else mml_err(27);

		first ? (x_mod_on = 0) : (x_mod_on += mod_on - old);
		mod_delay = -1;

		if(check_mod() == 0) put_mod(mod_on);
	}else{
		mod_on = 0;
		mod_delay = xget(&i);
		if(i != 0) mml_err(28);
		if(getbyte(0) != ',') mml_err(28); /* y?�θ�˥���ޤ��ʤ� */
		mod_after = xget(&i);
		/* x_mod_on = 0; ����ʤ��Ƥ褤 */
		if(i != 0) mml_err(9); /* �ͤ��Ѥʾ�� */
	}
}

/* ��Ĺ�μ��� */
int length(void)
{
	int i, jq = 0, num = 0, futen_pow2;

	i = getbyte(1);
	if(i == '%'){
		jq = 1;
		(void)getbyte(2);
		i = getbyte(1);
	}
	if(!is_digit(i)){
		num = clength; /* l���ͤ�Ȥ� */
		jq = 1;
	} else {
		do {
			num = num * 10 + dtoi(i);
			(void)getbyte(2);
			i = getbyte(1);
		} while(is_digit(i));
	}
	for(futen_pow2 = 1; i == '.'; futen_pow2 <<= 1){
		(void)getbyte(2);
		i = getbyte(1);
	}

	if(jq){
		num = (int)((long)num * (2*futen_pow2-1) / futen_pow2);
	} else {
		if(num) num = (int)((long)timebase * 4 *
				(2*futen_pow2-1) / futen_pow2 / num);
	}

	if(i == '^'){
		(void)getbyte(2);
		num += length();
	}else if(i == '-'){
		(void)getbyte(2);
		num -= length();
	}
	if(num < 0) mml_err(2);
	return num;
}

/*
  ���ͤμ�����

  ����դ��ο��ͤ�����ͤ��֤���
  ���ϡ�*i �ˤ�����롣�Ĥޤꡢ��椬�դ��Ƥ�����ˤϡ�
  *i = -1 �ޤ��� *i = 1 �Ȥʤ롣�����л�����Ф�����θ�Ǥ���
  ���ͤ���ά����Ƥ���̵����硢�Ĥޤꡢ������ͤ�ʤ�
  ��硢*i = -2 �Ȥʤ롣�ʤ��λ�������ͤ�0��
  ����ʳ��Ǥ� *i = 0 �Ȥʤ롣
  �����������ʤ���硢�Ĥޤꡢ-�����+�ϡ�-1�����+1�Ȥߤʤ���
  x??��16�ʿ��λ��꤬�Ǥ��롥��������x�θ��ɬ��2�Х��Ȥǻ��ꤹ�롣
  x�θ��3�Х����ܰʹߤ�̵�뤹�롣����դ���16�ʿ������Ǥ��롣-x2f�ʤ�
*/
int xget(int *i)
{
	int index;
	int num = 0;
	int j;
	int code = getbyte(1);

	*i = 0;
	if(code == '-'){
		*i = -1;
		(void)getbyte(2);
		code = getbyte(1);
	}else if(code == '+'){
		*i = 1;
		(void)getbyte(2);
		code = getbyte(1);
	}

	switch(code){
	case '&': /* nʬ�������ξ�� */
		(void)getbyte(2);
		num = length();
		break;
	case 'z': /* �ѿ��ξ�� */
		(void)getbyte(2);

		index = 0;
		for(;;){
			code = getbyte(1);
			if(is_digit(code)){
				index = index * 10 + dtoi(code);
				(void)getbyte(2);
			}else break;
		}
		if(index < 0 || 255 < index) mml_err(51);
		num = var[index];
		break;
	case 'C': /* �����ͥ��ֹ�ξ�� */
		num = cur_ch + 1;
		(void)getbyte(2);
		break;
	case 'R': /* �������������ͥ��ֹ�ξ�� */
		if(cur_ch == 9) num = 0;
		else if(cur_ch < 9) num = cur_ch + 1;
		else num = cur_ch;
		(void)getbyte(2);
		break;
	case 'x': /*��x�פǻϤޤ�16�ʿ� */
		goto get_hex;
	case '0':
		(void)getbyte(2);
		code = getbyte(1);
		if(code == 'x'){ /*��0x�פǻϤޤ�16�� */
		get_hex:
			(void)getbyte(2);
			if((code = getbyte(1)) == '{'){
				(void)getbyte(2);
				while((code = getbyte(0)) != '}'){
					if(!is_xdigit(code)) mml_err(12);
					num = num * 16 + xtoi(code);
				}
			} else {
				for(j = 0; j < 2; j++){
					code = getbyte(0);
					if(!is_xdigit(code)) mml_err(12);
					num = num * 16 + xtoi(code);
				}
			}
			break;
		} else if(!is_digit(code)){ /* 0���� */
			break;
		} else { /* ���̤�10�ʿ��� */
			/* FALLTHROUGH */
		} /* not break! */
	case '1':
	case '2':
	case '3':
	case '4': /* �̾�ο��� */
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		for(;;){
			(void)getbyte(2);
			num = num * 10 + dtoi(code);
			code = getbyte(1);
			if(!is_digit(code)) break;
		}
		break;
	default:
		if(*i == 0){ /* ������ͤ�̵����� */
			*i = -2;
			return 0;
		}else{ /* �������ξ�� */
			return *i;
		}
	}
	if(*i != 0) num = *i * num;
	code = getbyte(1);
	if(code == '+'){
		(void)getbyte(2);
		num += xget(&j); /* �Ƶ��ƤӽФ� */
	}else if(code == '-'){
		(void)getbyte(2);
		num -= xget(&j); /* �Ƶ��ƤӽФ� */
	}
	return num;
}

void write_length(long i, fileptr fp) /* ����Ĺ -> ����Ĺ */
{
#define j 0x80L
	tstep += i;

	fgetpos2(fp, &lastlenpos);
	 /* ľ���ν�°�ȥ�å�����ȥ�å����Ĺ����� write_length()�����
	    Ĺ����񤭽Ф����Ȥ��뤳�Ȥ����� �����ʤ�������SMF�ˤʤ�ʤ��Τ�
	    �����Ǥ��������å�
	    â�� �����Ϥ��ξ�郎����Ω�Ƥ�ñ��˥��顼�ˤ������������Ϥ����ʤ�
	    ľ���ν�°�ȥ�å�����ȥ�å����Ĺ���Ƥ� ���ߤν�°�ȥ�å���
	    ��Ƭ����̵���ʤ� ���write_length()���񤤤����Ĺ�����񤭤���
	    SMF�Ȥ��Ƥ�����ˤʤ��礬���� �����⤽����Ѷ�Ū�����Ѥ���
	    sample�ե����뤬�Ǥ��Ƥ��ޤäƤ���Τ� ���ߴ��Τ���ˤϤ�������
	    �������ϵߺѤ��ʤ��ƤϤʤ�ʤ�
	    ������ ���Ĺ����񤤤�����last_oddlen.pos(����-1)�˵�Ͽ���Ƥ���
	    ���줬-1�ˤʤ�ʤ�������lastlenpos���ʤ�Ǥ��ޤä���ȥ�å���
	    �������褿�ꤷ�����˽��ƥ��顼�Ȥ��� */
	if(last_oddlen.pos >= 0 && last_oddlen.pos < lastlenpos){
		cur_line = last_oddlen.lineno;
		mml_err(70);
	}
	if(i < 0){ /* Ĺ������ */
		if(last_oddlen.pos < 0){
			 /* Ʊ��ս�����Ĺ������ƽ񤳤��Ȥ������ֹ�
			    ������Ͽ */
			last_oddlen.pos = lastlenpos;
			last_oddlen.lineno = cur_line;
		}
	} else { /* Ĺ�������� last_oddlen.pos�ε�Ͽ�򥯥ꥢ */
		last_oddlen.pos = -1;
	}

	lastlen = i;

	if(i < j){
		putc2((char)i, fp);
	}else if(i / j < j){
		putc2((char)(i / j + j), fp);
		putc2((char)(i % j), fp);
	}else if(i / j / j < j){
		putc2((char)(i / j / j + j), fp);
		putc2((char)((i % (j * j)) / j + j), fp);
		putc2((char)((i % (j * j)) % j), fp);
	}else{
		putc2((char)(i / (j * j * j) + j), fp);
		putc2((char)((i % (j * j * j)) / (j * j) + j), fp);
		putc2((char)(((i % (j * j * j)) % (j * j)) / j + j), fp);
		putc2((char)(((i % (j * j * j)) % (j * j)) % j), fp);
	}
#undef j
}

static char *err_msgs[] = {
	NULL,	/* 0 */

	"loop end ']' is missing",
	"length parameter is negative",
	"loop start '[' is missing",
	"loop out ':' is wrong",
	"macro name '$?' is wrong",

	"macro call '$?' is wrong",
	"octave 'o?' is wrong",
	"beat 'BT?,?' is wrong",
	"control change 'y?,?' is wrong",
	"tempo 't?' is wrong",		/* 10 */

	"bend range 'BR?' is wrong",
	"parameter 'x??\047 or '0x??\047 is wrong",
	  /* to avoid being treated ??' as a trigraph, we use \047 */
	"no such a command 'B?'",
	"velocity 'k?' is wrong",
	"volume 'v?' is wrong",

	"expression 'E?' is wrong",
	"bend set 'BS?' is wrong",
	"relative bend set 'BB?' is wrong",
	"exclusive 'EX?' is wrong",
	"relative transfer '__?' or '*?' is wrong",	/* 20 */

	"transfer '_?' s wrong",
	"bank select 'H?,?' is wrong",
	"program change '@?' is wrong",
	"no such a command 'R?'",
	"illegal (or too long) arg of command 'F?'",

	"modulation 'MON?' is wrong",
	"no such a command 'MO?'",
	"modulation 'M?' is wrong",
	"command 'F?' is wrong",
	"command 'I?' is wrong",	/* 30 */

	"detune 'D?' is wrong",
	"NRPN 'N?' is wrong",
	"Too many parameters in NRPN 'Nn1,n2,n3'",
	"command 'BW?' is wrong",
	"command '=?' is wrong",

	"Master Part not found",
	"key off velocity 'w?' is wrong",
	"command 'K?' is wrong",
	"poli press 'A?' is wrong",
	"channel press 'G?' is wrong",		/* 40 */

	"command 'Q?' is wrong",
	"command 'J?' is wrong",
	"loop nest too deep",
	"macro call '$?' is wrong (in macro)",
	"undefined macro call '$?' (in macro)",

	"loop nest too deep (in macro)",
	"loop start '[' is missing (in macro)",
	"loop end ']' is missing (in macro)",
	"illegal string concatenation",
	"command 'Z{....}' is wrong",		/* 50 */

	"variable z? is wrong",
	"command 'U?' is wrong",
	"illegal (or too long) arg of command 'U?'",
	"command 'TC?' is wrong",
	"command 'TR?' is wrong",

	"track name is too long",
	"text too long",
	"command W?\"....\" is wrong",
	"Illegal arg of command 'I?'",
	"too many tracks",		/* 60 */

	"memory insufficient",
	"string end '\"' is missing (in macro)",
	"recursive macro call",
	"cannot interpret '", /* + Msg + "'" */
	"no such macro '", /* + Msg + "'" */

	"':' not within loop",
	"track name illegal",
	"command 'T?' is wrong",
	"command 'KJ?' or 'KI?' is wrong",
	"can't align sub-track (previous sub-track was longer than master track)", /* 70 */
};

static char *warn_msgs[] = {
	NULL,
	"cannot interpret '", /* + Msg + "'; ignored" */
	"loop out '/' is obsolete (please use ':' instead)",
	"using '^' instead of 'r' is obsolete (use 'r')"
};

void mml_err(int i)
 /* i����ξ��ϥ��顼�Ǥʤ��ٹ�
    i���ͤˤ�äƤϸƤӽФ�����Msg�����Ƥ���å����������� */
{
	int err_line;
	char *err_file;

	if(i > 0){
		strcat(text, "ERROR!  ");
		strcat(text, err_msgs[i]);
	} else {
		strcat(text, "Warning: ");
		strcat(text, warn_msgs[-i]);
	}
	switch(i){
	case 64:
	case 65:
		strcat(text, Msg);
		strcat(text, "'");
		break;
	case -1:
		strcat(text, Msg);
		strcat(text, "'; ignored");
		break;
	}

	err_line = cur_line, err_file = NULL;
	if(ppinfo . fname != NULL){
		err_line += ppinfo.line - ppinfo.actual_line - 1;
		if(*ppinfo . fname) err_file = ppinfo . fname;
	}
	if(err_file == NULL){
		wsprintf(Msg, " in line %d.\n", err_line);
	} else {
		wsprintf(Msg, " in line %d. <%s>\n", err_line, err_file);
	}
	strcat(text, Msg);
	if(i < 0){
		InvalidateRect(hWnd3, NULL, TRUE);
		UpdateWindow(hWnd3);
	} else {
		remove_file_and_owari();
	}
}

void write_header(void)
{
	wsprintf(Msg, "trk %d%c: ", tnum, talf + 'A');
	strcat(text,Msg);
	if(++trknum >= MAXTRKNUM) mml_err(60);
	smftrkheader(fp2, &trksize);
}

static void get_trackname(void)
{
	switch(mml_getstring(trackname, sizeof(trackname))){
	case -1:
		return;
	case sizeof(trackname):
		mml_err(56);
	}
	write_trackname();
}

static void write_trackname(void)
{
	int i;

	putc2(0xff, fp2);
	putc2(3, fp2);
	putc2(strlen(trackname), fp2);
	for(i = 0; i < strlen(trackname); i++){
		putc2(trackname[i], fp2);
	}
	write_length(0, fp2);
}

static void texts(void)
{

	switch(getbyte(0)){
	case 'T': /* ff 01 �ƥ����� */
		put_midistring(1);
		break;
	case 'I': /* ff 04 �ڴ�̾ */
		put_midistring(4);
		break;
	case 'L': /* ff 05 �λ� */
		put_midistring(5);
		break;
	case 'M': /* ff 06 Marker */
		put_midistring(6);
		break;
	case 'C': /* ff 07 Cue point */
		put_midistring(7);
		break;
	default:
		mml_err(58);
	}
}

static void put_midistring(int x)
{
	int i;
	char t[MAX_TEXT_STR + 1];

	switch(mml_getstring(t, sizeof(t))){
	case -1:
		mml_err(58);
	case sizeof(t):
		mml_err(57);
	}

	putc2(0xff, fp2);
	putc2(x, fp2);
	putc2(strlen(t), fp2);
	for(i = 0; i < strlen(t); i++) putc2(t[i], fp2);
	write_length(0, fp2);
}

static int mml_getstring(char *buf, int length)
 /* ����� -1:�ǽ餬��"�פǤʤ��ä� 0�ʾ�:�����������ʸ�����Ĺ��
    ʸ�������ˤ�'\0'��񤯡�����ͤϤ���'\0'��ޤޤʤ�Ĺ����ʸ����Ĺ�᤮���
    length�Х��Ȥ���������(�Ǹ��'\0'�ޡ�����񤫤���)length���֤� */
{
	int i, j, c, d;

	d = getbyte(1);
	if(d != '"') return -1;
	(void)getbyte(2);
	for(i = 0; ; i++){
		if(i == length) return i;
		switch(d = Getbyte(0, 1)){
		case '"':
			if((d = getbyte(1)) == '+'){ /* ʸ����Ϣ�� */
				(void)getbyte(2);
				if((d = getbyte(0)) != '"') mml_err(49);
				i--; continue;
			}
			 /* FALLTHROUGH */
		case -1:
			buf[i] = 0;
			return i;
		case '\\':
			d = Getbyte(0, 1);
			if(d == 'x'){
				d = 0;
				for(j = 0; j < 2; j++){
					c = Getbyte(1, 1);
					if(!is_xdigit(c)) break;
					(void)getbyte(2);
					d <<= 4, d |= xtoi(c);
				}
			} else
			if(is_octal(d)){
				d = dtoi(d);
				for(j = 0; j < 2; j++){
					c = Getbyte(1, 1);
					if(!is_octal(c)) break;
					(void)getbyte(2);
					d <<= 3, d |= dtoi(c);
				}
			} else {
				d = escchr(d);
			}
			buf[i] = d;
			break;
		default:
			buf[i] = d;
			if(mskanji && ismskanji1(d)){
				if(++i == length) return i;
				buf[i] = Getbyte(0, 1);
			}
		}
	}
}

int check_proc(int kind) /* kind: 0��MAX_KIND-1 */
{
	int i;

	for(i = 0; i < mmlproc_ptr; i++){
		if(mmlproc[i].kind == kind) return 1;
	}
	return 0;
}
