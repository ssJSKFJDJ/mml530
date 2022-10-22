
/*
 *      file    name            charproc.c
 */

/*
   ���Υե�����Ǥϡ�
   �ȥ�å����飱ʸ����äƤ������������ӡ��ޥ���ν�����Ԥ���
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

extern void mml_err(int); /* mml2mid.c �ˤ��� */

extern fileptr fp2;         /* ���ϥե�����(mid) */
extern void write_length(long, fileptr);

extern fileptr fp1;

extern int cur_line;           /* ���ߤ�MML��������ι��ֹ� */
extern int tnum; /* �ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� 0,1,2 */
extern int talf; /* �ȥ�å���ɽ�� 0A 1B 2C �ʤɤ� A,B,C */
extern int kloop_ptr;          /* �롼�ץͥ��ȿ� */
/* --- �ʲ����ԣͣˣҤ��ɲ� */
extern Fpos_t lastlenpos; /* �Ǹ�˲�Ĺ��񤭹�������ؤΥݥ��� */
extern long lastlen;      /* �Ǹ�˽񤭹������Ĺ���� */
extern long tstep;
struct master_step *master_step;	/* MKR�ɲ� */ /* Mod Nide */
int master_step_amount;
int master_count;			/* MKR�ɲ� */
int master_count_temp, master_count_temp_prev; /* Mod Nide */
#define master_step_temp (master_step[master_count_temp].step)
extern int mskanji;

extern int head; /* ��°�ѡ��Ȥ˥إå���񤤤����ɤ����Υե饰 */
extern void write_header(void);

static int cur_macro;          /* �����ɤ߹�����Υޥ��� */
static int cur_macronum;       /* �ޥ���θ����ɤ߹������ʸ�� */
/* static unsigned char **macros; */ /* �ޥ�����¸�� */
typedef struct macrostruct {
	int alcsiz, uselen;	/* alloc�������Ȼ�����Ĺ�� */
	unsigned char *mac;	/* ���� */
} macrostruct;
static macrostruct *mcrstr;	/* �ޥ�����¸�� */
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
 /* BCC�ʳ��Ǥ�file.h�Ǽ¸����Ƥ��� �����BCC�Ǥ�file.asm�Ǽ¸������ߤ��� */
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
#define freemacro(m) /* ���ꤷ���ޥ���Υ������ */ \
	free((m)->mac)
#define clearmacro(m) /* ���ꤷ���ޥ���Υ��ꥢ */ do{ \
	reallocmacro((m), 200); \
	(m)->uselen = 0; \
} while(0)
#define initmacro(m) /* ���ꤷ���ޥ���ν���� */ do{ \
	(m)->mac = 0; \
	clearmacro(m); \
} while(0)
 /* (m)��calloc������ΤǤ������clearmacro()�ǽ�ʬ����ư�ѿ��Ǥ�����ʤ�
    �ϡ��ǽ��(m)->mac��NULL�ˤ���ɬ�פ����뤿�ᡢinitmacro()������Ȥ� */
#define macrosizchk(m, i) /* ������i��­��ʤ���кƳ������ */ do{ \
	if((i) >= (m)->alcsiz) reallocmacro((m), (i) + 100); \
} while(0)
#define macroaddchar(m, c) /* �ޥ����ʸ��c���ɲá�ɬ�פʤ�Ƴ������ */ do{ \
	macrosizchk((m), (m)->uselen); \
	(m)->mac[(m)->uselen++] = (c); /* c��1�󤷤�ɾ������ʤ� */ \
} while(0)
#define macroNaddchar(n, c) /* �ֹ�n�Υޥ����ʸ��c���ɲ� */ \
	macroaddchar(&mcrstr[n], (c))

void macrocat(macrostruct *to, macrostruct *from){
	 /* �ޥ���to�θ���from���ɲ� â��to��������NULL�μ�ư�ɲäϤ��ʤ� */
	int needed = to->uselen + from->uselen - 1;
	macrosizchk(to, needed);
	memcpy(&to->mac[to->uselen], from->mac, from->uselen - 1);
	to->uselen += from->uselen - 1;
}

/* ���ޥ���β��� */
void free_all_macros(void)
{
	int i;

	for(i = 0; i < 260; i++) free(macros(i));
	free(mcrstr);
}

/* ���ޥ������������ */
void init_all_macros(void)
{
	mcrstr = (macrostruct *)calloc(10 * 26,	sizeof(macrostruct));
	if(NULL == mcrstr) mml_err(61);
}

/* �ޥ�����Υ롼�פ�Ÿ������ */
#define MAX_MACROLOOP_NEST_LEVEL 16
static void macroloop(unsigned char *mcr_src, macrostruct *mcr_dst)
{
	int inner_string_p = 0;
	int count;
	struct {
		unsigned char *start_pos;	/* mcr_src��Υ롼�׳��ϰ��� */
		int loop_count;			/* �롼�פ��ä���� */
	} mloop[MAX_MACROLOOP_NEST_LEVEL];
	int mloop_ptr = 0;
	int x, xx;

	for(;;){
		xx = *mcr_src++;
		if(inner_string_p){ /* ʸ������Ǥϡ�[ ]�פ�Ÿ���Ϥ��ʤ�(Nide) */
			 /* �ޥ������ʸ�����Ĥ��Ƥʤ����顼�Υ����å���
			    getmacro()���macroloop()��Ƥ����˺Ѥ�Ǥ�Ϥ� */
			switch(xx){
			case '"':
				break;		/* ���̤�Ʊ������ */
			case '\\': /* ��\n�פʤɤν����Ϥ��ȤǤ�� �����Ǥ�
				 ��\�פμ����ü�ʸ���������ʤ����������ǽ�ʬ */
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

			mloop[mloop_ptr].start_pos = mcr_src; /* �롼�׳��������Υ��å� */
			mloop[mloop_ptr].loop_count = -1;
				/* �롼�פ�����.�ǽ��-1�ˤ��Ƥ��� */
			mloop_ptr++;
			break;
		case '/':
		case ':':	/* �롼�ץ����� */
			if(!mloop_ptr) mml_err(66); /* �롼�׳����ä� */
			if(mloop[mloop_ptr - 1].loop_count == 1){
				int loop_inner_string_p = 0;

				if(xx == '/') mml_warn(2); /* obsolete�ηٹ� */

				count = 0;
				for(;;){
					x = *mcr_src++;	
					if(x == '\0')
						mml_err(48); /* ]���ʤ� */
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
						for(;;){ /* ���ͤ����Ф� */
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
				mloop_ptr--;	/* �롼�פ�ȴ�����Τǡ��ͥ��ȿ��򸺤餹 */
			}
			break;
		case ']':	/* �����֤��ν���ꡣ */
			if(mloop_ptr == 0) mml_err(47); /* �ޥ�����Υ롼�פ��� */
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
			 /* �ޤ��롼�פ��äƤʤ���硢������򥻥å� */
			if(mloop[mloop_ptr - 1].loop_count == -1)
				mloop[mloop_ptr - 1].loop_count = count;
			if(--mloop[mloop_ptr - 1].loop_count == 0){
				mloop_ptr--;
				break;
			}
			mcr_src = mloop[mloop_ptr - 1].start_pos; /* �롼�פ���Ƭ������ */
			break;
		case '\0':
			macroaddchar(mcr_dst, '\0');
			if(mloop_ptr != 0) mml_err(48); /* ]���ʤ� */
			return;
		case '"':
			inner_string_p ^= 1; /* FALLTHROUGH */
		default:
			macroaddchar(mcr_dst, xx);
			break;
		}
	}
}

/* �ޥ���Υ������ */
void scanmacro(void)
{
	int code;

	 /* �ޥ�����ι��ֹ���¸�ˤ�cur_line��Ȥ����Ȥˤ����̤��ѿ���
	    �Ѱդ���ΤϤ�᤿ */
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

/* scanmacro()����ƤФ�� �ޥ����������Ƥ���¸ */
static void getmacro(void)
{
	int code, inner_string_p;
	int cm; /* ������Υޥ��� */
	macrostruct mcr, mcr2;
	unsigned char *mcrptr;

	 /* �ޥ���������ֹ����Ƥ��� */
	code = getc2(fp1);
	cm = 0;
	if(is_digit(code)){
		cm = dtoi(code) * 26;
		code = getc2(fp1);
	}
	if(!is_lower(code)) mml_err(5);
	cm += code - 'a';

	 /* �ޥ����������Ƥ�mcr������� */
	initmacro(&mcr); /* �Ǹ�ˤϲ������뤳�� */
	for(inner_string_p = 0;;){
		code = getc2(fp1);
		if(code == EOF){
			break;
		} else
		if(code == '\n'){
			ungetc2(fp1); /* ���줬�ʤ��ȥ��� */
			 /* �����Ǵ��ɥݥ��󥿤�1���ᤷ�������scanmacro()��
			    �ǳ�for�롼����Ƭ��'\n'���ɤ��cur_line�����䤹 */
			break;
		} else
		if(code == '"'){
			inner_string_p ^= 1;
		} else
		if(inner_string_p &&
		   (code == '\\' || (mskanji && ismskanji1(code)))){
			macroaddchar(&mcr, code); /*��\n�פʤɤν����ϸ� */
			code = getc2(fp1);
			if(code == '\n' || code == EOF) mml_err(62);
			/* �ޥ������ʸ�����Ĥ��Ƥʤ��ĵ�����ʤ��Ϥ�������*/
		}
		macroaddchar(&mcr, code);
	} /* ���δؿ��Ǥ�fp1���ɤ�ΤϤ����ޤ� */
	macroaddchar(&mcr, '\0');

	 /* �ޥ�����Υ롼�פ�Ÿ������ mcr2�ϺǸ�ˤϲ������뤳�� */
	initmacro(&mcr2);
	macroloop(mcr.mac, &mcr2);
	freemacro(&mcr);
				
	 /* 2������Ǥ�(�Ĥޤ����macros(cm)!=NULL�Ǥ�)���顼�ȤϤ��ʤ� */
	clearmacro(&mcrstr[cm]);

	 /* mcr2��Υޥ����Ÿ�����Ĥ�mcrstr[cm]�إ��ԡ�
	    â��ʸ������Ǥϥޥ���Ÿ���Ϥ��ʤ�(Nide) */
	for(inner_string_p = 0, mcrptr = mcr2.mac;;){
		code = *mcrptr++;
		if(code == '\0') break;
		if(code == '"'){
			inner_string_p ^= 1;
		} else
		if(inner_string_p &&
		   (code == '\\' || (mskanji && ismskanji1(code)))){
			macroNaddchar(cm, code); /* ��\n�פʤɤν����ϸ� */
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
				mml_err(45); /* ̤����ޥ����Ƥ�� */
			if(cm == cm2)
				mml_err(63); /* �ޥ����Ƶ����Ƥ��� */

			macrocat(&mcrstr[cm], &mcrstr[cm2]);
			continue;
		}
		macroNaddchar(cm, code);
	}
	macroNaddchar(cm, '\0'); /* �ޥ���ʸ����ν�ü�����ɤ�NULL */
	freemacro(&mcr2);

	/* printf("macro number %d = '%s'\n", cm, macros(cm)); */
}

/*
   tnum, talf���б�����ȥ�å�����1ʸ����äƤ��롥
   ��äƤ���ʤ����(�ȥ�å��κǸ���褿���)�� -1 ���֤���
   ���ڡ��������֡�!�ʤɤν����⤳���Ǥ�롣
   �ޥ���ν����⤳���Ǥ�롣

   x == 0 ��ʸ����ä���˥ݥ��󥿰�ư��
   x == 1 ��ʸ����ä���˥ݥ��󥿤ϰ�ư���ʤ���
   x == 2 �ݥ��󥿰�ư��������������ľ����getbyte(1)���ƤФ�Ƥ���ɬ�פ����롣

   inner_string_p����0�ʤ饹�ڡ���̵���ޥ�������ʤɤϰ��ڤ��ʤ�(Nide)
   */
int Getbyte(int x, int inner_string_p)
{
	static int f = 0;	/* ���ιԤ�Ĵ�٤˹Ԥ����ɤ����Υե饰 0��2 */
	int i, code;		/* ���� */

	if(x == -1){f = 0; return 0;}

	for(;;){
		switch(f){
		case 0:		/* ���Υȥ�å�����Ƭ�˰�ư������ */
			i = nexttrack();
			if(i == -1) return -1;
			f = 1;
			 /* FALLTHROUGH */
		case 1:		/* �ȥ�å����椫�飱ʸ����äƤ����� */
			if(x == 2){
				fseek2(fp1, 1L, SEEK_CUR);
				return 0;
			}
			switch(code = track(inner_string_p)){
			case -1: /* EOF�����ä� */
				f = 0;	/* ���Υȥ�å�̾�Τ���� f = 0 ���ᤷ�Ƥ��� */
				return -1;
			case -2: /* �������褿 */
				f = 0;
				continue;
			case -3: /* �ޥ�����褿 */
				f = 2;
				continue;
			}
			/* �ȥ�å��⤫�飱ʸ����äƤ��� */
			if(x == 1) ungetc2(fp1);
			break;
		default:	/* case 2: �ޥ����⤫�飱ʸ����äƤ����� */
			if(x == 2){
				cur_macronum++;
				return 0;
			}
			code = macro(x, inner_string_p);
			if(code == -1){	/* �ޥ������ʸ�����Ԥ��� */
				f = 1;
				continue;
			}
			break;
		}

		 /* �ȥ�å����뤤�ϥޥ�����1ʸ����äƤ������ȥݥ��󥿤�
		    ��ư���������ζ��̽��� */
		if(x == 0){
			if (tnum==0) { /* ��ȥ�å��ξ�� */
				/* ��°�ȥ�å��Υ��ƥå׹�碌 */
				for (i=master_count-1;i>=0;i--) {
					if (master_step[i].step == -2) {
						master_step[i].step = tstep;
						 /* master_step[i].linenum������ѤߤΤϤ� */
					} else break;
				}
			} else { /* ��°�ȥ�å��ξ�� */
				if (master_count_temp>=0) { /* ��°�ȥ�å�����Ƭ */
				/* tstep ��1������(?)���ƥå�
				   master_step_temp�Ͻ�°�ȥ�å�����Ƭ�Υ��ƥå�
				   (master_step_temp == master_step[master_count_temp].step) */

					/* if(master_step_temp<tstep) mml_err(37); */
					if(head == 0){
						head = 1;
						write_header(); /* ��°�ȥ�å��Υإå���� */
					}
					/* ��ȥ�å��μ���Ʊ����°�ȥ�å������٤�³���Ƥ��Ƥ�
					   ���Τ��Ӥ˰��ֹ�碌�Ϥ��ʤ�
					   master_count_temp_prev��������ֹ�碌�����Ȥ���
					   master_count_temp���� */
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
		return code; /* �ȥ�å��⤢�뤤�ϥޥ����⤫��1ʸ����äƤ��줿 */
	}
}

/*
   ���Υȥ�å�����Ƭ�˰�ư���롣
   ���Υȥ�å����ʤ����ϡ�-1���֤���
   */
static int nexttrack()
{
	int li;

	for(;;){
		li=gettrack();
		switch(li){
		case -1:		/* EOF */
			return -1;
		case -2:		/* ��ȥ�å� */
			break;
		case -3:		/* ��°�ȥ�å������б������ȥ�å��ʤ� */
			mml_err(36);
		default:		/* ��°�ȥ�å����ȥ�å��˹�碌�褦
					   �Ȥ��Ƥ��� master_step[li].step��
					   ��碌��٤����ƥå� */
			master_count_temp=li;	/*�����Ǥ��ѿ��˳�Ǽ���������*/
			break;
		}

		if(skipts() != -1) return 0; /* ���ιԤ˲������ä� */
		/* ���ιԤ˲���ʤ����³�� */
	}
}

/*
   �ޥ�����1ʸ����äƤ��롣
   �ޥ������ʸ�����Ԥ����Ȥ���-1���֤���

   inner_string_p����0�ʤ����ʤɤ�̵�밷���Ϥ��ʤ�(Nide)
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
   �ȥ�å��⤫�飱ʸ����äƤ��롣
   ��äƤ���ʤ�(EOF)���ϡ�-1���֤���
   �������褿���ϡ�-2���֤���
   �ޥ�������ä����ϡ�-3���֤���

   inner_string_p����0�ʤ����̵���ޥ�������ʤɤϰ��ڤ��ʤ�(Nide)
   */
#define MAX_LOOP_NEST_LEVEL 16
static int track(int inner_string_p)
{
	static struct {
		Fpos_t start_pos;		/* �롼�׳��ϰ��� */
		int loop_count;			/* �롼�פ��ä���� */
		int start_line;			/* �롼�׳��������ι��ֹ� */
	} kloop[MAX_LOOP_NEST_LEVEL];
	int i, code, count;		/* ���� */

	for(;;){
		code = getc2(fp1);	/* ���ߤξ�꤫���ɤ�Ǥ��� */
		if(inner_string_p){ /* Add Nide */
			switch(code){
			case EOF: case '\n':
				break; /* ����2�ĤˤĤ��Ƥ�ʸ���󳰤�Ʊ������ */
			default:
				return code;
			}
		}
		switch(code){
		case EOF:
		/* case '!': */ /* '!'�ʹߤ�����̵�뤹�� */
			return -1;
		case '\n':
			ungetc2(fp1);
			return -2;
		/* case '\r': */ /* 0x0d ��̵�뤹�� */
		case '\t':	/* ���֤�̵�뤹�� */
		case ' ':	/* ���ڡ�����̵�뤹�� */
		case '|':	/* ��������̵�뤹�� */
			break;
		case '[':	/* �����֤��γ��� */
			if(kloop_ptr >= MAX_LOOP_NEST_LEVEL) mml_err(43);

			fgetpos2(fp1, &kloop[kloop_ptr].start_pos);
			 /* �롼�׳��������Υ��å� */
			kloop[kloop_ptr].loop_count = -1;
			 /* �롼�פ��������ǽ��-1�ˤ��Ƥ��� */
			kloop[kloop_ptr].start_line = cur_line; /* ���ֹ����¸ */
			kloop_ptr++;
			break;
		case '/':
		case ':':	/* �롼�ץ����� */
			if(!kloop_ptr) mml_err(66);
			if(kloop[kloop_ptr - 1].loop_count == 1){
				int loop_inner_string_p = 0;
				if(code == '/') mml_warn(2); /* obsolete�ηٹ� */

				count = 0;
				i = cur_line;	/* ���ֹ����¸�ʥ��顼��Ф��Ȥ��Τ���ˡ� */
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
							cur_line = i; /* ':'�����ä����ֹ���᤹ */
							mml_err(4);
						}
					}else if(code == EOF){
						cur_line = i; /* ':'�����ä����ֹ���᤹ */
						mml_err(4);
					}
				}
				kloop_ptr--;	/* �롼�פ�ȴ�����Τǡ��ͥ��ȿ��򸺤餹 */
			}
			break;
		case ']':				/* �����֤��ν���ꡣ */
			if(kloop_ptr == 0) mml_err(3);
			count = getnum();
			if(count == 0) count = 2;
			 /* �ޤ��롼�פ��äƤʤ���硢������򥻥å� */
			if(kloop[kloop_ptr - 1].loop_count == -1)
				kloop[kloop_ptr - 1].loop_count = count;
			if(--kloop[kloop_ptr - 1].loop_count == 0){
				kloop_ptr--;
				break;
			}
			fsetpos2(fp1, &kloop[kloop_ptr - 1].start_pos);
			 /* �롼�פ���Ƭ������ */
			cur_line = kloop[kloop_ptr - 1].start_line; /* ���ֹ���᤹ */
			break;
		case '$':	/* �ޥ��� */
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

				 /* Msg�˥ޥ����ֹ������� */
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
   tnum, talf ���б�����ȥ�å���1�Ԥ�����äƤ��롥
   �ʥȥ�å�����Ƭ�˥ݥ��󥿤�ܤ���

   0A��A��Ʊ����?��A����Z�Υ磻��ɥ����ɡ�����ɽ����ǽ��

   �ȥ�å�̾������:
   A       == 0A
   0A      == 0A
   1A      == 1A
   ?       == 0A - 0Z
   2?      == 2A - 2Z
   1A2B    == 1A, 2B
   0?2?    == 0A - 0Z, 2A - 2Z
   0A1BC2D == 0A, 1B, 0C, 2D
   ABCD1?  == 0A, 0B, 0C, 0D, 1A - 1Z

   1BC �ʤɤȤ�äƤ⡤1B, 1C �ˤϤʤ�ʤ�������ա�(1B,0C�ˤʤ�)
   1B,1C����ꤷ�������ˤϡ�1B1C�Ƚ񤫤ʤ��Ȥ����ʤ���
   ��Ƭ�˶���䥿�֤����äƤϤ����ʤ���
   �ȥ�å�̾��ľ��ˤ�1�İʾ�ζ��򤬤ʤ��Ȥ����ʤ���

   �ȥ�å�̾�ȸ��ʤ��ʤ����ϡ�̵�뤹�롥�ʥ��顼�ϽФ��ʤ���

   ���ߤι��ֹ椬��� cur_line �����롥

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

	 /* master_step_amount[master_count]�ޤǤ�¸�ߤ��ݾ� */
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

static void read_ppinfo(void)/* Add Nide, �����ץ�ץ��å�����ξ���ν��� */
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
		case '\\': /* ¾��·���� â����\n�פ�0x0a�ˤ���ʤɤν����Ͼʤ��� */
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
 /* �֤��ͤϡ����ߤΥ��ƥå׿�����Ǽ����Ƥ���master_step[]��ź��(��°
    �ȥ�å����ȥ�å��˹�碌�褦�Ȥ��Ƥ�����)�����뤤��-1(EOF), -2
    (��ȥ�å��ξ��), -3(��°�ȥ�å������б������ȥ�å����ʤ�) */
{
	int code;
	int i1, i2;

	if(talf == 0 && tnum == 0 && scan_flag == 0){ /* �ȥ�å��ޥåפκ��� */
		int talf2;

		scan_flag = 1;
		for(talf2 = 0; talf2 < 26; talf2++){
			if(gettrack2(talf2) != -1) track_map[talf2] = 1;

			fseek2(fp1, 0L, SEEK_SET); /* �ե��������Ƭ�˰�ư */
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
			if(i1 == '#'){ /* �����ץ�ץ��å�����ξ���Nide */
				read_ppinfo();
				break;
			} else
			if(i1 == '$'){ /* �ޥ�������� */
				break;
			} else
			if(i1 == '='){ /* ��Ƭ�� =n ����ꤷ����� */
				ungetc2(fp1);
				return tnum ? master_count : -2;
			}

			if(is_upper(i1) || i1 == '?'){ /*0��9����ά����Ƥ���*/
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
				if(tnum == 0){ /* ��ȥ�å����� */
					if(i1 == '0'){ /* ��ȥ�å�ȯ�� */
						master_count++;
						check_master_step_amount();
						master_step[master_count-1].step = -2;
						master_step[master_count-1].linenum = cur_line;
						master_step[master_count].step = -1;
						return -2;
					} else { /* ��°�ȥ�å�ȯ�� */
						track_map[dtoi(i1) * 32 + talf] = 1;
						/* ��°�ȥ�å��ޥå׺��� */
					}
				} else { /* ��°�ȥ�å����� */
					if(i1 == '0'){ /* ��ȥ�å�ȯ�� */
						master_count++;
						check_master_step_amount();
					   /* master_step[]�ؤγ�Ǽ�ϼ�ȥ�å�
					      �������˽���äƤ��뤫������? */
					} else if(dtoi(i1) == tnum){ /* ��°�ȥ�å�ȯ�� */
						if(master_count == -1){
							return -3; /* ��ȥ�å����ʤ� */
						}
						if(!kloop_ptr){ /* ��[ ]�פˤ��롼����Ǥʤ����
						  ��ľ���μ�ȥ�å�����Ƭ���ƥå�(�������롼����
						  �ʤ餽�κǽ�λ��Υ��ƥå�)�˹�碌���㤪�� */
							int tmp1, tmp2;

							tmp1 = tmp2 = master_count;
							/* ��ȥ�å������˸��Ĥ��äƤ��뤫��
							   master_count >= 0 */
							while(master_step[tmp1+1].step >= 0 &&
							      master_step[tmp1+1].linenum <= cur_line){
							 /* master_step�Υǡ�������������뤳�ȤϤʤ�
							    �Ǹ��-1��ɬ������(check_master_step_amount
							    ��Ƥ֤ʤɤ��Ƥ�)�Ϥ��ʤΤǡ�master_count
							    �Υ����Хե������å��Ϥ��ʤ��Ƥ��� */
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

/* �ȥ�å��ޥå׺������˸ƤФ�� EOF��-1, �����Ǥʤ����0�֤� */
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

			if(!is_digit(i1)){ /*0��?����ά����Ƥ���*/
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
   �ȥ�å��κǽ�Υ��ڡ��������֤��������ޤǰ�ư���롥
   ���ڡ����ޤǤ��Ѥ�ʸ�������ä��饨�顼�����ڡ��������֤���������
   ���Ĥ��������EOF�������˹Ԥ������ä��顢-1���֤�  */
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
   ���ͤμ���
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
