#!/usr/bin/wish
# tkmml2mid v0.1d

############### ����ե�����졼�������

##### ��å�����������ܸ�ˤ��뤫 1:���ܸ� 0:�Ѹ�
set conf(Japanese) 1

##### ��ʬ��̾��
set conf(myname) tkmml2mid

##### �������ޥ�ɤ�����  ���ޥ��̾��configuration��
##### ���ץ������ߤη������줹�뤿��ߤ�ʥꥹ�Ȥˤ��Ƥ���

# MML��MID����С�����(�����)ɬ�ܥ��ץ����
set conf(mml2mid) {mml2mid}

if {[string compare $tcl_platform(platform) windows] != 0} { # UNIX������
	# MIDI�ץ졼�������
	# ��{{{���ܸ�����} {�Ѹ�����}}	{���ޥ�ɥ饤��}}�פ�1���å�
	# ʣ�����åȤ�����ǽ�Τ�Τ��ǥե����
	set conf(playmidi_list) {
		# ������ˤ����Ϲ�Ƭ(or��Ƭ�ζ����ľ��)�ˤ����񤱤ʤ�

		# TiMidity(Tcl/Tk���󥿥ե�����)
		{
			{
				{Timidity (Tcl/Tk���󥿥ե�����)}
				{Timidity (Tcl/Tk interface)}
			}
			{timidity -ik}
		}

		# Playmidi(X���󥿥ե�����������)
		{
			{
				{Playmidi (X11���󥿥ե�����)}
				{Playmidi (X11 interface)}
			}
		      	{xplaymidi}
		}

		# TiMidity(XAW���󥿥ե�����)�Ŀ���
		{
			{
				{Timidity (XAW���󥿥ե�����)}
				{Timidity (XAW interface)}
			}
			{timidity -ia}
		}
		# ���Ѥ���TiMidity��Tcl/Tk, XAW�Τɤ���Υ��󥿥ե�������
		# �Ȥ߹��ޤ�Ƥ��ʤ������׽���
	}
	# �������ե�����ˤϰʾ�Τ��������ܤι��ܤ����Ф�Ƥ��뤫���񤫤��

	# MML�ե������Խ��ѥ��ǥ���
	set conf(editor) {emacs}
	# set conf(editor) {xemacs}
	# set conf(editor) {mule}
	# set conf(editor) {kterm -e vi}
		# kterm�򵯤����Ƥ������vi��ư����

	# mml2mid WebPage�����ѥ֥饦��
	set conf(webBrouser) {firefox}
	# set conf(webBrouser) {mozilla}
	# set conf(webBrouser) {kterm -e lynx}
		# kterm�򵯤����Ƥ������lynx��ư����

	# �ɥ�����ȥե���������ѥ��ޥ�� ���ˤ���������ʰץӥ塼����Ȥ�
	set conf(viewDoc) {}
	# set conf(viewDoc) {kterm -e less}
		# kterm�򵯤����Ƥ������less��ư����
	# set conf(viewDoc) {sh -c {exec emacs "$1" -f view-mode} dummy}
		# emacs�򵯤�����view�⡼�ɤǥɥ�����Ȥ򸫤�
} else { # Windows������
	# MIDI�ץ졼�������
	# ��{{{���ܸ�����} {�Ѹ�����}} {���ޥ�ɥ饤��}}�פ�1���å�
	# ʣ�����åȤ�����ǽ�Τ�Τ��ǥե����
	set conf(playmidi_list) {
		# ������ˤ����Ϲ�Ƭ(or��Ƭ�ζ����ľ��)�ˤ����񤱤ʤ�

		# Windows�Υ�ǥ����ץ졼��
		{
			{
				{��ǥ����ץ졼��}
				{Media player}
			}
			{mplayer -play -close}
		}

		# ��Ϣ�Ť����ޥ�ɡ�start���ޥ�ɤǵ�ư
		{
			{
				{��.mid�פ˴�Ϣ�Ť���줿���ޥ��}
				{".mid"-related command}
			}
			{start}
		}
	}

	# MML�ե������Խ��ѥ��ǥ���
	set conf(editor) {notepad}

	# mml2mid WebPage�����ѥ֥饦��
	set conf(webBrouser) {start}	;# .html�˴�Ϣ�դ����Ƥ��륳�ޥ��
	# set conf(webBrouser) {{c:\Program Files\Internet Explorer\iexplore}}
		# ��Ϣ�Ť��˴ؤ�餺���IE��Ȥ��������
		# ���꤫��ʬ��Ĵ�٤Ƥ����˽񤯤���

	# �إ�ױ����ѥ��ޥ��
	set conf(viewDoc) {winhelp}	;# Windows�Υإ��
	# set conf(viewDoc) {start}	;# .hlp�˴�Ϣ�Ť����Ƥ��륳�ޥ��
}

##### mml2mid Web�ڡ���URL
set conf(mml2mid_www) "http://hpc.jp/~mml2mid"

##### �ɥ�����Ȥ�����ե�����Τ��꤫
if {[string compare $tcl_platform(platform) unix] == 0} { # UNIX������
	# �ɥ�����ȥե�����(�ƥ�����)
	set conf(docFile) "/usr/local/lib/mml2mid/mml2mid.txt"

	# �������ե�����
	set conf(cnfFile) [file join $env(HOME) ".tkmml2mid"]
		# �ۡ���ǥ��쥯�ȥ겼
} else { # Windows������
	# �إ�ץե�����
	set conf(docFile) [file join [file dirname $argv0] "mml2mid.hlp"]
		# �ܥե������Ʊ���ǥ��쥯�ȥ�β�

	# �������ե�����
	set conf(cnfFile) [file join [file dirname $argv0] "tkmmlmid.rc"]
		# �ܥե������Ʊ���ǥ��쥯�ȥ�β� �����NT�ǤϤޤ���?
}

##### ����ե���������ꥦ����ɥ����ѹ���ǽ�ʤ�ΤΥǥե������
set conf(outFmt.def) 1		;# MIDI�ե��������format 0��format0 1��format1
set conf(revParen.def) 0	;# ��(�ס�)�פ�դˤ��뤫 1�Ĥ���
set conf(revAngbr.def) 0	;# ��<�ס�>�פ�դˤ��뤫 1�Ĥ���
set conf(transp.def) 0		;# ��Ĵ(Ⱦ��ñ��) +�Ĺ⤤����
set conf(doPlay.def) 0		;# ����ѥ����˱��դ��뤫 1�Ĥ���
set conf(player.def) 0		;# ���դ������MIDI�ץ졼�� playmidi_list��
				 # �����β����ܤ�

##### �ƥ����ȥ��������åȤ��Ѥ��������ե���Ȥ�����
set conf(textFontOpt) {}	;# �ե���ȥ��ץ������Ǽ���� ����ͤ϶�
switch $tcl_platform(platform) "unix" { # UNIX�ξ��
	if {[info tclversion] >= 8} { # Tcl8.0�ʹ�
		if {![catch {font create text16 \
		  -compound {8x16 kanji16}}]} {
			set conf(textFontOpt) {-font text16}
			# ����ѥ���ɥե���Ⱥ��������������
			# �����Ȥ�
		}
	} else {
		set conf(textFontOpt) {-font 8x16 -kanjifont kanji16}
	}
} "windows" { # Windows�ξ��
	if {[info tclversion] >= 8} {
		set conf(textFontOpt) {-font {FixedSys 16}}
	}
}

############### ����ե�����졼������������

# �����ѿ�
set intl(srcFile) ""	;# �������Ф�Ƥ������ϥե�����
set intl(midiFile) ""	;# �嵭���Ф���MIDI�ե�����
set intl(readFd) ""	;# �¹���Υ��ޥ�ɤν����ɤ߽Ф���fd

# ʸ���󥳥ޥ�ɤ��������ܸ첽Tcl/Tk�ξ��kstring���Ǥʤ����string
# â��Tcl/Tk8.1�ʾ�Ϲ�ݲ�����Ƥ���kstring���ʤ��Ƥ����ܸ줬������
set intl(strCmd) string
if {[info tclversion] >= 8.1} {
	# ��ݲ���
} elseif {[info commands kstring] != ""} {
	# ���ܸ첽Tcl/Tk��ʸ���󥳥ޥ�ɤȤ��Ƥ�kstring��Ȥ�
	set intl(strCmd) kstring
} else {
	# conf(Japanese)�����Ǥ�Ѹ�⡼�ɤ˶����ѹ�
	if {$conf(Japanese) > 0} {set conf(Japanese) 0}
}

proc strUncomment {vname} { # ʸ�����ꥹ�Ȥι�Ƭ�Ρ�#�פ�������ޤǤ���
	upvar $vname s
	while {[regsub "(^|\n)\[ \t\]*#\[^\n\]*(\n|\$)" $s {\1\2} s]} {}
}
# conf(playmidi_list)��˹�Ƭ�˸¤ꥳ���Ȥ��񤱤�褦��
strUncomment conf(playmidi_list)

proc msgSel {j e} { # ��å���������������
	global conf

	if {$conf(Japanese)} {return $j} else {return $e}
}

proc mml2mid_mkOpt {} { # mml2mid�ؤΥ��ץ������äƥꥹ�Ȥ��֤�
	global conf

	set optlist {}
	if {!$conf(outFmt.cur)} {lappend optlist "-f"}
	if {$conf(revParen.cur)} {lappend optlist "-v"}
	if {$conf(revAngbr.cur)} {lappend optlist "-x"}
	if {$conf(transp.cur)} {lappend optlist "-t$conf(transp.cur)"}
	return $optlist
}

proc quitWithConfirm {} { # ������ȴ���뤫��ǧ�� yes�ʤ�exit
	if {[tk_dialog .confirm Confirm [
		msgSel "��λ���ޤ���?" "Really quit?"
	] "" 1 Yes No] == 0} exit
}

proc openMMLFile {} {
	# ������̤ǥե���������򤵤���ɽ�������srcFile�ؤΥ��å�
	# ���Ф�ʤ���Ф��Τޤ�return
	global intl

	# Windows��tk8.0�������õ���ǥ��쥯�ȥ����¸���Ƥ���ʤ�?
	if {[string compare $intl(srcFile) ""] == 0} {
		set iniDir [pwd]
	} elseif {[file isdirectory $intl(srcFile)]} {
		set iniDir $intl(srcFile)
	} else {
		set iniDir [file dirname $intl(srcFile)]
	}

	set select [tk_getOpenFile -initialdir $iniDir \
		-filetypes {{MML .mml} {MIDI .mid} {ALL "*"}}]
	if {[string compare $select ""] == 0} return	;# ���Ф�ʤ��ä�

	if {[info tclversion] >= 8} {
		set select [file nativename $select]
	}
	selectMMLFile $select
}
proc selectMMLFile {fnm} { # ���򤵤줿�ե������ɽ�������srcFile�ؤΥ��å�
	global intl

	set intl(srcFile) $fnm		;# ���Ф줿��Τ�srcFile�˥��å�
	.entry.filename xview moveto 1	;# ���Ф줿�ե�����̾��ɽ���ϰϤ�ü��
}

proc fileNameUnspecialize {fnm} {
	# ���ץ����Ȳ�ᤵ��ʤ��褦fnm����Ƭ��ɬ�פʤ��./�פ��ղä��֤�
	# ���ߤΤȤ���open�ط��Υȥ�֥��ɻߤΤ����<�פʤɤǻϤޤ�����ˤ�
	# ���ΰ����򤹤�
	global intl

	if {[$intl(strCmd) match {[-2<>&|]*} $fnm]} {
		file join [file dirname ""] $fnm
	} else {
		return $fnm
	}
}

proc editMML {} { # ���߻��ꤵ��Ƥ���MML�ե�����������ǥ������Խ�
	global conf intl

	# �ե����뤬�ޤ����Ф�Ƥʤ���Х��顼ɽ�����������
	if {[string compare $intl(srcFile) ""] == 0} {
		dispErrMsg [
			msgSel "�ե����뤬�ޤ����򤵤�Ƥ��ޤ���" \
				"Input file is not specified yet"
		]
		return
	}

	set mmlf [fileNameUnspecialize $intl(srcFile)]
	if {[catch {eval exec "$conf(editor) [list $mmlf] &"} msg]} {
		dispErrMsg $msg
	}
}

proc isaMIDIFile {fnm} { # fnm���ɤ߲ĤǤ���MIDI�ե�����Ǥ���п����֤�
	set MIDIFileMagic "MThd"

	if {[catch {open [fileNameUnspecialize $fnm]} readFd]} {return 0}
	fconfigure $readFd -translation binary
	fconfigure $readFd -eofchar ""
	set rslt [expr [string compare $MIDIFileMagic [read $readFd 4]] == 0]
	catch {close $readFd}
	return $rslt
}
proc fnewer {f1 f2} { # ��1�����Υե�����������������ʤ鿿���֤�
	if {![file exists $f1] || ![file exists $f2]} {
		# �ɤä�������¸�ߤʤ鵶 ���λ���file mtime��ɾ�����ƤϤ���
		return 0
	} else {
		expr [file mtime $f1] > [file mtime $f2]
	}
}
proc toMIDIFileName {fnm} { # �б�����MIDI�ե�����̾���ä��֤�
	return "[file root $fnm].mid"
}

proc listNormalize {list} { # �ꥹ�Ȥ����Ƥ��Ѥ����������������֤�
	set ret {}
	foreach i $list {lappend ret $i}
	return $ret
}
proc forceCompileMML {} {
	# MML�ե�����򥳥�ѥ��� .mid�������������Ƥ⥳��ѥ������
	compileMML 1
}
proc compileMML {{forceFlg 0}} { # ���߻��ꤵ��Ƥ���MML�ե�����򥳥�ѥ���
	global conf intl tcl_platform

	# �ե����뤬�ޤ����Ф�Ƥʤ���Х��顼ɽ�����������
	if {[string compare $intl(srcFile) ""] == 0} {
		dispErrMsg [
			msgSel "�ե����뤬�ޤ����򤵤�Ƥ��ޤ���" \
				"Input file is not specified yet"
		]
		return
	}

	if {[isaMIDIFile $intl(srcFile)]} {
		# MIDI�ե�����Ǥ��ä���ñ�˱��� ���ξ��ϡ֥���ѥ����
		# ���աפ����꤬�ʤ��Ƥ���դ���
		playMIDI $intl(srcFile) 1
	} else {
		# �ޤ�����������õ�
		forceModify .display.dialog delete 1.0 end

		# MML����С����Υ��ޥ�ɥ饤��Υꥹ�Ȥ�cmdLine�˺��
		set mmlf [fileNameUnspecialize $intl(srcFile)]
		set intl(midiFile) [toMIDIFileName $mmlf]
		if {!$forceFlg && [fnewer $intl(midiFile) $mmlf]} {
			# MIDI�ե������������������Х���ѥ���Ϥ�������
			forceModify .display.dialog insert end [msgSel \
			  "$intl(midiFile) ���������������ᥳ��ѥ���ϹԤ��ޤ���\n" \
			  "$intl(midiFile) is newer; not compiled\n"
			] bluegreen
			playMIDI $intl(midiFile)
			return
		}
		set cmdLine "$conf(mml2mid) [mml2mid_mkOpt] \
			[list $mmlf $intl(midiFile)]"

		# windows�ξ��stderr����stdout�ؤΥ�����쥯�ȤϤǤ��ʤ�?
		if {[string compare $tcl_platform(platform) windows] == 0} {
			set actualCmdLine $cmdLine
		} else {
			set actualCmdLine "$cmdLine 2>@ stdout"
		}

		if {[catch {eval open {"|$actualCmdLine"} r} stat]} {
			# ���ޥ�ɼ¹Լ���
			dispErrMsg $stat
			return
		}
		set intl(readFd) $stat	;# ���ޥ�ɤ���ν��Ϥ�readFd�����
		# ���ޥ�ɥ饤�����ɽ��
		forceModify .display.dialog insert end \
			"[listNormalize $cmdLine]\n" blue
		# readFd�ؤν��Ϥ���Х����������ɲä����褦
		fileevent $intl(readFd) readable addDialog

		# ���ʱ�Compile�ܥ����Inturrupt�ܥ�����ѹ�
		.entry.compile configure -text [msgSel "���" "Inturrupt"] \
			-command intrCompiling
		# �����ߥ���������ߵ�ǽ�˥Х����
		bind . <<Cancel>> intrCompiling
		# ��˥塼��Compile���ޥ�ɤ���������ǽ��
		.menubar.file.menu entryconfigure $intl(cmpBtnLbl) \
			-state disabled
		.menubar.file.menu entryconfigure $intl(cmpForceBtnLbl) \
			-state disabled
	}
}

proc endCompiling {{intrFlg 0}} {
	# MML����ѥ��뽪λ/���ǻ��˸ƤФ��
	# intrFlg�ϳ��������Ǥ��ɤ����Υե饰
	global intl

	# readFd�򥯥������Ƥ��� ���顼�ʤ�MML����ѥ��餬���顼��λ���Ƥ�
	set cmpErr [catch {close $intl(readFd)}]

	# Inturrupt�ܥ����Compile�� �����ߥ����ΥХ���ɤ���
	# ��˥塼��Compile������Ĥ��᤹
	.entry.compile configure -text [msgSel "����ѥ���" "Compile"] \
		-command compileMML
	bind . <<Cancel>> {}
	.menubar.file.menu entryconfigure $intl(cmpBtnLbl) -state normal
	.menubar.file.menu entryconfigure $intl(cmpForceBtnLbl) -state normal

	if {$cmpErr && !$intrFlg} { # ���顼��λ
		forceModify .display.dialog insert end \
			[msgSel "���顼��λ\n" "Failed\n"] red
	} elseif {$intrFlg} { # ���ǽ�λ
		forceModify .display.dialog insert end \
			[msgSel "���ǽ�λ\n" "Inturrupt\n"] red
	} else { # ���ｪλ ���ճ���
		playMIDI $intl(midiFile)
	}
}
proc intrCompiling {} { # ����ѥ���ؤγ����߻��˸ƤФ��
	global intl tcl_platform

	if {[string compare $tcl_platform(platform) unix] == 0} {
		# UNIX�ˤ�kill���ޥ�ɤ�����
		exec kill [pid $intl(readFd)]
	}
	endCompiling 1
}

proc playMIDI {midif {forceFlg 0}} { # MIDI�ץ졼���Ƥ�Ǳ��ճ���
	global conf

	# �֥���ѥ������աפ����ꤵ��Ƥʤ���硢�������եե饰��
	# ���ꤵ��Ƥʤ��¤겿�⤻�����
	if {!$conf(doPlay.cur) && !$forceFlg} return

	# player.cur��playmidi_list�β����ܤ�ץ졼��Ȥ��ƻȤ��������äƤ���
	set player [lindex [lindex $conf(playmidi_list) $conf(player.cur)] 1]
	if {[catch {eval exec "$player [list $midif] &"} msg]} {
		dispErrMsg $msg
		return
	}
}

proc forceModify {dialog args} {
	# �����Խ��ԲĤˤ��Ƥ���������������Խ����Խ��ԲĤ��᤹
	$dialog configure -state normal
	eval "[list $dialog] $args"
	$dialog configure -state disabled
}
proc addDialog {} {
	# readFd�ؤν��Ϥ����ä������������������ɲä��뤿��˸ƤФ��
	global intl

	if {[gets $intl(readFd) line] == -1} {
		# ���Ͻ�λ ���ޥ�ɽ�λ�����Ƥ�
		endCompiling
	} else {
		forceModify .display.dialog insert end "$line\n"
		.display.dialog see end
	}
}

proc dispErrMsg {msg} { # ���顼��å�������ɽ������ǧ
	tk_messageBox -type ok -icon error -title "Error" -message $msg
}

proc www {} { # Web�֥饦����ư�����ꤷ���ڡ�����ɽ��
	global conf

	set cmd "$conf(webBrouser) [list $conf(mml2mid_www)]"
	if {[catch {eval "exec $cmd &"} msg]} {dispErrMsg $msg}
}

proc topwinno {} { # ���˺��٤��ȥåץ�٥륦����ɥ����̻��ʥ�Ф��֤�
	global intl

	if {[info exists intl(topwinno)]} {
		incr intl(topwinno)
	} else {
		set intl(topwinno) 0
	}
}
proc myMessage {name args} {
	# �����Υ�å������ܥå������� Tcl/Tk��message����Windows��Tk4.2����
	# �Ǥϡ�\n�פǤ��������Ԥ������ʤ�?�ΤǼ����
	set msglist {}; set anchor center
	while 1 {
		# -msglist�ǥ�å������Υꥹ�� -anchor�ǰ��ֹ�碌��Ϳ����
		# �����Ϻǽ��Ϳ�����Τ�����
		switch -- [lindex $args 0] "-msglist" {
			set msglist [lindex $args 1]
			set args [lreplace $args 0 1]
		} "-anchor" {
			set anchor [lindex $args 1]
			set args [lreplace $args 0 1]
		} default {
			break
		}
	}

	# �ե졼�०�������å�����
	eval "frame [list $name] $args"
	# ��å������ꥹ�Ȥ����ꤵ��Ƥ���Ф��줾����٥륦�������åȤˤ���
	if {[string compare $msglist {}] == 0} return
	set num 0
	foreach s $msglist {
		set lname $name.line$num
		label $lname -text $s -pady 0
		pack $lname -side top -anchor $anchor
		incr num
	}
}
proc about {} {
	# ����ɽ�� ������ɥ�̾���֤�(�Ǥ⸽�߻ȤäƤʤ�)
	# conf(Japanese)��0�Ǥ� ���������Ϥޤ��Ѹ��å��������Ѱդ��Ƥ��ʤ�
	set msg0a {
		"mml2mid v5.30"
		"by"
	}		;# mml2mid�ΥС�������ư�������٤�?
	set msg0b {
		"���Ķǿ� (Monden Akito)"
		"ƣ�潨�� (Fujii Hideki)"
		"���ĵ��� (Kuroda Hisayasu)"
		"���о�Ƿ (Nide Naoyuki)"
	}
	set msg1 {
		"mml2mid�ϥե꡼���եȤǤ���"
		"���䤤��碌�ϲ���URL�ˤơ�"
		"WWW: http://hpc.jp/~mml2mid/"
	}

	# �ȥåץ�٥륦����ɥ�����
	set winno [topwinno]
	set owinname .msg$winno
	set uwinname $owinname.upper
	toplevel $owinname
	wm title $owinname "About mml2mid"
	frame $uwinname -borderwidth 5
	pack $uwinname -side top -expand 1
	# �������Ƥ⿿������褦��

	# about mml2mid: ��å�������Ⱦʬ
	frame $uwinname.msg0 -borderwidth 5
	pack $uwinname.msg0 -side top
	myMessage $uwinname.msg0.a -msglist $msg0a
	pack $uwinname.msg0.a -side top
	myMessage $uwinname.msg0.b -msglist $msg0b -anchor w
	pack $uwinname.msg0.b -side top
	# ��å�������Ⱦʬ
	myMessage $uwinname.msg1 -msglist $msg1 -borderwidth 8
	pack $uwinname.msg1 -side top

	# OK�ܥ���Ret��Esc�ˤ��Υ�����ɥ��ξõ��Х����
	set destroycmd "destroy $owinname"
	button $owinname.ok -text [msgSel "λ��" "OK"] -command $destroycmd \
		-pady 5
	pack $owinname.ok -side top
	bind $owinname <Return> $destroycmd
	bind $owinname <Escape> $destroycmd

	focus $owinname
	return $owinname
}
proc guide {} { # �إ�ײ��̤�ɽ��
	global conf

	if {[string compare $conf(viewDoc) ""] == 0} {
		# �Ȥ߹��ߥӥ塼��
		builtin_viewer $conf(docFile)
	} else {
		# �����ӥ塼��
		set cmd "$conf(viewDoc) [list $conf(docFile)]"
		if {[catch {eval "exec $cmd"} msg]} {
			# dispErrMsg $msg
		} ;# winhelp�Ϥʤ������顼��λ�����ɤ��֤�?
	}
}

proc builtin_viewer {fnm} {
	# �إ��ɽ�����Ȥ߹��ߥӥ塼�� ���ΤȤ����������Ϥʤ�
	# ������ɥ�̾���֤�(�Ǥ⸽�߻ȤäƤʤ�)
	global conf

	if {[catch {open [fileNameUnspecialize $fnm]} readFd]} {
		dispErrMsg $readFd
		return {}
	}

	# �ȥåץ�٥륦����ɥ�����
	set winno [topwinno]
	set winname .doc$winno
	toplevel $winname
	wm title $winname "mml2mid guide"

	# �ƥ����ȥ��������åȤȥ�������С����Ѱ�
	frame $winname.doc
	text $winname.doc.body -width 80 -height 25 -relief sunken \
		-yscrollcommand "$winname.doc.ysc set"
	catch {eval $winname.doc.body configure $conf(textFontOpt)}
	pack $winname.doc.body -side left -fill both -expand 1
	scrollbar $winname.doc.ysc -command "$winname.doc.body yview" \
		-orient vertical
	grid $winname.doc.body $winname.doc.ysc -sticky news
	grid rowconfigure $winname.doc 0 -weight 1
	grid columnconfigure $winname.doc 0 -weight 1

	# �����ץ󤷤��ɥ�����ȥե���������Ƥ������ɽ��
	while {[gets $readFd line] != -1} {
		$winname.doc.body insert end "$line\n"
	}
	catch {close $readFd}
	$winname.doc.body delete "end -1 chars"		;# �Ǹ�β��Ԥ����
	$winname.doc.body see 1.0
	$winname.doc.body configure -state disabled	;# �ѹ���ǽ��
	pack $winname.doc -side top -fill both -expand 1

	# ����dismiss�ܥ�����äƤ���
	set destroycmd "destroy $winname"
	button $winname.ok -text [msgSel "��λ" "dismiss"] -command $destroycmd
	pack $winname.ok -side top -pady 3

	focus $winname
	return $winname
}

proc openTmp {fnm} {
	# �ե�����fnm���뤿��ΰ���ե�����򥪡��ץ� fd�Ȱ���ե�����̾��
	# �ڥ����֤� ���Ԥ���������֤�
	global tcl_platform intl

	set fnm [fileNameUnspecialize $fnm]
	# ��ĥ�Ҥ��������ʬ�����
	if {[$intl(strCmd) match ".*" [file tail $fnm]]} {
		set base $fnm
	} else {
		set base [file rootname $fnm]
	}

	# ���θ���.tmp��Ĥʤ��ƥ����ץ󤷤Ƥߤ� ����ʤ�.tm0 .tm1 �Ĥ�
	# �Τ�UNIX�Ρ�14ʸ�����¡פ����θ^^;
	for {set i -1} {$i < 1000} {incr i} {
		if {$i < 0} {set j ""} else {set j $i}
		set ext [string range "tmp" 0 [expr 2 - [string length $j]]]
		set tmpfnm "$base.$ext$j"
		if {![catch {open $tmpfnm {WRONLY CREAT EXCL} 0644} retfd]} {
			return [list $retfd $tmpfnm]
		}
	}
	return {} 
}

proc readConf {toExt cnfFile {errFlg 0}} {
	# ����ե�������ɤ� errFlg�����ꤵ��Ƥ����
	# ����ե�������¸�߻��ˤϥ��顼�ˤʤ�
	global conf

	# ����ե�����򥪡��ץ�
	if {!$errFlg && ![file exist $cnfFile]} return
	if {[catch {open [fileNameUnspecialize $cnfFile]} confFd]} {
		dispErrMsg $confFd
		return
	}
	while {[gets $confFd line] != -1} {
		string trim line
		if {[string compare line ""] == 0} {continue}	;# ����
		# �Ԥ��ֹ��� [��]�פȤ��������Ǥʤ��ȥ��顼
		if {[regexp {^([A-Za-z_][A-Za-z_0-9]*)([ \t]+([^ \t]*))?$} \
		  $line dummy0 item dummy1 val] == 0} {
			dispErrMsg [
	  			msgSel "����ե�����Υ��顼: $line\n" \
					"Error in config file: $line\n"
	  		]
			break
		}
		# �Τ�ʤ����ܤϥ��顼
		if {![info exists conf($item.def)]} {
			dispErrMsg [
				msgSel "����ե������̤�Τι���: $line\n" \
					"Unknown item in config file: $line\n"
			]
			break
		}
		set conf($item$toExt) $val
	}
	catch {close $confFd}
}
proc saveConf {cnfFile fromExt} {
	# ����ե�����򥻡���
	global conf

	set tmpCnf [openTmp $cnfFile]	;# ����ե��������
	if {[string compare $tmpCnf {}] == 0} {	# ����ե������������
		dispErrMsg [msgSel "����ե������������" "Can't make tmpfile"]
		return
	}
	set confFd [lindex $tmpCnf 0]

	set t [expr [string length $fromExt] + 1]
	foreach i [array names conf "*$fromExt"] {
		set j [string range $i 0 [expr [string length $i] - $t]]
		puts $confFd "$j $conf($i)"
	}
	if {[catch {close $confFd} msg]} {
		dispErrMsg $msg
		return
	}

	# ����ե����������Υե�����̾��rename
	if {[catch {file rename -force [lindex $tmpCnf 1] $cnfFile}]} {
		dispErrMsg $msg
		return
	}
}
proc copyConf {toExt fromExt {delFlg 0}} {
	# conf���ͤΤ������ꥦ����ɥ����Ѥ����Τ� ���ߤ���conf(*.cur)����
	# �����conf(*.win)�� ���뤤�ϵո����� �ʤɤ˥��ԡ���Ԥ�
	global conf

	set t [expr [string length $fromExt] + 1]
	foreach i [array names conf "*$fromExt"] {
		set j [string range $i 0 [expr [string length $i] - $t]]
		set conf($j$toExt) $conf($i)
		# delFlg�λ��꤬����Х��ԡ����ѿ�������
		if {$delFlg} {unset conf($i)}
	}
}
proc confOpts {} { # ���ꥦ����ɥ�
	global conf
	
	# �ȥåץ�٥륦����ɥ����Ѱ�
	if {[catch {toplevel .config}]} { # ʣ���ĤϺ��ʤ�
		dispErrMsg [
			msgSel "���ꥦ����ɥ���ʣ�������ޤ���" \
				"No multiple configuration windows"
		]
		return
	}
	wm title .config "mml2mid configuration"
	set confwin .config.inner
	frame $confwin
	pack $confwin -side top -expand 1	;# �������Ƥ⿿������褦��

	# ���ѥ졼���亸���ޡ��������
	set sepcolor purple4
	set sepwidth 3
	set sepmarginThru 10
	set sepmarginAcross 3
	set lrmargin 12

	copyConf .win .cur
	# ������ˤ�conf(*.win)���Ѥ�� OK�򲡤��Ȥ������ͤ�
	# conf(*.cur)�˥��åȤ��᤹

	# ����Cancel��Ok�ʤɤΥܥ������������ꥦ����ɥ�������
	frame $confwin.main
	pack $confwin.main -side top
	frame $confwin.left	;# ��Ⱦʬ
	frame $confwin.right	;# ��Ⱦʬ
	frame $confwin.midsep -bg $sepcolor -width $sepwidth 
	pack $confwin.left -in $confwin.main -side left -anchor n
	pack $confwin.midsep -in $confwin.main -side left \
		-pady $sepmarginThru -fill y
	pack $confwin.right -in $confwin.main -side left -anchor n

	# MIDI�ե����ޥå�0/1���ڤ��ؤ�
	set outfmt $confwin.left.outfmt
	frame $outfmt
	pack $outfmt -side top -padx $lrmargin -anchor w
	label $outfmt.lbl \
		-text [msgSel "MIDI���ϥե��������" "MIDI output file format"]
	pack $outfmt.lbl -side top -padx 10
	frame $outfmt.main
	pack $outfmt.main -side top -anchor w
	radiobutton $outfmt.main.fmt0 -variable conf(outFmt.win) \
		-text "format 0" -value 0
	radiobutton $outfmt.main.fmt1 -variable conf(outFmt.win) \
		-text "format 1" -value 1
	pack $outfmt.main.fmt0 $outfmt.main.fmt1 -side top -anchor w

	# ���ο��楻�ѥ졼��
	frame $confwin.left.sep -bg $sepcolor -height $sepwidth
	pack $confwin.left.sep -side top \
		-pady $sepmarginAcross -padx $sepmarginThru -fill x

	# ��<�ס�>�פ��(�ס�)�פ�ȿž
	set revsym $confwin.left.revsym
	frame $revsym
	pack $revsym -side top -padx $lrmargin -anchor w
	checkbutton $revsym.revparen -variable conf(revParen.win) \
		-text [msgSel "��(�ס�)�פ�ȿž" "reverse '(' and ')'"]
	checkbutton $revsym.revangbr -variable conf(revAngbr.win) \
		-text [msgSel "��<�ס�>�פ�ȿž" "reverse '<' and '>'"]
	pack $revsym.revparen $revsym.revangbr -side top -anchor w

	# ��Ĵ
	set transp $confwin.right.transpose
	frame $transp
	pack $transp -side top -padx $lrmargin
	label $transp.lbl \
		-text [msgSel "��Ĵ(�̾�0)" "transpose (ordinally set to 0)"]
	pack $transp.lbl -side top
	scale $transp.scale -from -12 -to 12 -variable conf(transp.win) \
		-tickinterval 12 -showvalue true -orient horizontal -length 170
	pack $transp.scale -side top

	# ���ο��楻�ѥ졼��
	frame $confwin.right.sep -bg $sepcolor -height $sepwidth
	pack $confwin.right.sep -side top \
		-pady $sepmarginAcross -padx $sepmarginThru -fill x

	# ����ѥ������դ�����
	set doplay $confwin.right.playAftCmp
	frame $doplay
	pack $doplay -side top -padx $lrmargin -anchor w
	checkbutton $doplay.flg -variable conf(doPlay.win) \
		-text [msgSel "����ѥ����˱���" "Play after compile"]
	pack $doplay.flg -side top -anchor w
	frame $doplay.sub
	pack $doplay.sub -side top -anchor w -padx 10
	# �ץ졼��ꥹ�ȤΤ��줾��ˤĤ��ƥ饸���ܥ������
	for {set i 0} {$i < [llength $conf(playmidi_list)]} {incr i} {
		set j [lindex [lindex $conf(playmidi_list) $i] 0]
		if {$conf(Japanese)} {
			set j [lindex $j 0]
		} {
			set j [lindex $j 1]
		}
		radiobutton $doplay.sub.choice$i -text $j \
			-variable conf(player.win) -value $i
		pack $doplay.sub.choice$i -side top -anchor w
	}

	# ����Cancel��Ok�ʤɤΥܥ���
	frame $confwin.buttonsep -height 10
	pack $confwin.buttonsep -side top	;# �ᥤ�����Ȥ��Υ��
	set button $confwin.button
	frame $button
	pack $button -side top
	frame $button.frame0
	frame $button.frame1
	pack $button.frame0 $button.frame1 -side top -pady 3
	button $button.cancel -text [msgSel "�˴�" "Cancel"] -command {
		# Cancel���˴���λ
		destroy .config
	}
	button $button.ok -text [msgSel "Ŭ��" "Ok"] -command {
		# Ok�ĥ�����ɥ��������򸽺�ͭ��������إ��ԡ�
		copyConf .cur .win 1
		destroy .config
	}
	button $button.default -text [msgSel "ɸ����᤹" "Default"] -command {
		# Default�ĥǥե����������ɤ�Ǥ���
		copyConf .win .def
	}
	button $button.reread -text [msgSel "���ɤ߹���" "Reread"] -command {
		# Reread������ե������������ɤ�Ǥ���
		readConf .win $conf(cnfFile) 1
	}
	button $button.save -text [msgSel "�������¸" "Save"] -command {
		# Save�ĸ�������ɥ�������������ե�����˽񤭽Ф�
		if {[tk_dialog .confirm Confirm \
		  [msgSel "��¸���ޤ���?" "Really save?"] "" 1 Yes No] == 0} {
			saveConf $conf(cnfFile) .win
		}
	}
	pack $button.default $button.reread $button.save \
		-in $button.frame0 -side left -padx 20
	pack $button.cancel $button.ok \
		-in $button.frame1 -side left -padx 30

	focus $confwin
}

proc convUl {label ul} {
	# -underline���ץ����1ʸ������������褦�ˤ��뤿����Ѵ��ؿ�
	global intl

	if {![regexp {^-?[0-9]+$} $ul]} { # ul�������Ǥʤ�
		set uln [$intl(strCmd) first \
			[$intl(strCmd) tolower $ul] \
			[$intl(strCmd) tolower $label]]
		if {$uln >= 0} {set ul $uln} ;# label��ˤʤ���Ф��Τޤ�
	} elseif {$ul < 0} { # ul�����
		set ul [expr [$intl(strCmd) length $label] + $ul]
	}
	return $ul
}
proc mymenubutton {name args} {
	# menubutton��-underline��text���1ʸ���ǻ���Ǥ���褦�ˤ������
	while 1 { # -text -underline���ץ����Ϻǽ������Τ�����
		switch -- [lindex $args 0] "-text" {
			set text [lindex $args 1]
			set args [lreplace $args 0 1]
		} "-underline" {
			set ul [lindex $args 1]
			set args [lreplace $args 0 1]
		} default {
			break
		}
	}
	if {[info exist text] && [info exist ul]} {
		set ul [convUl $text $ul] ;# underline���ץ������Ѵ����Ƥ���
	}

	set opt {}
	if {[info exist text]} {lappend opt "-text" $text}
	if {[info exist ul]} {lappend opt "-underline" $ul}
	eval "[list menubutton $name] $opt $args"
}
proc addMenu {menu kind args} {
	# menu add��-underline��label���1ʸ���ǻ���Ǥ���褦�ˤ������
	while 1 { # -label -underline���ץ����Ϻǽ������Τ�����
		switch -- [lindex $args 0] "-label" {
			set label [lindex $args 1]
			set args [lreplace $args 0 1]
		} "-underline" {
			set ul [lindex $args 1]
			set args [lreplace $args 0 1]
		} default {
			break
		}
	}
	if {[info exist label] && [info exist ul]} {
		set ul [convUl $label $ul] ;# underline���ץ������Ѵ����Ƥ���
	}

	set opt {}
	if {[info exist label]} {lappend opt "-label" $label}
	if {[info exist ul]} {lappend opt "-underline" $ul}
	eval "[list $menu add $kind] $opt $args"
}
	
proc nomenubutton {name args} {
	# ���̤Υܥ���Τ褦�˿����񤦥�˥塼�ܥ������ȥ�å�
	# mymenubutton��Ƥ֤Τ�-underline���ץ������Ѵ���ʤ����
	global intl

	# -command -text -underline���ץ����Ϻǽ������Τ�����
	while 1 {
		switch -- [lindex $args 0] "-command" {
			set cmd [lindex $args 1]
			set args [lreplace $args 0 1]
		} "-text" {
			set text [lindex $args 1]
			set args [lreplace $args 0 1]
		} "-underline" {
			set ul [lindex $args 1]
			set args [lreplace $args 0 1]
		} default {
			break
		}
	}

	if {![info exist cmd]} {set cmd ""}
	set opt {}
	if {[info exist text]} {lappend opt "-text" $text}
	if {[info exist ul]} {lappend opt "-underline" $ul}

	# ��˥塼�ܥ������ ���̤Υܥ������˿����񤦤褦Ĵ��
	set dmymenu $name.dummy
	eval "mymenubutton [list $name] $opt $args [list -menu $dmymenu]"
	set top [winfo toplevel $name]
	set intl(btnPrsFlg.$name) 0

	# ��˥塼���ޥ����ȥ����Τɤ���ǵ�ư���줿���Τ������
	# 1:Acceralation key���ޥ����ܥ���1����˲����줿
	bind $top <Alt-KeyPress> "
		# intl(btnPrsFlg.$name)�������ɸ��Х���ɤ����ˤ��
		set intl(btnPrsFlg.$name) 1
		[bind $top <Alt-KeyPress>]	;# ɸ��Х����
	"
	bind $name <Button-1> "
		set intl(btnPrsFlg.$name) 0
	"

	menu $dmymenu -tearoff 0 -postcommand "
		if {\$intl(btnPrsFlg.$name)} {
			# ���硼�ȥ��åȥ������褿���������ޥ�ɼ¹�
			after idle {
				# ��˥塼�ܥ����Υ�������֤��᤹
				event generate $dmymenu <ButtonRelease-1>
				$cmd
			}
		} else { # �ޥ�������å����褿
			after idle {$dmymenu unpost}
			# ���ߡ���˥塼�Ͼõ� â���ܥ����<ButtonRelease-1>��
			# �˸����᤹
		}
	"

	bind $name <ButtonRelease-1> "
		# �ޥ����ܥ����Υ���� �ޥ�������˥塼�ܥ�����ʤ饳�ޥ�ɼ¹�
		if {\[string compare \[$name cget -state\] active\] == 0} {
			after idle {
				$cmd
			}
			# ��˥塼�ܥ����Υ�������֤��᤹
			event generate $name <Leave>
		}
	"
}

# Windows�ξ�硢������ץȤ�¸�ߥǥ��쥯�ȥ��PATH�κǸ���ղ�
if {[string compare $tcl_platform(platform) windows] == 0} {
	set env(PATH) "$env(PATH);[file dirname $argv0]"
}
# Cancel���٥�Ȥؤ�ʪ�����٥�Ȥ��б���������Ƥ���
switch $tcl_platform(platform) "unix" {
	event add <<Cancel>> <Control-c>
} "windows" {
	event add <<Cancel>> <Escape>
} "macintosh" { # �������Ǥ�Mac���б����Ƥ���櫓�ǤϤʤ�
	event add <<Cancel>> <Command-.>
}

# �ޤ��ǥե���������ͤ������ͤإ��ԡ�
copyConf .cur .def
readConf .cur $conf(cnfFile)

# ������ɥ��Υ����ȥ롦��ư�����������Ǿ�������
wm title . $conf(myname)
wm geometry . 560x400
wm minsize . 370 205
# ������ɥ�������٥���褿��quitWithConfirm�򵯤���
wm protocol . WM_DELETE_WINDOW quitWithConfirm

# ���־�Υ�˥塼�С�
if {[info tclversion] >= 8 &&
    [string compare $tcl_platform(platform) unix] != 0} {
	# Tcl8.0�ʹ����� ������UNIX�ǤϺ��ΤȤ���������? (������פ�
	# �ޥ�������å��������ʤ� Alt+S��OK)
	menu .menubar -tearoff 0
	. conf -menu .menubar

	frame .menubar.file
	addMenu .menubar cascade -label [msgSel "�ե�����(F)" "File"] \
		-underline F -menu .menubar.file.menu
	addMenu .menubar command -label [msgSel "����(S)" "Setting"] \
		-underline S -command confOpts
	frame .menubar.menuhelp
	addMenu .menubar cascade -label [msgSel "�إ��(H)" "Help"] \
		-underline H -menu .menubar.menuhelp.menu

	# Windows�Ѥ˥����ȥ�С��Υ�˥塼���Ѱդ��٤�������?
} else {
	frame .menubar -relief raised -borderwidth 2
	pack .menubar -side top -fill x

	mymenubutton .menubar.file -text [msgSel "�ե�����(F)" "File"] \
		-underline F -menu .menubar.file.menu 
	pack .menubar.file -side left
	nomenubutton .menubar.setting -text [msgSel "����(S)" "Setting"] \
		-underline S -command confOpts
	pack .menubar.setting -side left
	mymenubutton .menubar.menuhelp -text [msgSel "�إ��(H)" "Help"] \
		-underline H -menu .menubar.menuhelp.menu
	pack .menubar.menuhelp -side left
}

# ��˥塼�С���File��˥塼
# Open, Compile, Force compile, 1�Ķ��ڤä�Exit��ޤ�
menu .menubar.file.menu -tearoff 0
addMenu .menubar.file.menu command \
	-label [msgSel "����(O)" "Open"] -underline O -command openMMLFile
addMenu .menubar.file.menu command \
	-label [msgSel "�Խ�(E)" "Edit"] -underline E -command editMML
addMenu .menubar.file.menu command \
	-label [set intl(cmpBtnLbl) [msgSel "����ѥ���(C)" "Compile"]] \
	-underline C -command compileMML
addMenu .menubar.file.menu command \
	-label [set intl(cmpForceBtnLbl) \
		[msgSel "��������ѥ���(F)" "Force compile"]] \
	-underline F -command forceCompileMML
addMenu .menubar.file.menu separator
addMenu .menubar.file.menu command \
	-label [msgSel "��λ(X)" "Exit"] -underline X -command quitWithConfirm
# ������Setting��˥塼
# �Ǹ��Help��˥塼 guide(Help), Web page, About��ޤ�
menu .menubar.menuhelp.menu -tearoff 0
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2mid�Υإ��(H)" "mml2mid Help"] -underline H \
	-command guide
addMenu .menubar.menuhelp.menu separator
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2mid��Web�ڡ���(W)" "mml2mid Web page"] \
	-underline [msgSel -2 W] -command www
addMenu .menubar.menuhelp.menu separator
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2mid�ˤĤ���(A)" "About mml2mid"] -underline A \
	-command about

# ���β���MML�ե�����̾ɽ����Open, Compile�Υܥ����ޤ�frame
set entryColor springgreen1
frame .oentry -borderwidth 3
pack .oentry -side top -fill x
frame .entry -borderwidth 2 -bg $entryColor
pack .entry -side top -fill x -in .oentry

# MML�ե�����̾ɽ����Open, Compile�Υܥ���
label .entry.label -text [msgSel "MML�ե�����:" "MML file:"] -bg $entryColor
pack .entry.label -side left
frame .entry.commands
pack .entry.commands -side right
button .entry.open -text [msgSel "����" "Open"] -command openMMLFile
button .entry.edit -text [msgSel "�Խ�" "Edit"] -command editMML
button .entry.compile -text [msgSel "����ѥ���" "Compile"] -command compileMML
pack .entry.open .entry.edit .entry.compile -side left -in .entry.commands
entry .entry.filename -textvariable intl(srcFile) -relief sunken
	# srcFile���ͤ�ưȿ��
pack .entry.filename -side left -fill x -expand 1
frame .entry.pad -width 3 -bg $entryColor
pack .entry.pad -side left

# ���β��Υ�������ɽ����
frame .display
pack .display -side top -fill both -expand 1

# ��������ɽ�������Τȥ�������С�
text .display.dialog -relief sunken -yscrollcommand ".display.ysc set"
#	-xscrollcommand ".display.xsc set"
catch {eval .display.dialog configure $conf(textFontOpt)}
pack .display.dialog -side left -fill both -expand 1
scrollbar .display.ysc -command ".display.dialog yview" -orient vertical
grid .display.dialog .display.ysc -sticky news
# scrollbar .display.xsc -command ".display.dialog xview" -orient horizontal
# grid .display.xsc -sticky ew
grid rowconfigure .display 0 -weight 1
grid columnconfigure .display 0 -weight 1
.display.dialog tag configure blue -foreground blue
.display.dialog tag configure red -foreground red
.display.dialog tag configure bluegreen -foreground #008060
# �������������ʤ��Խ��Բ� �Խ��ˤ�forceModify��Ȥ�
.display.dialog configure -state disabled

# ���ޥ�ɥ饤��˥ե�����̾1�Ĥ����ꤵ�줿��ľ���˥���ѥ���/����
switch [llength $argv] 0 { # ����0��
	# empty
} 1 { # ����1��
	selectMMLFile [lindex $argv 0]
	compileMML
} default {
	dispErrMsg [
		msgSel "������ʣ������Ǥ��ޤ���" \
			"Can't specify 2 or more auguments."
	]
	exit
}
