SEEK_CUR	EQU 1
SEEK_END	EQU 2
SEEK_SET	EQU 0


gettop		macro
		mov	ax,[si].top_seg
		mov	dx,ax
		shl	ax,4
		shr	dx,12
		endm

		.186

		.MODEL	SMALL
	
		.DATA
	
file struc	;TopはDOSファンクションから得たSeg:offset がそのまま入る。
		;cur,endは１Ｍフラット値が入る。使用時に変換して使う。
top_adr		dw	?
top_seg		dw	?
cur_adr		dw	?
cur_seg		dw	?
end_adr		dw	?
end_seg		dw	?
flag		dw	?
f		dw	?
file ends

		.CODE


putcur		macro	cn1
		endm
		; pusha
		; mov	ax,0ff00h
		; mov	al,cn1
		; call	puthex2
		; mov	ax,[si].cur_seg
		; call	puthex2
		; mov	ax,[si].cur_adr
		; call	puthex2
		; mov	ax,0eeeeh
		; call	puthex2
		; popa
		;endm

;------------------------------------------------
;curがendを越えていればend=curに設定する。
check_end1	proc	near

		push	ax
		mov	ax,[si].cur_seg
		cmp	ax,[si].end_seg
		jz	@che1jump1
		ja	@che1jump2
@che1jump3:	pop	ax
		ret
@che1jump1:
		mov	ax,[si].cur_adr
		cmp	ax,[si].end_adr
		jna	@che1jump3
@che1jump2:	mov	ax,[si].cur_seg
		mov	[si].end_seg,ax
		mov	ax,[si].cur_adr
		mov	[si].end_adr,ax
		
		pop	ax
		ret

check_end1	endp

;------------------------------------------------
; DX:AXに格納されたポインタをリニア変換
		PUBLIC	get_liner
get_liner	proc	near

		push	cx
		mov	cx,dx
		shr	dx,12
		shl	cx,4	;dx:cx=Seg部のリニア
		add	ax,cx
		adc	dx,0
		pop	cx
		ret

get_liner	endp

;------------------------------------------------
; int getc2(file *fp)
		PUBLIC _getc2
_getc2		proc near

		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	bx
		push	si

		lds	si,[bp+4]
		
    ;if(fp->cur_adr == fp->end_adr) return -1;
		mov	bx,[si].cur_adr
		cmp	bx,[si].end_adr
		jz	@gc2jump1	;end_adrに到達している（かも）
@gc2jump2:
    ;i = (unsigned char)fp->cur_adr[0];
		mov	ax,[si].cur_seg
		shl	ax,12
		mov	es,ax
		mov	al,es:[bx]
		xor	ah,ah
		add	[si].cur_adr,1
		adc	[si].cur_seg,0
		pop	si
		pop	bx
		pop	es
		pop	ds
		pop	bp
		ret	;4

@gc2jump1:	mov	ax,[si].cur_seg
		cmp	ax,[si].end_seg
		jnz	@gc2jump2	;やっぱりendには達していない。
		mov	ax,-1
		pop	si
		pop	bx
		pop	es
		pop	ds
		pop	bp
		ret	;4

_getc2		endp
;------------------------------------------------
; void fseek2(file *fp, long l, int s)
		PUBLIC	_fseek2
_fseek2		PROC NEAR

		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	ax
		push	bx
		push	dx
		push	si

		lds	si,[bp+4]
    ;if(fp->cur_adr > fp->end_adr) fp->end_adr = fp->cur_adr;
		call	check_end1
		
		mov	ax,[bp+12]
		cmp	ax,SEEK_CUR
		jnz	@fskjump1
		
		;------ SEEK_CUR処理 --------
		mov	ax,[bp+8]	;dx:ax = l
		mov	dx,[bp+10]
		add	[si].cur_adr,ax
		adc	[si].cur_seg,dx
		 putcur	0
		jmp	@fskjump2
@fskjump1:
		cmp	ax,SEEK_SET
		jnz	@fskjump2
		
		;------ SEEK_SET処理 ---------
		;mov	ax,[si].top_adr
		;mov	dx,[si].top_seg
		;call	get_liner
		gettop
		add	ax,[bp+8]	;dx:ax = l
		adc	dx,[bp+10]
		mov	[si].cur_adr,ax
		mov	[si].cur_seg,dx
		 putcur	1
@fskjump2:
		;-------- 終了時の比較処理 ----------
		call	check_end1
		
		pop	si
		pop	dx
		pop	bx
		pop	ax
		pop	es
		pop	ds
		pop	bp
		ret	;10

_fseek2		ENDP

;------------------------------------------------
;file *fopen2(char c1[], char c2[])
		PUBLIC	_fopen2
_fopen2		proc near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di
		
    		mov	cx,ds	;ds退避
    ;file *fp = (file *)malloc(sizeof(file));
		mov	ah,48h
		mov	bx,1	;FILE struc のサイズ = 16
		int	21h
		jnc	@fop2jump8
		jmp	@fop2err
@fop2jump8:
		mov	ds,ax	;DS:SI = 構造体アドレス
		mov	si,0
	
		push	ds
		mov	ds,cx
		mov	bx,[bp+6]	;DS:BX=c2[]
		cmp	byte ptr [bx],'r'
		pop	ds
		jz	@fop2jump7
		jmp	@fop2jump1
@fop2jump7:
		;--------- file Open (read) ----------
		push	ds		;Open
		mov	ds,cx
		mov	dx,[bp+4]
		mov	ax,3d00h
		int	21h
		pop	ds
		jnc	@fop2jump6
		jmp	@fop2err2
@fop2jump6:
		mov	[si].f,ax	;Set File Handle
		mov	[si].flag,0
		mov	bx,ax
		mov	ax,4202h
		xor	dx,dx
		xor	cx,cx
		int	21h		;DX:AX = FileSize
		
		push	ax		;Pointerを元に戻しておく
		push	dx
		mov	ax,4200h
		xor	dx,dx
		xor	cx,cx
		int	21h
		pop	dx
		pop	ax
		
		push	ax		;メモリ確保
		push	bx
		push	dx
		add	ax,3	;CR,LF,EOFの３文字分余分に確保
		add	ax,15	;dx:axをパラグラフ単位に。
		adc	dx,0
		and	ax,0fff0h
		mov	bx,ax	;BX=(DX:AX)>>4
		shr	bx,4
		shl	dx,12
		or	bx,dx
		
		mov	ah,48h
		int	21h	;Alloc
		mov	[si].top_seg,ax
		mov	[si].top_adr,0
		pop	dx
		pop	bx
		pop	ax
		jnc	@fop2jump5
		jmp	@fop2err3
@fop2jump5:
		push	ax
		push	dx
		gettop	;cur=top , ただしリニアに変換する
		mov	[si].cur_seg,dx
		mov	[si].cur_adr,ax
		pop	dx
		pop	ax
		
		add	ax,[si].cur_adr	;end=top+cur
		adc	dx,[si].cur_seg
		mov	[si].end_adr,ax
		mov	[si].end_seg,dx
		
		push	ds		;ファイルリード
		mov	ax,[si].top_seg
		mov	ds,ax
		xor	dx,dx
@fop2loop1:
		mov	cx,8000h
		mov	ah,3fh
		int	21h
		cmp	ax,8000h
		jc	@fop2jump3
		mov	ax,ds
		add	ax,800h
		mov	ds,ax
		jmp	@fop2loop1
@fop2jump3:	push	bx
		mov	bx,ax
		mov	byte ptr [bx],0dh
		mov	byte ptr [bx+1],0ah
		mov	byte ptr [bx+2],1ah
		
		pop	bx
		pop	ds
		jmp	@fop2jump4

@fop2jump1:
		;--------- file Create (Write) ----------
        ;fp->f = creat(c1,S_IREAD | S_IWRITE);
		push	ds
		mov	ds,cx
		mov	dx,[bp+4]
		mov	ah,3ch
		mov	cx,20h
		int	21h
		pop	ds
		jc	@fop2err2
		mov	[si].f,ax	;Set File Handle
        
        ;fp->flag = 1;
		mov	[si].flag,1
        
        ;fp->top_adr = fp->cur_adr = (char *)malloc(65000);
	;if(fp->top_adr == NULL){
	;    free(fp);
	;    return NULL; /* 異常終了 */
	;}
		mov	bx,1000h	;10000h(=65536)のパラグラフ
		mov	ah,48h
		int	21h
		jc	@fop2err3
		mov	[si].top_adr,0
		mov	[si].top_seg,ax
		gettop
		mov	[si].cur_adr,ax
		mov	[si].cur_seg,dx
        ;fp->end_adr = fp->top_adr;
		mov	[si].end_adr,ax
		mov	[si].end_seg,dx
@fop2jump4:
		mov	ax,si
		mov	dx,ds
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;8
		
@fop2err3:	;--------------- 異常終了 ---------------
		mov	bx,[si].f	;File Handleを解放
		mov	ah,3eh
		int	21h
@fop2err2:
		mov	ax,ds	;FILE構造体を解放
		mov	es,ax
		mov	ah,49h
		int	21h
@fop2err:
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		xor	ax,ax
		xor	dx,dx
		ret	;8

_fopen2		endp

;------------------------------------------------
close_sub	proc near

        	push	bp
        	
        ;if(fp->cur_adr > fp->end_adr) fp->end_adr = fp->cur_adr;
		call	check_end1
	
	;(void)write(fp->f,fp->top_adr,(long)(fp->end_adr - fp->top_adr));
		push	ds
		mov	bx,[si].f
		gettop
		
		mov	bp,[si].end_seg
		mov	di,[si].end_adr	;BP:DI=End
		sub	di,ax
		sbb	bp,dx		;BP:DI=Size
		
		mov	ax,[si].top_seg
		mov	ds,ax
@fcl2loop1:
		or	bp,bp
		jz	@fcl2jump2
		mov	ah,40h		;800h書き込み・１回目
		xor	dx,dx
		mov	cx,8000h
		int	21h
		mov	ax,ds
		add	ax,800h
		mov	ds,ax
		mov	ah,40h		;800h書き込み・２回目
		xor	dx,dx
		mov	cx,8000h
		int	21h
		mov	ax,ds
		add	ax,800h
		mov	ds,ax
		dec	bp
		jmp	@fcl2loop1
@fcl2jump2:
		mov	ah,40h		;64k未満の半端書き込み
		xor	dx,dx
		mov	cx,di
		int	21h
		pop	ds

		pop	bp
		ret
		
close_sub	endp

;------------------------------------------------
;void fclose2(file *fp)
		PUBLIC	_fclose2
_fclose2	proc near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di
		
		lds	si,[bp+4]
    ;if(fp->flag == 1){
		cmp	[si].flag,1
		jnz	@fcl2jump1
        
        	call	close_sub
    ;}
@fcl2jump1:
    ;close(fp->f);
		mov	bx,[si].f
		mov	ah,3eh
		int	21h
    ;ffree(fp);
		mov	es,[si].top_seg
		mov	ah,49h
		int	21h
		
		mov	ax,ds
		mov	es,ax
		mov	ah,49h
		int	21h
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;4

_fclose2		endp

;------------------------------------------------
;void fgetpos2(file *fp, long *l)
		PUBLIC	_fgetpos2
_fgetpos2	proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di
		
		mov	ax,ds
		mov	es,ax
		
		mov	bx,[bp+8]
		lds	si,[bp+4]
    ;l[0] = (long)(fp->cur_adr - fp->top_adr);
		
		gettop
		mov	cx,[si].cur_adr
		sub	cx,ax
		mov	es:[bx],cx
		mov	cx,[si].cur_seg
		sbb	cx,dx
		mov	es:[bx+2],cx
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;8
		
_fgetpos2	endp

;------------------------------------------------
;void fsetpos2(file *fp, long *l)
		PUBLIC	_fsetpos2
_fsetpos2	proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di
		
		mov	ax,ds
		mov	es,ax
		
		mov	bx,[bp+8]
		lds	si,[bp+4]
    
    ;if(fp->cur_adr > fp->end_adr) fp->end_adr = fp->cur_adr;
		call	check_end1
    
    ;fp->cur_adr = fp->top_adr + l[0];
		gettop
		mov	cx,es:[bx]
		add	cx,ax
		mov	[si].cur_adr,cx
		mov	cx,es:[bx+2]
		adc	cx,dx
		mov	[si].cur_seg,cx
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;8
		
_fsetpos2	endp
		
;------------------------------------------------
;void fclose3(file *fpa, file *fpb)
		PUBLIC	_fclose3
_fclose3		proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di

		lds	si,[bp+4]	;fpa
		call	close_sub
		
		mov	ax,[si].f	;fpaのハンドルを取得
		lds	si,[bp+8]
		xchg	[si].f,ax	;fpb->fにfpa->fを代入,fpb->fを保存
		push	ax
		call	close_sub
		pop	ax
		mov	[si].f,ax	;fpb->fを戻す
		
    ;close(fp->f);
		lds	si,[bp+4]	;再度fpa
		mov	bx,[si].f
		mov	ah,3eh
		int	21h
    ;ffree(fp);
		mov	es,[si].top_seg
		mov	ah,49h
		int	21h
		
		mov	ax,ds
		mov	es,ax
		mov	ah,49h
		int	21h
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;8
		
_fclose3		endp

;------------------------------------------------
;void fclose4(file *fpa, file *fpb)
		PUBLIC	_fclose4
_fclose4	proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di

		lds	si,[bp+4]	;fpa
		call	check_end1
		
		mov	ax,[si].f	;fpaのハンドルを取得
		lds	si,[bp+8]
		xchg	[si].f,ax	;fpb->fにfpa->fを代入,fpb->fを保存
		push	ax
		call	close_sub
		pop	ax
		mov	[si].f,ax	;fpv->fを戻す
		
    ;close(fp->f);
		lds	si,[bp+4]	;再度fpa
		mov	bx,[si].f
		mov	ah,3eh
		int	21h
    ;ffree(fp);
		mov	es,[si].top_seg
		mov	ah,49h
		int	21h
		
		mov	ax,ds
		mov	es,ax
		mov	ah,49h
		int	21h
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;8
		
_fclose4	endp

;------------------------------------------------
;file *fmalloc(void)
		PUBLIC	_fmalloc
_fmalloc	proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di

    ;file *fp = (file *)malloc(sizeof(file));
    ;if(fp == NULL) return NULL; /* 異常終了 */
		mov	ah,48h
		mov	bx,1	;FILE struc のサイズ = 16
		int	21h
		jc	@fmalerr
		
		mov	ds,ax	;DS:SI = 構造体アドレス
		mov	si,0
		
    ;fp->top_adr = fp->cur_adr = (char *)malloc(65000);
		mov	bx,2800h	;64k*2.5 = 160kのパラグラフ
		mov	ah,48h
		int	21h	;Alloc
		mov	[si].top_seg,ax
		mov	[si].top_adr,0
		jc	@fmalerr2
		gettop
		mov	[si].cur_seg,dx
		mov	[si].cur_adr,ax
    ;fp->end_adr = fp->top_adr;
		mov	[si].end_seg,dx
		mov	[si].end_adr,ax
		
    ;return fp; /* 正常終了 */
		mov	dx,ds
		mov	ax,si
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret
		
@fmalerr2:	;--------- 異常終了 -------------
		mov	ax,ds	;FILE構造体を解放
		mov	es,ax
		mov	ah,49h
		int	21h
@fmalerr:
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		xor	ax,ax
		xor	dx,dx
		ret

_fmalloc		endp

;------------------------------------------------
;void ffree(file *fp);
		PUBLIC	_ffree
_ffree		proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di

		lds	si,[bp+4]
		
		mov	es,[si].top_seg
		mov	ah,49h
		int	21h
		
		mov	ax,ds
		mov	es,ax
		mov	ah,49h
		int	21h
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;4

_ffree		endp

;------------------------------------------------
; void putc2(int c, file *fp);
		PUBLIC	_putc2
_putc2		proc	near
		
		push	bp
		mov	bp,sp
		push	ds
		push	es
		push	si
		push	di

		lds	si,[bp+6]
		mov	bx,[si].cur_adr
		mov	ax,[si].cur_seg
		shl	ax,12
		mov	es,ax
		mov	ax,[bp+4]
		mov	es:[bx],al
		add	[si].cur_adr,1
		adc	[si].cur_seg,0
		
		pop	di
		pop	si
		pop	es
		pop	ds
		pop	bp
		ret	;6
_putc2		endp

;------------------------------------------------
;＝＝＝＝＝＝＝　１６進出力１（８ビット）　＝＝＝＝＝＝＝
puthex1		proc	near
		
		push	ax
		push	ax
		shr	al,1
		shr	al,1
		shr	al,1
		shr	al,1
		call	puthex3
		pop	ax
		and	al,0fh
		call	puthex3
		pop	ax
		ret
puthex1		endp

;＝＝＝＝＝＝＝　１６進出力２（１６ビット）　＝＝＝＝＝＝＝
puthex2		proc	near
		
		push	ax
		mov	al,ah
		call	puthex1
		pop	ax
		call	puthex1
		ret
puthex2		endp

;＝＝＝＝＝＝＝　１６進出力３（４ビット）　＝＝＝＝＝＝＝
puthex3		proc	near

		push	ax
		push	dx
		
		add	al,'0'
		cmp	al,'9'+1
		jc	@ph3jump1
		add	al,7
@ph3jump1:	mov	dl,al
		mov	ah,6
		int	21h
		
		pop	dx
		pop	ax
@ph3jump2:
		ret
puthex3		endp

		END

