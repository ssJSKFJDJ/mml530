#!/usr/bin/wish
# tkmml2mid v0.1d

############### コンフィギュレーション部

##### メッセージ類を日本語にするか 1:日本語 0:英語
set conf(Japanese) 1

##### 自分の名前
set conf(myname) tkmml2mid

##### 外部コマンドの選択  コマンド名のconfigurationは
##### オプション込みの形に統一するためみんなリストにしてある

# MML→MIDコンバータと(あれば)必須オプション
set conf(mml2mid) {mml2mid}

if {[string compare $tcl_platform(platform) windows] != 0} { # UNIX用設定
	# MIDIプレーヤの設定
	# 「{{{日本語説明} {英語説明}}	{コマンドライン}}」で1セット
	# 複数セットある場合最初のものがデフォルト
	set conf(playmidi_list) {
		# この中には注釈は行頭(or行頭の空白の直後)にしか書けない

		# TiMidity(Tcl/Tkインタフェース)
		{
			{
				{Timidity (Tcl/Tkインタフェース)}
				{Timidity (Tcl/Tk interface)}
			}
			{timidity -ik}
		}

		# Playmidi(Xインタフェースを前提)
		{
			{
				{Playmidi (X11インタフェース)}
				{Playmidi (X11 interface)}
			}
		      	{xplaymidi}
		}

		# TiMidity(XAWインタフェース)…新設
		{
			{
				{Timidity (XAWインタフェース)}
				{Timidity (XAW interface)}
			}
			{timidity -ia}
		}
		# 利用するTiMidityにTcl/Tk, XAWのどちらのインタフェースも
		# 組み込まれていない場合は要修正
	}
	# 初期設定ファイルには以上のうち何番目の項目が選ばれているかが書かれる

	# MMLファイル編集用エディタ
	set conf(editor) {emacs}
	# set conf(editor) {xemacs}
	# set conf(editor) {mule}
	# set conf(editor) {kterm -e vi}
		# ktermを起こしてその中でviを動かす

	# mml2mid WebPage閲覧用ブラウザ
	set conf(webBrouser) {firefox}
	# set conf(webBrouser) {mozilla}
	# set conf(webBrouser) {kterm -e lynx}
		# ktermを起こしてその中でlynxを動かす

	# ドキュメントファイル閲覧用コマンド 空にすると内部簡易ビューアを使う
	set conf(viewDoc) {}
	# set conf(viewDoc) {kterm -e less}
		# ktermを起こしてその中でlessを動かす
	# set conf(viewDoc) {sh -c {exec emacs "$1" -f view-mode} dummy}
		# emacsを起こしてviewモードでドキュメントを見る
} else { # Windows用設定
	# MIDIプレーヤの設定
	# 「{{{日本語説明} {英語説明}} {コマンドライン}}」で1セット
	# 複数セットある場合最初のものがデフォルト
	set conf(playmidi_list) {
		# この中には注釈は行頭(or行頭の空白の直後)にしか書けない

		# Windowsのメディアプレーヤ
		{
			{
				{メディアプレーヤ}
				{Media player}
			}
			{mplayer -play -close}
		}

		# 関連づけコマンド。startコマンドで起動
		{
			{
				{「.mid」に関連づけられたコマンド}
				{".mid"-related command}
			}
			{start}
		}
	}

	# MMLファイル編集用エディタ
	set conf(editor) {notepad}

	# mml2mid WebPage閲覧用ブラウザ
	set conf(webBrouser) {start}	;# .htmlに関連付けられているコマンド
	# set conf(webBrouser) {{c:\Program Files\Internet Explorer\iexplore}}
		# 関連づけに関わらず常にIEを使いたければ
		# ありかを自分で調べてここに書くこと

	# ヘルプ閲覧用コマンド
	set conf(viewDoc) {winhelp}	;# Windowsのヘルプ
	# set conf(viewDoc) {start}	;# .hlpに関連づけられているコマンド
}

##### mml2mid WebページURL
set conf(mml2mid_www) "http://hpc.jp/~mml2mid"

##### ドキュメントと設定ファイルのありか
if {[string compare $tcl_platform(platform) unix] == 0} { # UNIX用設定
	# ドキュメントファイル(テキスト)
	set conf(docFile) "/usr/local/lib/mml2mid/mml2mid.txt"

	# 初期設定ファイル
	set conf(cnfFile) [file join $env(HOME) ".tkmml2mid"]
		# ホームディレクトリ下
} else { # Windows用設定
	# ヘルプファイル
	set conf(docFile) [file join [file dirname $argv0] "mml2mid.hlp"]
		# 本ファイルと同じディレクトリの下

	# 初期設定ファイル
	set conf(cnfFile) [file join [file dirname $argv0] "tkmmlmid.rc"]
		# 本ファイルと同じディレクトリの下 これはNTではまずい?
}

##### 設定ファイルや設定ウィンドウで変更可能なもののデフォルト値
set conf(outFmt.def) 1		;# MIDIファイル出力format 0…format0 1…format1
set conf(revParen.def) 0	;# 「(」「)」を逆にするか 1…する
set conf(revAngbr.def) 0	;# 「<」「>」を逆にするか 1…する
set conf(transp.def) 0		;# 移調(半音単位) +…高い方へ
set conf(doPlay.def) 0		;# コンパイル後に演奏するか 1…する
set conf(player.def) 0		;# 演奏する場合のMIDIプレーヤ playmidi_listの
				 # うちの何番目か

##### テキストウィジェットで用いる等幅フォントの設定
set conf(textFontOpt) {}	;# フォントオプションを格納する 初期値は空
switch $tcl_platform(platform) "unix" { # UNIXの場合
	if {[info tclversion] >= 8} { # Tcl8.0以降
		if {![catch {font create text16 \
		  -compound {8x16 kanji16}}]} {
			set conf(textFontOpt) {-font text16}
			# コンパウンドフォント作成に成功すれば
			# それを使う
		}
	} else {
		set conf(textFontOpt) {-font 8x16 -kanjifont kanji16}
	}
} "windows" { # Windowsの場合
	if {[info tclversion] >= 8} {
		set conf(textFontOpt) {-font {FixedSys 16}}
	}
}

############### コンフィギュレーション部終わり

# 内部変数
set intl(srcFile) ""	;# 現在選ばれている入力ファイル
set intl(midiFile) ""	;# 上記に対するMIDIファイル
set intl(readFd) ""	;# 実行中のコマンドの出力読み出し用fd

# 文字列コマンドの選択。日本語化Tcl/Tkの場合kstring、でなければstring
# 但しTcl/Tk8.1以上は国際化されていてkstringがなくても日本語が扱える
set intl(strCmd) string
if {[info tclversion] >= 8.1} {
	# 国際化…
} elseif {[info commands kstring] != ""} {
	# 日本語化Tcl/Tk。文字列コマンドとしてはkstringを使う
	set intl(strCmd) kstring
} else {
	# conf(Japanese)が正でも英語モードに強制変更
	if {$conf(Japanese) > 0} {set conf(Japanese) 0}
}

proc strUncomment {vname} { # 文字列やリストの行頭の「#」から行末までを削除
	upvar $vname s
	while {[regsub "(^|\n)\[ \t\]*#\[^\n\]*(\n|\$)" $s {\1\2} s]} {}
}
# conf(playmidi_list)内に行頭に限りコメントが書けるように
strUncomment conf(playmidi_list)

proc msgSel {j e} { # メッセージの日英選択
	global conf

	if {$conf(Japanese)} {return $j} else {return $e}
}

proc mml2mid_mkOpt {} { # mml2midへのオプションを作ってリストで返す
	global conf

	set optlist {}
	if {!$conf(outFmt.cur)} {lappend optlist "-f"}
	if {$conf(revParen.cur)} {lappend optlist "-v"}
	if {$conf(revAngbr.cur)} {lappend optlist "-x"}
	if {$conf(transp.cur)} {lappend optlist "-t$conf(transp.cur)"}
	return $optlist
}

proc quitWithConfirm {} { # 本当に抜けるか確認し yesならexit
	if {[tk_dialog .confirm Confirm [
		msgSel "終了しますか?" "Really quit?"
	] "" 1 Yes No] == 0} exit
}

proc openMMLFile {} {
	# 選択画面でファイルを選択させ、表示およびsrcFileへのセット
	# 選ばれなければそのままreturn
	global intl

	# Windowsのtk8.0は前回の探索ディレクトリを保存してくれない?
	if {[string compare $intl(srcFile) ""] == 0} {
		set iniDir [pwd]
	} elseif {[file isdirectory $intl(srcFile)]} {
		set iniDir $intl(srcFile)
	} else {
		set iniDir [file dirname $intl(srcFile)]
	}

	set select [tk_getOpenFile -initialdir $iniDir \
		-filetypes {{MML .mml} {MIDI .mid} {ALL "*"}}]
	if {[string compare $select ""] == 0} return	;# 選ばれなかった

	if {[info tclversion] >= 8} {
		set select [file nativename $select]
	}
	selectMMLFile $select
}
proc selectMMLFile {fnm} { # 選択されたファイルの表示およびsrcFileへのセット
	global intl

	set intl(srcFile) $fnm		;# 選ばれたものをsrcFileにセット
	.entry.filename xview moveto 1	;# 選ばれたファイル名の表示範囲を左端へ
}

proc fileNameUnspecialize {fnm} {
	# オプションと解釈されないようfnmの先頭に必要なら「./」を付加し返す
	# 現在のところopen関係のトラブル防止のため「<」などで始まる引数にも
	# この扱いをする
	global intl

	if {[$intl(strCmd) match {[-2<>&|]*} $fnm]} {
		file join [file dirname ""] $fnm
	} else {
		return $fnm
	}
}

proc editMML {} { # 現在指定されているMMLファイルを外部エディタで編集
	global conf intl

	# ファイルがまだ選ばれてなければエラー表示だけで戻る
	if {[string compare $intl(srcFile) ""] == 0} {
		dispErrMsg [
			msgSel "ファイルがまだ選択されていません" \
				"Input file is not specified yet"
		]
		return
	}

	set mmlf [fileNameUnspecialize $intl(srcFile)]
	if {[catch {eval exec "$conf(editor) [list $mmlf] &"} msg]} {
		dispErrMsg $msg
	}
}

proc isaMIDIFile {fnm} { # fnmが読み可でかつMIDIファイルであれば真を返す
	set MIDIFileMagic "MThd"

	if {[catch {open [fileNameUnspecialize $fnm]} readFd]} {return 0}
	fconfigure $readFd -translation binary
	fconfigure $readFd -eofchar ""
	set rslt [expr [string compare $MIDIFileMagic [read $readFd 4]] == 0]
	catch {close $readFd}
	return $rslt
}
proc fnewer {f1 f2} { # 第1引数のファイルの方が新しいなら真を返す
	if {![file exists $f1] || ![file exists $f2]} {
		# どっちかが非存在なら偽 この時はfile mtimeを評価してはだめ
		return 0
	} else {
		expr [file mtime $f1] > [file mtime $f2]
	}
}
proc toMIDIFileName {fnm} { # 対応するMIDIファイル名を作って返す
	return "[file root $fnm].mid"
}

proc listNormalize {list} { # リストを内容を変えずに正規化して返す
	set ret {}
	foreach i $list {lappend ret $i}
	return $ret
}
proc forceCompileMML {} {
	# MMLファイルをコンパイル .midの方が新しくてもコンパイルを強制
	compileMML 1
}
proc compileMML {{forceFlg 0}} { # 現在指定されているMMLファイルをコンパイル
	global conf intl tcl_platform

	# ファイルがまだ選ばれてなければエラー表示だけで戻る
	if {[string compare $intl(srcFile) ""] == 0} {
		dispErrMsg [
			msgSel "ファイルがまだ選択されていません" \
				"Input file is not specified yet"
		]
		return
	}

	if {[isaMIDIFile $intl(srcFile)]} {
		# MIDIファイルであったら単に演奏 この場合は「コンパイル後
		# 演奏」の設定がなくても演奏する
		playMIDI $intl(srcFile) 1
	} else {
		# まずダイアログを消去
		forceModify .display.dialog delete 1.0 end

		# MMLコンバータのコマンドラインのリストをcmdLineに作る
		set mmlf [fileNameUnspecialize $intl(srcFile)]
		set intl(midiFile) [toMIDIFileName $mmlf]
		if {!$forceFlg && [fnewer $intl(midiFile) $mmlf]} {
			# MIDIファイルの方が新しければコンパイルはせず演奏
			forceModify .display.dialog insert end [msgSel \
			  "$intl(midiFile) の方が新しいためコンパイルは行いません\n" \
			  "$intl(midiFile) is newer; not compiled\n"
			] bluegreen
			playMIDI $intl(midiFile)
			return
		}
		set cmdLine "$conf(mml2mid) [mml2mid_mkOpt] \
			[list $mmlf $intl(midiFile)]"

		# windowsの場合stderrからstdoutへのリダイレクトはできない?
		if {[string compare $tcl_platform(platform) windows] == 0} {
			set actualCmdLine $cmdLine
		} else {
			set actualCmdLine "$cmdLine 2>@ stdout"
		}

		if {[catch {eval open {"|$actualCmdLine"} r} stat]} {
			# コマンド実行失敗
			dispErrMsg $stat
			return
		}
		set intl(readFd) $stat	;# コマンドからの出力がreadFdへ来る
		# コマンドラインを青表示
		forceModify .display.dialog insert end \
			"[listNormalize $cmdLine]\n" blue
		# readFdへの出力あればダイアログに追加されるよう
		fileevent $intl(readFd) readable addDialog

		# 中段右CompileボタンをInturruptボタンに変更
		.entry.compile configure -text [msgSel "中止" "Inturrupt"] \
			-command intrCompiling
		# 割り込みキーも割り込み機能にバインド
		bind . <<Cancel>> intrCompiling
		# メニューのCompileコマンドも選択一時不能に
		.menubar.file.menu entryconfigure $intl(cmpBtnLbl) \
			-state disabled
		.menubar.file.menu entryconfigure $intl(cmpForceBtnLbl) \
			-state disabled
	}
}

proc endCompiling {{intrFlg 0}} {
	# MMLコンパイル終了/中断時に呼ばれる
	# intrFlgは割り込み中断かどうかのフラグ
	global intl

	# readFdをクローズしておく エラーならMMLコンパイラがエラー終了してる
	set cmpErr [catch {close $intl(readFd)}]

	# InturruptボタンをCompileに 割り込みキーのバインドも削除
	# メニューのCompileも選択可に戻す
	.entry.compile configure -text [msgSel "コンパイル" "Compile"] \
		-command compileMML
	bind . <<Cancel>> {}
	.menubar.file.menu entryconfigure $intl(cmpBtnLbl) -state normal
	.menubar.file.menu entryconfigure $intl(cmpForceBtnLbl) -state normal

	if {$cmpErr && !$intrFlg} { # エラー終了
		forceModify .display.dialog insert end \
			[msgSel "エラー終了\n" "Failed\n"] red
	} elseif {$intrFlg} { # 中断終了
		forceModify .display.dialog insert end \
			[msgSel "中断終了\n" "Inturrupt\n"] red
	} else { # 正常終了 演奏開始
		playMIDI $intl(midiFile)
	}
}
proc intrCompiling {} { # コンパイルへの割り込み時に呼ばれる
	global intl tcl_platform

	if {[string compare $tcl_platform(platform) unix] == 0} {
		# UNIXにはkillコマンドがある
		exec kill [pid $intl(readFd)]
	}
	endCompiling 1
}

proc playMIDI {midif {forceFlg 0}} { # MIDIプレーヤを呼んで演奏開始
	global conf

	# 「コンパイル後演奏」に設定されてない場合、強制演奏フラグが
	# 指定されてない限り何もせず戻る
	if {!$conf(doPlay.cur) && !$forceFlg} return

	# player.curにplaymidi_listの何番目をプレーヤとして使うかが入っている
	set player [lindex [lindex $conf(playmidi_list) $conf(player.cur)] 1]
	if {[catch {eval exec "$player [list $midif] &"} msg]} {
		dispErrMsg $msg
		return
	}
}

proc forceModify {dialog args} {
	# 普段編集不可にしてあるダイアログを強制編集し編集不可に戻す
	$dialog configure -state normal
	eval "[list $dialog] $args"
	$dialog configure -state disabled
}
proc addDialog {} {
	# readFdへの出力があった時それをダイアログに追加するために呼ばれる
	global intl

	if {[gets $intl(readFd) line] == -1} {
		# 出力終了 コマンド終了処理呼ぶ
		endCompiling
	} else {
		forceModify .display.dialog insert end "$line\n"
		.display.dialog see end
	}
}

proc dispErrMsg {msg} { # エラーメッセージの表示・確認
	tk_messageBox -type ok -icon error -title "Error" -message $msg
}

proc www {} { # Webブラウザを起動し指定したページを表示
	global conf

	set cmd "$conf(webBrouser) [list $conf(mml2mid_www)]"
	if {[catch {eval "exec $cmd &"} msg]} {dispErrMsg $msg}
}

proc topwinno {} { # 次に作るべきトップレベルウィンドウの通算ナンバを返す
	global intl

	if {[info exists intl(topwinno)]} {
		incr intl(topwinno)
	} else {
		set intl(topwinno) 0
	}
}
proc myMessage {name args} {
	# 自前のメッセージボックスを作る Tcl/TkのmessageだとWindowsのTk4.2以前
	# では「\n」での明示改行が効かない?ので自作した
	set msglist {}; set anchor center
	while 1 {
		# -msglistでメッセージのリスト -anchorで位置合わせを与える
		# これらは最初に与えるものと前提
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

	# フレームウィジェット生成
	eval "frame [list $name] $args"
	# メッセージリストが指定されていればそれぞれをラベルウィジェットにする
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
	# 情報表示 ウィンドウ名を返す(でも現在使ってない)
	# conf(Japanese)が0でも ここだけはまだ英語メッセージを用意していない
	set msg0a {
		"mml2mid v5.30"
		"by"
	}		;# mml2midのバージョンを自動取得すべき?
	set msg0b {
		"門田暁人 (Monden Akito)"
		"藤井秀樹 (Fujii Hideki)"
		"黒田久泰 (Kuroda Hisayasu)"
		"新出尚之 (Nide Naoyuki)"
	}
	set msg1 {
		"mml2midはフリーソフトです。"
		"お問い合わせは下記URLにて。"
		"WWW: http://hpc.jp/~mml2mid/"
	}

	# トップレベルウィンドウを作る
	set winno [topwinno]
	set owinname .msg$winno
	set uwinname $owinname.upper
	toplevel $owinname
	wm title $owinname "About mml2mid"
	frame $uwinname -borderwidth 5
	pack $uwinname -side top -expand 1
	# 広げられても真中に来るように

	# about mml2mid: メッセージ上半分
	frame $uwinname.msg0 -borderwidth 5
	pack $uwinname.msg0 -side top
	myMessage $uwinname.msg0.a -msglist $msg0a
	pack $uwinname.msg0.a -side top
	myMessage $uwinname.msg0.b -msglist $msg0b -anchor w
	pack $uwinname.msg0.b -side top
	# メッセージ下半分
	myMessage $uwinname.msg1 -msglist $msg1 -borderwidth 8
	pack $uwinname.msg1 -side top

	# OKボタン・Ret・Escにこのウィンドウの消去をバインド
	set destroycmd "destroy $owinname"
	button $owinname.ok -text [msgSel "了解" "OK"] -command $destroycmd \
		-pady 5
	pack $owinname.ok -side top
	bind $owinname <Return> $destroycmd
	bind $owinname <Escape> $destroycmd

	focus $owinname
	return $owinname
}
proc guide {} { # ヘルプ画面の表示
	global conf

	if {[string compare $conf(viewDoc) ""] == 0} {
		# 組み込みビューア
		builtin_viewer $conf(docFile)
	} else {
		# 外部ビューア
		set cmd "$conf(viewDoc) [list $conf(docFile)]"
		if {[catch {eval "exec $cmd"} msg]} {
			# dispErrMsg $msg
		} ;# winhelpはなぜかエラー終了コードを返す?
	}
}

proc builtin_viewer {fnm} {
	# ヘルプ表示用組み込みビューア 今のところサーチ等はない
	# ウィンドウ名を返す(でも現在使ってない)
	global conf

	if {[catch {open [fileNameUnspecialize $fnm]} readFd]} {
		dispErrMsg $readFd
		return {}
	}

	# トップレベルウィンドウを作る
	set winno [topwinno]
	set winname .doc$winno
	toplevel $winname
	wm title $winname "mml2mid guide"

	# テキストウィジェットとスクロールバーを用意
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

	# オープンしたドキュメントファイルの内容を取り込み表示
	while {[gets $readFd line] != -1} {
		$winname.doc.body insert end "$line\n"
	}
	catch {close $readFd}
	$winname.doc.body delete "end -1 chars"		;# 最後の改行を除去
	$winname.doc.body see 1.0
	$winname.doc.body configure -state disabled	;# 変更不能に
	pack $winname.doc -side top -fill both -expand 1

	# 下にdismissボタンを作っておく
	set destroycmd "destroy $winname"
	button $winname.ok -text [msgSel "終了" "dismiss"] -command $destroycmd
	pack $winname.ok -side top -pady 3

	focus $winname
	return $winname
}

proc openTmp {fnm} {
	# ファイルfnmを作るための一時ファイルをオープンし fdと一時ファイル名の
	# ペアを返す 失敗したら空を返す
	global tcl_platform intl

	set fnm [fileNameUnspecialize $fnm]
	# 拡張子を除いた部分を求める
	if {[$intl(strCmd) match ".*" [file tail $fnm]]} {
		set base $fnm
	} else {
		set base [file rootname $fnm]
	}

	# その後ろに.tmpをつなげてオープンしてみる だめなら.tm0 .tm1 …を試す
	# 昔のUNIXの「14文字制限」は非考慮^^;
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
	# 設定ファイルを読む errFlgが指定されていると
	# 設定ファイル非存在時にはエラーになる
	global conf

	# 設定ファイルをオープン
	if {!$errFlg && ![file exist $cnfFile]} return
	if {[catch {open [fileNameUnspecialize $cnfFile]} confFd]} {
		dispErrMsg $confFd
		return
	}
	while {[gets $confFd line] != -1} {
		string trim line
		if {[string compare line ""] == 0} {continue}	;# 空行
		# 行が「項目 [値]」という形式でないとエラー
		if {[regexp {^([A-Za-z_][A-Za-z_0-9]*)([ \t]+([^ \t]*))?$} \
		  $line dummy0 item dummy1 val] == 0} {
			dispErrMsg [
	  			msgSel "設定ファイルのエラー: $line\n" \
					"Error in config file: $line\n"
	  		]
			break
		}
		# 知らない項目はエラー
		if {![info exists conf($item.def)]} {
			dispErrMsg [
				msgSel "設定ファイルに未知の項目: $line\n" \
					"Unknown item in config file: $line\n"
			]
			break
		}
		set conf($item$toExt) $val
	}
	catch {close $confFd}
}
proc saveConf {cnfFile fromExt} {
	# 設定ファイルをセーブ
	global conf

	set tmpCnf [openTmp $cnfFile]	;# 一時ファイルを作る
	if {[string compare $tmpCnf {}] == 0} {	# 一時ファイル作成失敗
		dispErrMsg [msgSel "一時ファイル作成失敗" "Can't make tmpfile"]
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

	# 一時ファイルを本来のファイル名にrename
	if {[catch {file rename -force [lindex $tmpCnf 1] $cnfFile}]} {
		dispErrMsg $msg
		return
	}
}
proc copyConf {toExt fromExt {delFlg 0}} {
	# confの値のうち設定ウィンドウで用いるものを 現在の値conf(*.cur)から
	# 一時値conf(*.win)へ あるいは逆向きへ などにコピーを行う
	global conf

	set t [expr [string length $fromExt] + 1]
	foreach i [array names conf "*$fromExt"] {
		set j [string range $i 0 [expr [string length $i] - $t]]
		set conf($j$toExt) $conf($i)
		# delFlgの指定があればコピー元変数は抹消
		if {$delFlg} {unset conf($i)}
	}
}
proc confOpts {} { # 設定ウィンドウ
	global conf
	
	# トップレベルウィンドウを用意
	if {[catch {toplevel .config}]} { # 複数個は作らない
		dispErrMsg [
			msgSel "設定ウィンドウは複数開けません" \
				"No multiple configuration windows"
		]
		return
	}
	wm title .config "mml2mid configuration"
	set confwin .config.inner
	frame $confwin
	pack $confwin -side top -expand 1	;# 広げられても真中に来るように

	# セパレータや左右マージンの量
	set sepcolor purple4
	set sepwidth 3
	set sepmarginThru 10
	set sepmarginAcross 3
	set lrmargin 12

	copyConf .win .cur
	# 設定中にはconf(*.win)が変わり OKを押すとそれらの値を
	# conf(*.cur)にセットし戻す

	# 下のCancel・Okなどのボタンを除く、設定ウィンドウの本体
	frame $confwin.main
	pack $confwin.main -side top
	frame $confwin.left	;# 左半分
	frame $confwin.right	;# 右半分
	frame $confwin.midsep -bg $sepcolor -width $sepwidth 
	pack $confwin.left -in $confwin.main -side left -anchor n
	pack $confwin.midsep -in $confwin.main -side left \
		-pady $sepmarginThru -fill y
	pack $confwin.right -in $confwin.main -side left -anchor n

	# MIDIフォーマット0/1の切り替え
	set outfmt $confwin.left.outfmt
	frame $outfmt
	pack $outfmt -side top -padx $lrmargin -anchor w
	label $outfmt.lbl \
		-text [msgSel "MIDI出力ファイル形式" "MIDI output file format"]
	pack $outfmt.lbl -side top -padx 10
	frame $outfmt.main
	pack $outfmt.main -side top -anchor w
	radiobutton $outfmt.main.fmt0 -variable conf(outFmt.win) \
		-text "format 0" -value 0
	radiobutton $outfmt.main.fmt1 -variable conf(outFmt.win) \
		-text "format 1" -value 1
	pack $outfmt.main.fmt0 $outfmt.main.fmt1 -side top -anchor w

	# 左の真中セパレータ
	frame $confwin.left.sep -bg $sepcolor -height $sepwidth
	pack $confwin.left.sep -side top \
		-pady $sepmarginAcross -padx $sepmarginThru -fill x

	# 「<」「>」や「(」「)」の反転
	set revsym $confwin.left.revsym
	frame $revsym
	pack $revsym -side top -padx $lrmargin -anchor w
	checkbutton $revsym.revparen -variable conf(revParen.win) \
		-text [msgSel "「(」「)」の反転" "reverse '(' and ')'"]
	checkbutton $revsym.revangbr -variable conf(revAngbr.win) \
		-text [msgSel "「<」「>」の反転" "reverse '<' and '>'"]
	pack $revsym.revparen $revsym.revangbr -side top -anchor w

	# 移調
	set transp $confwin.right.transpose
	frame $transp
	pack $transp -side top -padx $lrmargin
	label $transp.lbl \
		-text [msgSel "移調(通常0)" "transpose (ordinally set to 0)"]
	pack $transp.lbl -side top
	scale $transp.scale -from -12 -to 12 -variable conf(transp.win) \
		-tickinterval 12 -showvalue true -orient horizontal -length 170
	pack $transp.scale -side top

	# 右の真中セパレータ
	frame $confwin.right.sep -bg $sepcolor -height $sepwidth
	pack $confwin.right.sep -side top \
		-pady $sepmarginAcross -padx $sepmarginThru -fill x

	# コンパイル後演奏の設定
	set doplay $confwin.right.playAftCmp
	frame $doplay
	pack $doplay -side top -padx $lrmargin -anchor w
	checkbutton $doplay.flg -variable conf(doPlay.win) \
		-text [msgSel "コンパイル後に演奏" "Play after compile"]
	pack $doplay.flg -side top -anchor w
	frame $doplay.sub
	pack $doplay.sub -side top -anchor w -padx 10
	# プレーヤリストのそれぞれについてラジオボタンを作る
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

	# 下のCancel・Okなどのボタン
	frame $confwin.buttonsep -height 10
	pack $confwin.buttonsep -side top	;# メイン部とやや離す
	set button $confwin.button
	frame $button
	pack $button -side top
	frame $button.frame0
	frame $button.frame1
	pack $button.frame0 $button.frame1 -side top -pady 3
	button $button.cancel -text [msgSel "破棄" "Cancel"] -command {
		# Cancel…破棄終了
		destroy .config
	}
	button $button.ok -text [msgSel "適用" "Ok"] -command {
		# Ok…ウィンドウ上の設定を現在有効な設定へコピー
		copyConf .cur .win 1
		destroy .config
	}
	button $button.default -text [msgSel "標準に戻す" "Default"] -command {
		# Default…デフォルト設定を読んでくる
		copyConf .win .def
	}
	button $button.reread -text [msgSel "再読み込み" "Reread"] -command {
		# Reread…設定ファイルの設定を読んでくる
		readConf .win $conf(cnfFile) 1
	}
	button $button.save -text [msgSel "設定を保存" "Save"] -command {
		# Save…現ウィンドウ上の設定を設定ファイルに書き出し
		if {[tk_dialog .confirm Confirm \
		  [msgSel "保存しますか?" "Really save?"] "" 1 Yes No] == 0} {
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
	# -underlineオプションが1文字や負数も取れるようにするための変換関数
	global intl

	if {![regexp {^-?[0-9]+$} $ul]} { # ulが整数でない
		set uln [$intl(strCmd) first \
			[$intl(strCmd) tolower $ul] \
			[$intl(strCmd) tolower $label]]
		if {$uln >= 0} {set ul $uln} ;# label内になければそのまま
	} elseif {$ul < 0} { # ulが負数
		set ul [expr [$intl(strCmd) length $label] + $ul]
	}
	return $ul
}
proc mymenubutton {name args} {
	# menubuttonで-underlineをtext中の1文字で指定できるようにしたもの
	while 1 { # -text -underlineオプションは最初に来るものと前提
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
		set ul [convUl $text $ul] ;# underlineオプションを変換しておく
	}

	set opt {}
	if {[info exist text]} {lappend opt "-text" $text}
	if {[info exist ul]} {lappend opt "-underline" $ul}
	eval "[list menubutton $name] $opt $args"
}
proc addMenu {menu kind args} {
	# menu addで-underlineをlabel中の1文字で指定できるようにしたもの
	while 1 { # -label -underlineオプションは最初に来るものと前提
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
		set ul [convUl $label $ul] ;# underlineオプションを変換しておく
	}

	set opt {}
	if {[info exist label]} {lappend opt "-label" $label}
	if {[info exist ul]} {lappend opt "-underline" $ul}
	eval "[list $menu add $kind] $opt $args"
}
	
proc nomenubutton {name args} {
	# 普通のボタンのように振る舞うメニューボタンを作るトリック
	# mymenubuttonを呼ぶので-underlineオプションの変換もなされる
	global intl

	# -command -text -underlineオプションは最初に来るものと前提
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

	# メニューボタンを作り 普通のボタン風に振る舞うよう調整
	set dmymenu $name.dummy
	eval "mymenubutton [list $name] $opt $args [list -menu $dmymenu]"
	set top [winfo toplevel $name]
	set intl(btnPrsFlg.$name) 0

	# メニューがマウスとキーのどちらで起動されたか知る策を作る
	# 1:Acceralation keyがマウスボタン1より後に押された
	bind $top <Alt-KeyPress> "
		# intl(btnPrsFlg.$name)の設定を標準バインドより先にやる
		set intl(btnPrsFlg.$name) 1
		[bind $top <Alt-KeyPress>]	;# 標準バインド
	"
	bind $name <Button-1> "
		set intl(btnPrsFlg.$name) 0
	"

	menu $dmymenu -tearoff 0 -postcommand "
		if {\$intl(btnPrsFlg.$name)} {
			# ショートカットキーで来た。すぐコマンド実行
			after idle {
				# メニューボタンを離した状態に戻す
				event generate $dmymenu <ButtonRelease-1>
				$cmd
			}
		} else { # マウスクリックで来た
			after idle {$dmymenu unpost}
			# ダミーメニューは消去 但しボタンは<ButtonRelease-1>時
			# に元に戻す
		}
	"

	bind $name <ButtonRelease-1> "
		# マウスボタンを離した マウスがメニューボタン内ならコマンド実行
		if {\[string compare \[$name cget -state\] active\] == 0} {
			after idle {
				$cmd
			}
			# メニューボタンを離した状態に戻す
			event generate $name <Leave>
		}
	"
}

# Windowsの場合、スクリプトの存在ディレクトリをPATHの最後に付加
if {[string compare $tcl_platform(platform) windows] == 0} {
	set env(PATH) "$env(PATH);[file dirname $argv0]"
}
# Cancelイベントへの物理イベントの対応を準備しておく
switch $tcl_platform(platform) "unix" {
	event add <<Cancel>> <Control-c>
} "windows" {
	event add <<Cancel>> <Escape>
} "macintosh" { # 現時点ではMacに対応しているわけではない
	event add <<Cancel>> <Command-.>
}

# まずデフォルト設定値を現設定値へコピー
copyConf .cur .def
readConf .cur $conf(cnfFile)

# ウィンドウのタイトル・起動時サイズ・最小サイズ
wm title . $conf(myname)
wm geometry . 560x400
wm minsize . 370 205
# ウィンドウ削除イベント来たらquitWithConfirmを起こす
wm protocol . WM_DELETE_WINDOW quitWithConfirm

# 一番上のメニューバー
if {[info tclversion] >= 8 &&
    [string compare $tcl_platform(platform) unix] != 0} {
	# Tcl8.0以降専用 ただしUNIXでは今のところおかしい? (「設定」の
	# マウスクリックが効かない Alt+SはOK)
	menu .menubar -tearoff 0
	. conf -menu .menubar

	frame .menubar.file
	addMenu .menubar cascade -label [msgSel "ファイル(F)" "File"] \
		-underline F -menu .menubar.file.menu
	addMenu .menubar command -label [msgSel "設定(S)" "Setting"] \
		-underline S -command confOpts
	frame .menubar.menuhelp
	addMenu .menubar cascade -label [msgSel "ヘルプ(H)" "Help"] \
		-underline H -menu .menubar.menuhelp.menu

	# Windows用にタイトルバーのメニューも用意すべきだろうか?
} else {
	frame .menubar -relief raised -borderwidth 2
	pack .menubar -side top -fill x

	mymenubutton .menubar.file -text [msgSel "ファイル(F)" "File"] \
		-underline F -menu .menubar.file.menu 
	pack .menubar.file -side left
	nomenubutton .menubar.setting -text [msgSel "設定(S)" "Setting"] \
		-underline S -command confOpts
	pack .menubar.setting -side left
	mymenubutton .menubar.menuhelp -text [msgSel "ヘルプ(H)" "Help"] \
		-underline H -menu .menubar.menuhelp.menu
	pack .menubar.menuhelp -side left
}

# メニューバーのFileメニュー
# Open, Compile, Force compile, 1つ区切ってExitを含む
menu .menubar.file.menu -tearoff 0
addMenu .menubar.file.menu command \
	-label [msgSel "選択(O)" "Open"] -underline O -command openMMLFile
addMenu .menubar.file.menu command \
	-label [msgSel "編集(E)" "Edit"] -underline E -command editMML
addMenu .menubar.file.menu command \
	-label [set intl(cmpBtnLbl) [msgSel "コンパイル(C)" "Compile"]] \
	-underline C -command compileMML
addMenu .menubar.file.menu command \
	-label [set intl(cmpForceBtnLbl) \
		[msgSel "強制コンパイル(F)" "Force compile"]] \
	-underline F -command forceCompileMML
addMenu .menubar.file.menu separator
addMenu .menubar.file.menu command \
	-label [msgSel "終了(X)" "Exit"] -underline X -command quitWithConfirm
# 次いでSettingメニュー
# 最後にHelpメニュー guide(Help), Web page, Aboutを含む
menu .menubar.menuhelp.menu -tearoff 0
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2midのヘルプ(H)" "mml2mid Help"] -underline H \
	-command guide
addMenu .menubar.menuhelp.menu separator
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2midのWebページ(W)" "mml2mid Web page"] \
	-underline [msgSel -2 W] -command www
addMenu .menubar.menuhelp.menu separator
addMenu .menubar.menuhelp.menu command \
	-label [msgSel "mml2midについて(A)" "About mml2mid"] -underline A \
	-command about

# その下のMMLファイル名表示とOpen, Compileのボタンを含むframe
set entryColor springgreen1
frame .oentry -borderwidth 3
pack .oentry -side top -fill x
frame .entry -borderwidth 2 -bg $entryColor
pack .entry -side top -fill x -in .oentry

# MMLファイル名表示とOpen, Compileのボタン
label .entry.label -text [msgSel "MMLファイル:" "MML file:"] -bg $entryColor
pack .entry.label -side left
frame .entry.commands
pack .entry.commands -side right
button .entry.open -text [msgSel "選択" "Open"] -command openMMLFile
button .entry.edit -text [msgSel "編集" "Edit"] -command editMML
button .entry.compile -text [msgSel "コンパイル" "Compile"] -command compileMML
pack .entry.open .entry.edit .entry.compile -side left -in .entry.commands
entry .entry.filename -textvariable intl(srcFile) -relief sunken
	# srcFileの値を自動反映
pack .entry.filename -side left -fill x -expand 1
frame .entry.pad -width 3 -bg $entryColor
pack .entry.pad -side left

# その下のダイアログ表示部
frame .display
pack .display -side top -fill both -expand 1

# ダイアログ表示部本体とスクロールバー
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
# ダイアログは普段は編集不可 編集にはforceModifyを使う
.display.dialog configure -state disabled

# コマンドラインにファイル名1つが指定されたら直ちにコンパイル/演奏
switch [llength $argv] 0 { # 引数0個
	# empty
} 1 { # 引数1個
	selectMMLFile [lindex $argv 0]
	compileMML
} default {
	dispErrMsg [
		msgSel "引数は複数指定できません" \
			"Can't specify 2 or more auguments."
	]
	exit
}
