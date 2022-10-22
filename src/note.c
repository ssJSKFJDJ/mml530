
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
	int oncho; /* ��Ĺ */
	int oto; /* r(����):1 \:3 K:4 A:8 �ɤ�Ǥ�ʤ����0 */
#define OTO_KYUFU 001
#define OTO_BS_CMD 002 /* \�ξ��OTO_KYUFU�ӥåȤ�Ω�� */
#define OTO_K_CMD 004
#define OTO_A_CMD 010
	int velocity;	/* local��velocity */
	int gatetime;	/* local��gatetime */
	int koff;	/* local�ʥ������ե٥��ƥ� */
};

/* mml2mid.c ����� extern */
extern fileptr fp2;		/* ���ϥե�����(mid) */
extern void write_length(long, fileptr);
extern Fpos_t lastlenpos;	/* �Ǹ�˲�Ĺ��񤭹�������ؤΥݥ��� */
extern long lastlen;		/* �Ǹ�˽񤭹������Ĺ���� */
extern int length(void);
extern int xget(int *);
extern int trans;
extern int german_scale, backcompati;

/* mmlproc.c ����� extern */
extern long tstep;	/* �ȥ�å��θ��ߤΥ��ƥåץ����� */
extern int cur_ch;	/* ���ߤΥ����ͥ� */
extern int run;		/* ���˥󥰥��ơ����� */
extern int octave;	/* ���������� */
extern int under;	/* žĴ */
extern int transcale;	/* Ĵ�� */
extern int velocity, rvel1, rvel2; /* �٥��ƥ��� */
extern int randvel1, randvel2; /* ������٥��ƥ��� */
extern int ktencho;	/* J���ޥ�ɤ��� */
extern int gatetimeQ;	/* Q���ޥ�ɤ��� */
extern int gatetime;	/* q���ޥ�ɤ��� */
extern int gatetime2;
extern int ctl_values[]; /* expression, panpot, �Ĥ������ޤȤ᤿���� */
extern int rpan1, rpan2;
extern int mod_delay, mod_after;
extern struct keyproc2 *keyproc2;
extern int keyproc2_ptr /* �Ŀ� */, keyproc2_amount;
extern int koff;	/* �������ա��٥��ƥ� */
extern int codef[KIND_MAX][MAX_F_ARG+1];
extern int codeu[KIND_MAX][MAX_U_ARG+1];
extern int codei[KIND_MAX][5];
extern int check_cntchange(int, int);
extern int check_bendchange(int);
extern void mml_err(int);
extern int psw;

/* 98.3.6 �ɲ� */
/* U������� last=first=0
   F,I���ϻ���U,F,I��̵���ʤ�last=first=0
   F,I���ϻ���U,F,I������ʤ�x_* -= last-first,first=z
   F,I���last����
*/
long first_F[KIND_MAX];
long last_F[KIND_MAX];
long first_I[KIND_MAX];
long last_I[KIND_MAX];

/*
	98/3/4�ɲá�
	F,I��Ʊ�����Ѥ��ǽ�ˤ��롣
	F,I���ޥ�ɷϤ��ͤ�mmlproc�������ͤǽ񤯤���ˡ��١����Ȥʤ�
	�ͤ�x_*����¸���롣
*/
int x_values[KIND_MAX]; /* x_expression, x_panpot�Ĥ������ޤȤ᤿���� */

int andflag;	/* &�����ä����ɤ����Υե饰 */
/* �������ջ��˽������٤��������� */
struct keyproc *keyproc;
int keyproc_ptr /* �Ŀ� */, keyproc_amount;

struct mmlproc *mmlproc;
int mmlproc_ptr /* �Ŀ� */, mmlproc_amount;

void note(int); /* ���䡢�������*/
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
}; /* mmlproc�˵�Ͽ�����̤�������٥�Ȥؤ���1�ѥ�᡼��
	���expressiom, panpot, �Ĥ��Ф����Ρ�-1�ϻȤ��Ƥ��ʤ���� */

/* ���л���ϲ�ǽ�������ͤξ�ά���Բ�ǽ���ͤ�������뤿��δؿ� */
/* �ͤ���ά����Ƥ������ϡ�-1 ���֤� */
int soutai0(int base)
{
	int rt;
	int i;
	int num = xget(&i);

	if(i == 0){
		rt = num;
	}else if(i == -1 || i == 1){ /* ���л���ξ�� */
		rt = base + num;
	}else{
		return -1; /*�ͤ���ά���줿��� */
	}
	ALIGN(rt); /* ���л���ʳ��Ǥ⶯��Ū��0��127���ϰϤˤʤ�褦�ˤ��� */
	return rt;
}

/* ���л��꤬��ǽ�ǡ����ġ��ͤξ�ά���ǽ���ͤ�������뤿��δؿ� */
static int soutai(int base)
{
	int rt = soutai0(base);

	if(rt == -1) rt = base;
	return rt;
}

/* ��������ʤޤ���velocity0�ǥ������աˤ�������δؿ� */
static void putnote(int p1, int p2)
{
	if(run != 9){
		putc2(0x90 + cur_ch, fp2);
		run = 9;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

/* 80h�ǥ������դ�������δؿ� */
static void putnoteoff(int p1, int p2)
{
	if(run != 8){
		putc2(0x80 + cur_ch, fp2);
		run = 8;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

/* a0h�ǥݥ�ץ�å��㡼��Ϳ��������δؿ� */
static void ppres(int p1, int p2)
{
	if(run != 0xa){
		putc2(0xa0 + cur_ch, fp2);
		run = 0xa;
	}
	putc2(p1, fp2);
	putc2(p2, fp2);
}

 /* mmlproc[i].st���Ǿ��Ȥʤ�i���֤� */
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

  �����ζ��̡�

  ȯ�����ˤ��ʤ��Ȥ����ʤ�������
    [��] F���ޥ�ɤǻ��ꤵ���ǽ��expression���͡�
  ȯ����ˤ��ʤ��Ȥ����ʤ�������
    [��] ��ľ����ȯ�����줿���Υ����ȥ����ब�ޥ��ʥ����ä�����
           �ò����ʤ��Ȥ����ʤ���
	 ��M���ޥ��
	 ��F���ޥ��
    ���ƥåץ������ͤ���ɬ�פ����롥
  ȯ����ˤ��ʤ��Ȥ����ʤ�����
    [��] ��M���ޥ�ɤ�ȤäƤ���硤modulation���ͤν������
         ��F���ޥ�ɤ�ȤäƤ���硤expression���ͤν������
         �Ȼפä����ɡ�ľ��ϥޥ���������

  �ͤ���٤�������

�����ȥ����ब�ޥ��ʥ�������ӡ����ν����η�͹礤��
�㡧q-4 e4&a2c4
    c���������󤷤Ƥ��顢e��a���������դ���ʤ��Ȥ����ʤ���

�����ȥ����ब�ޥ��ʥ��ǡ����ġ�&�����ˤ����硣
  �����ȥ����ब�ޥ��ʥ��������ǥ������հ��֤򵭲����롣
ľ����&��̵�����䤬�褿�Ȥ��ˤʤäƽ��ơ����ޤ�
�����ȥ�����ޥ��ʥ��ǵ����������ν����򤹤롣

���ȥ��٥�Ȥη�͹礤��
�㡧M48,127 c2&e4
MON127��1�٤����񤫤�ʤ���
�����⡢�������ո�ˡ�MON0 ��ɬ�ס�

����ȡ����������ȥ�����Ȥη�͹礤��
�㤨�С�
a2.^4. �� a2.&r4. ������Ʊ���ˤ��롣�Ĥޤꡢ�����ȥ���������ϡ�
&r4.�ΤȤ���Ǥ�Ԥ��롣
���Τ�����Τ��Ȥ�ͤ���ȡ�����ȵ���ν����ϻ��Ƥ롣
�㣲��
q-4 c2r4 ���ȡ��ɤ��������뤫��

�����ȥ����ब�ޥ��ʥ��λ���
ľ���β��������ȥ�����ޥ��ʥ���ȯ������Ƥ��ơ�����ȯ�����������β�Ĺ��
��Τ�����û����硣


qn0,n1
    n0 == gatetime  �ޥ��ʥ��Ǥ��ɤ���
    n1 == gatetime2
���ޤǤ�q�Υޥ��ʥ����ޥ�ɤ�̵���ʤä���
����ˡ�q100,4 �Τ褦�ˤ�����ɤ����Ĥޤꡢ���ѥ�᡼�����ͤ��礭�����롣

���ξ���ϸŤ���
��
�����䡢���� (a-g,r)n0,n1,n2,n3[&]
       n0 == note length
       n1 == local velocity
       n2 == local gatetime
       n3 == local keyoff velocity (use 80h event)
  - preandflag = andflag
  - andflag ����
  - localgatetime ��̵����С� localgatetime = gatetime;
  - keyoff velocity ��̵����С� localkoff = koff;
  - localgatetime < 0 �ʤ�С�gtflag = 1; else gtflag = 0;
  - ���β����򥭡����󤹤롣����ʤ�Ф��ʤ���
  - �ɤΰ��֤ޤǥ��٥�Ȥ�������뤫��Ƥ���롣
    andflag == 0 && gtflag == 0�ʤ�С�
      ��Ĺ - localgatetime < gatetime2 �ʤ�С�localgatetime = ��Ĺ
    else
      localgatetime = ��Ĺ
  - mmlproc�ν���
    * �㥤�٥�Ȥν�����
      ���٥�Ȥ�mmlproc�˽��֤˽񤤤Ƥ�����
      ���λ��ˡ�preandflag != 0 �ʤ�С��Ĥޤꡢľ����&�����ä����ϡ�
      ���椫�饤�٥�Ȥ�������롣����ϡ����٥�Ȥ�񤯼�³���ˤ�����
      ���ޤ���θ���롣�㤨�С�M���ޥ�ɤǤϡ����椫��ν����Ǥϥ⥸��졼�����
      ��񤭹��ޤʤ���
      �ɤ��ޤǥ��٥�Ȥ�񤯤��ϡ�localgatetime �ޤǽ񤯡�
      ɬ�פ˱����ơ�keyproc�ˤ�񤯡�
    * ���٥�Ȥν������Ԥ��ʤ����⤢�롣����ϡ�
      ����ǡ������⡢preandflag == 0 �ξ�硣
    * ��ޥ��ʥ������ȥ�����ν�����
      preandflag == 0 �ʤ�С��Ĥޤꡢľ����&��̵�����ϡ�
      keyproc2 ���� mmlproc �˽񤯡�
      ��������keyproc2_st > localgatetime �ξ��ϡ�
      keyproc2_st = localgatetime�Ȥ��롣
      �Ǹ�ˡ�keyproc2_ptr = 0 �ˤ��롣
    * ����ξ��⡢�ޥ��ʥ������ȥ����������Ԥ���
  - mmlproc��ºݤ˽񤭹�������򤹤롣
    ���Ū�ˡ�mmlproc_ptr = 0 �Ȥʤ롣
  - keyoff�����򤹤롣
    �ʲ��� || ���� && keyproc_ptr != 0 �ξ�������
    localkoff��ͤ��ƥ������դ��롣
    * gtflag == 1 �ξ��ϡ��������դ��ʤ���
      ���Ǹ�Υ��ƥåץ����� = 0 ��񤯡�
      ��keyproc2����¸���롣�ʲ���ξ�������
    * else andflag == 1 �ξ��ϡ��������դ��ʤ���
      ���Ǹ�Υ��ƥåץ����� = 0 ��񤯡�
      ��keyproc����¸���롣�ʲ���ξ�������
    * else
      ��keyproc�˴ޤޤ�벻�򥭡����դ��롣
      ����ʬ���Ȥ򥭡����դ��롣�ʲ���ξ�������
      ���Ǹ�Υ��ƥåץ����������ȹͤ��ƽ񤯡�
*/

 /* note()�β����� *vars��6�Ĥ��ͤ���������
    LSI-C�ǥ���ѥ����ǽ�ˤ��뤿���note()����ʬΥ����(�������ʤ���LSI-C�Ǥ�
    ������­�ǥ���ѥ���Ǥ��ʤ�) */
static void get_note_values(int code, struct local_note_vars *vars)
{
	int i, gatetimetmp;

	vars->oto = 0;
#ifdef __GNUC__
	vars->onkai = vars->oncho = 0; /* �ٹ���ޤ�����Ū */
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
	} /* ������vars->oto��0, OTO_KYUFU, OTO_KYUFU | OTO_BS_CMD, OTO_K_CMD,
	     OTO_A_CMD�Τ����줫 vars->onkai�������ͤΤޤޤ��������Τ�
	     vars->oto��OTO_KYUFU��OTO_KYUFU | OTO_BS_CMD�ξ����� */

	if(vars->oto == 0 && psw == 0){
		if(rpan1 != 0) set_rpan(); /* running panpot �ν��� */
		if(rvel1 != 0) set_rvel(); /* running velocity �ν��� */
	}

	if(!(vars->oto & OTO_KYUFU)){
		 /* ���ΤȤ�code��'a'��'h'�Τ����줫 */
		vars->onkai += 12 + get_hanon(code) + octave * 12 + under;
		if(gatetime2 != -1) vars->onkai += trans;
		if(ktencho != 0) vars->onkai = ktencho; /* ����žĴ���� */
		ALIGN(vars->onkai); /* �ϰϳ������å� */
	}
	if(!(vars->oto & (OTO_K_CMD | OTO_A_CMD)))
		vars->oncho = length();

   	gatetimetmp = gatetime + vars->oncho * (8 - gatetimeQ) / 8;

	 /* �����ޤǤ�vars->{onkai,oncho,oto}����ޤ�(â��vars->oto��
	    OTO_KYUFU��OTO_KYUFU | OTO_BS_CMD�ξ���vars->onkai������
	    OTO_K_CMD��OTO_A_CMD�ξ���vars->oncho������)
	    ����vars->{velocity,gatetime,koff}�η��� */

	if(getbyte(1) == ','){
		(void)getbyte(2);
		vars->velocity = soutai(velocity);
		if(getbyte(1) == ','){
			int num;

			(void)getbyte(2);
			num = xget(&i);
			if(i != -2) vars->gatetime = num;
			else{
				vars->gatetime = gatetimetmp; /* �ͤξ�ά */
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

/* ���䡢���䡤\�ν��� */
void note(int code)
{
	int i;
	long j;
	int gt;
	long p = 0;
	int preandflag, minst_ptr;
	struct local_note_vars local;

	/* mmlproc_ptr = 0; */ /* ������ 0 �ˤ���Ȥ�Ф� */

	get_note_values(code, &local);
	 /* local��6�Ĥ��ͤ�����Ƶ��äƤ��� â��local.oto��
	    OTO_KYUFU��OTO_KYUFU | OTO_BS_CMD�ξ���local.onkai������
    	    OTO_K_CMD��OTO_A_CMD�ξ���local.oncho������ */

	if((local.oto & OTO_KYUFU) && local.oncho == 0)
		return; /* ��Ĺ����ε����̵�� */

	if(local.oto & OTO_K_CMD){ /* K���ޥ�ɤξ�� */
		if(psw != 0) return;
		putnote(local.onkai, local.velocity);
		write_length(0, fp2);
		return;
	}
	if(local.oto & OTO_A_CMD){ /* A���ޥ�ɤξ�� */
		if(psw != 0) return;
		ppres(local.onkai, local.velocity);
		write_length(0, fp2);
		return;
	} /* ����������local.oncho������ǤϤʤ� */

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

	/* gatetime2 == -1 �Ȥʤ�Τ� RT �λ� */
	if((local.oto & OTO_KYUFU) || gatetime2 == -1) local.gatetime = 0;
	gt = local.gatetime;

	/* local.gatetime �ϡ��ɤ�steptime�ޤǥ��٥�Ȥ�������뤫 */
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

	/* mmlproc �ν��������������롥*/
	/* mmlproc �ν����Ȥ����Τϡ�����������ˤ��ʤ��Ȥ����ʤ������λ� */
	/* ���ƥå�0��local.gatetime �ޤǽ������� */
	/* ȯ��������Ԥ��ƽ������ʤ��Ȥ����ʤ����⤢��Τ��� */
	/* ȯ�����˽�������Τϡ��������󡿥��հʳ��ǥ��ƥå�0�Υ��٥�� */
	if(local.oto == 0 && preandflag == 0 && mod_delay != -1){
		x_mod_on = 0;
		set_mmlproc(6, 0, 0); /* ����Ƥ���ɬ�פ��� (6:modulation) */
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

	/* ȯ�����˽������ʤ��Ȥ����ʤ����٥�Ȥ�񤭹��� */
	/* ����Ū�ˤϡ����ƥåץ�����0�Υ������󡿥��հʳ��Υ��٥�Ȥ��٤� */
	write_step0_change();

	/* �褦�䤯ȯ������ */
	if(!(local.oto & OTO_KYUFU)){
		putnote(local.onkai, local.velocity);
		p = 0; /* ����ȵ����Ʊ���˰�������� p ��Ƴ�������������κ� */
	}else{ /* ����ξ�� */
		tstep -= lastlen;
		fsetpos2(fp2, &lastlenpos); /* �虜�虜�᤹�������κ� */
		p = lastlen; /* p�Ȥ����Τϡ��ǽ��1����������Ƥ��륹�ƥåץ����� */
	}
	if(local.oncho == 0){ /* ��Ĺ����β���ξ�� */
	 /* ���ξ��(local.oto & OTO_KYUFU) != 0�ʤΤǡ�local.onkai��
	    ����ǤϤʤ� */
		andflag = 1; /* ����&������Τ�Ʊ�������ˤ��� */
		write_length(0, fp2);
		set_keyproc(local.onkai, local.koff); /* ��ǥ������դ�ɬ�פȤ������� */
		return;
	}

	/* �ʲ��ϡ�mmlproc ��񤭹������ */
	j = local.gatetime; /* j�ϡ��������դޤǤΥ��ƥåץ����� */
	while(mmlproc_ptr > 0){
		minst_ptr = find_minst();
		if(mmlproc[minst_ptr].st > j){
		 /* Mod Nide; U���ޥ�ɤΥХ��к��������Ǵ���mmlproc[i].st����
		    j�������ᡢ���mmlproc[i].st����������Ͻ�����j���餹 */
			for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= j;
			break; /* ������break�Ϥ��ΤޤޤǤ���������? */
		}
		write_mmlproc(minst_ptr, &p, &j);
	}

	/* �������ս��� */
	if(!(local.oto & OTO_KYUFU) || keyproc_ptr != 0){
		long xx;

		if(gt < 0){ /* �����ȥ����ब�� */
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
			/* p = 0; ������ʤ��Ƥ��ɤ� */
			if(!(local.oto & OTO_KYUFU)){ /* ����λ� */
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

			/* �Ρ��ȥ�����ˤ�mmlproc�ν����򤹤� 98/3/4 */
			xx = lastlen + local.oncho - local.gatetime;
			while(mmlproc_ptr > 0){
				minst_ptr = find_minst();
				if(mmlproc[minst_ptr].st > xx){
				 /* Mod Nide; U���ޥ�ɤΥХ��к� */
					for(i = 0; i < mmlproc_ptr; i++)
						mmlproc[i].st -= xx;
					break; /* ����break�Ϥ��Τޤޤ�OK��? */
				}
				write_mmlproc(minst_ptr, NULL, &xx);
			}
			write_length(xx, fp2); /* ����Ϥ��䤷������ǽ�������롣 */
		}
	}else{ /* ñ�ʤ���� */
		long xx;

		xx = p + j + local.oncho - local.gatetime;
		while(mmlproc_ptr > 0){
			minst_ptr = find_minst();
			if(mmlproc[minst_ptr].st > 0) break;
			 /* Mod Nide; U���ޥ�ɤΥХ��к���xx�Ǥʤ�0����� */
			write_mmlproc(minst_ptr, NULL, &xx);
		}
		write_length(xx, fp2); /* ����Ϥ��䤷������ǽ�������롣 */
		/* p = 0; ������ʤ��Ƥ��ɤ� */
	}

	/* ���פʤϤ��ʤΤ�����98.3.4
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

 /* expression, panpot, �ĤΥǥե������ */
int def_values[] = {
	127, 64, 100, 64*128, 64, 64, 0, 0, 0
};
 /* code{f,u,i}[x][i]��expression, panpot, �Ĥβ��ܤ�񤯤� */
int each_factor[] = {
	128, 128, 128, 1, 128, 128, 128, 128, 16
};

/* U���ޥ�ɤ�񤤤����ˣ�������ƤФ�롣 */
void setcode_U(int localgatetime, int x)
{
	long real_value;
	int num; /* ���󤸤�ʤ��Ƥ褤 */
	int step; /*���󤸤�ʤ��Ƥ褤 */
	int step3;
	int z, factor;

	first_F[x] = last_F[x] = first_I[x] = last_I[x] = 0;

	factor = each_factor[x];
	if(ctl_values[x] == -1) ctl_values[x] = def_values[x];
	z = ctl_values[x] * factor;
	x_values[x] = ctl_values[x];

	num = z + codeu[x][1];
	if(x != MMLPROC_DTEMPO) ALIGN2(num); /* �ƥ�ݰʳ����ϰ���� */
	step = 0;
	set_mmlproc(x, num/factor - z/factor, 0);
	real_value = num/factor; /* �ºݤ˺��Ѥ����� */

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
		real_value = num/factor; /* �ºݤ˺��Ѥ����� */
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
		if(x != MMLPROC_DTEMPO) ALIGN2(num[x]); /* �ƥ�ݰʳ����ϰ���� */
		step[x] = 0;
		set_mmlproc(x, num[x]/factor - z/factor, 0);
		real_value[x] = num[x]/factor; /* �ºݤ˺��Ѥ����� */
	} else {
		/* real_value[x] = ?? */
	}
	step3 = step[x];

	for(;;){
		int k;
		long k11, k12, k13;

		i = num[x];
		/* ���Τ�����ϡ����٤������ͤǷ׻�����褦���ѹ�����
		   ɬ�פ����롣98/3/4 */
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
				if(k11 <= step[x] && step[x] <= k12){ /*���if��<=���㤦*/
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

		/* ��Ǥ������񤯤Τǡ��������ϰϤϤ߽Ф������å��򤹤롣*/
		if(x != MMLPROC_DTEMPO) ALIGN2(num[x]);
		if(num[x] / factor == i / factor) continue;
		set_mmlproc(x, num[x]/factor - real_value[x], step[x] - step3);
		real_value[x] = num[x]/factor; /* �ºݤ˺��Ѥ����� */
	}
}

/* I ���ޥ�ɤ�ȯ�����뤿���mmlproc�˽񤭹��� */
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
		real_value[x] = num[x]/factor; /* �ºݤ˺��Ѥ����� */
	} else {
		/* real_value[x] = ?? */
	}
	step3 = step[x];

	for(;;){
		long i = num[x];

		/* codei[x][0��4]: ���� ���� �ǥ��쥤 ��Ĺ ���翶���ޤ� */
		if(step[x] >= codei[x][2]){ /* completely rewritten. Nide */
			long num_diff; /* long�Ƿ׻�����ɬ�פ��� */
			int dt = step[x] - codei[x][2] + codei[x][1];
			int max_timing = codei[x][4] + codei[x][1];
			 /* step[x]��0�ˤʤ륿���ߥ󥰤��ǽ餫��codei[x][1]����
			    �٤�Ƥ���餷������Ψ��Ϥ���ˤ�dt��max_timing��
			    ���줾���codei[x][1]��­���Ƥ���ɬ�פ����� */

			num_diff = (long)codei[x][0]*4 * (dt%codei[x][3])
					/ codei[x][3];
			if(labs(num_diff) > (long)abs(codei[x][0])*3)
				num_diff -= (long)codei[x][0]*4;
			else if(labs(num_diff) > (long)abs(codei[x][0]))
				num_diff = (long)codei[x][0]*2 - num_diff;
			/* num_diff: �Υ������Ȥδ���ͤȤκ� */

			if(dt < max_timing) num_diff = num_diff * dt/max_timing;
			 /* ���翶���ˤʤ�ޤǤϿ��������̤�� */

			num[x] = (int)((long)z + num_diff);
			if(x != MMLPROC_DTEMPO) ALIGN2(num[x]); /* �ƥ�ݰʳ����ϰ���� */
		}

		step[x] += codei[x][1];
		if(step[x] - step3 >= localgatetime){
			step[x] -= codei[x][1];
			last_I[x] = z;
			break;
		}
		if(num[x]/factor == i/factor) continue;	/* �񤭹����ͤ��Ѥ�äƤʤ����*/
		set_mmlproc(x, num[x]/factor - real_value[x], step[x] - step3);
		real_value[x] = num[x]/factor; /* �ºݤ˺��Ѥ����� */
	}
}

static void set_rpan(void)
{
	if(panpot == -1) panpot = 64; /* Add Nide; ɬ�פ�?? */
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

	if(!seed_set){ /* ���٤���seed�򥻥å� */
		long t;

		t = (long)time(NULL);
#ifdef UNIX
		t ^= ((long)getpid() * 0x8001);
#endif
		srand((int)t);
		seed_set = 1;
	}

 /* rand()�μ����ˤ�äƤϲ��̤ΥӥåȤϾ�̤ۤɥ�����Ǥʤ���Τ⤢��Τ�
    �Ǥ���в��̥ӥåȤ���������˻Ȥ��Τ��򤱤� */
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
		mmlproc[mmlproc_ptr].kind = -1; /* ���̡ᥭ������ */
		mmlproc[mmlproc_ptr].param.keyoff = keyproc2[i].keyproc;
		 /* ����&��������velocity */

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
			/* �����ˤ���ʤ��Ϥ� */
			i++; continue;
		default:
			x = x_values[kind] + mmlproc[i].param_ctrl;
			switch(kind){
			case MMLPROC_BEND: /* bend */
				ALIGN2(x); break;
			case MMLPROC_DTEMPO: /* tempo */
				break; /* �����ˤ��뤳�ȤϤ��ꤦ�뤫? */
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
	 /* minst_ptr��mmlproc[minst_ptr].st���Ǿ��Ȥʤ�褦��ź����minst��
	    �����Ǥ�st��
	    *add_steptime�Ϻǽ��1����������Ƥ��륹�ƥåץ����ࡣDTEMPO�ʳ���
	    ���٥�Ȥ��񤭽Ф������ϡ�write_length�ǽ񤭽Ф����Ĺ����
	    �����ä�������*add_steptime��0�ˤ��롣
	    *gatetime�ϥ������դޤǤΥ��ƥåץ����ࡣDTEMPO�ʳ��Υ��٥�Ȥ�
	    �񤭽Ф������Ϥ����minst�������餹 */
	int i, x, kind;
	long minst, actual_length;

	actual_length = minst = mmlproc[minst_ptr].st;
	if(add_steptime != NULL) actual_length += *add_steptime;

	kind = mmlproc[minst_ptr].kind;
	if(kind != MMLPROC_DTEMPO){
		*gatetime -= minst;
		if(add_steptime != NULL) *add_steptime = 0;
		write_length(actual_length, fp2);
	} /* DTEMPO�ξ��ƥ�ݥޥåפ˽񤫤�Ƹ�ǽ��������Τ�
	     �����Ǥ�write_length���ʤ� ��ä�tstep��ʤޤʤ� */

	switch(kind){
	case MMLPROC_KEYOFF: /* �������դλ� */
		noteoff(mmlproc[minst_ptr].param_onkai,
			mmlproc[minst_ptr].param_velo);
		break;
	default: /* control change��tempo�λ� */
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
			/* ���ߡ��Υ����å���ɬ��*/
			break;
		case MMLPROC_BEND:
			(void)check_bendchange(x); /*���ߡ��Υ����å���ɬ�� */
			break;
		/* case MMLPROC_CPRES:
			(void)check_aft(kind2param(kind), x);
		���ߡ��Υ����å������ס���Ȥ��Ϣ³�ͥ����å����Ƥʤ����� */
		}
		do_mmlproc0(kind, x, actual_length);
		 /* do_mmlproc0()��kind��DTEMPO�ξ��ʳ���3�����Ի��� */
		break;
	}

	 /* �񤭹����mmlproc�����Ƥ���ʸ��1�Ĥ��餷��st�򸺤餹 */
	shift_array(mmlproc, minst_ptr, mmlproc_ptr);
	mmlproc_ptr--;
	if(kind != MMLPROC_DTEMPO){
		for(i = 0; i < mmlproc_ptr; i++) mmlproc[i].st -= minst;
	}
}

/* �������ե٥��ƥ������θ���ƥΡ��ȥ��դ���ؿ� */
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
	default: /* Ĵ��򸫤� */
		if(german_scale){
			if(onpu == 'b') break; /* Ĵ����԰��� */
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
