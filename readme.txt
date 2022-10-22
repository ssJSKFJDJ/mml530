
ファイルの説明


readme.txt      .... このファイル
header.txt        .... アップロード用ヘッダファイル

doc ディレクトリ  ... ドキュメント類

  00start.txt  初めてお使い頂く方への概要説明
  copyrigh.txt 著作権に関する注意
  chnglog.txt  バージョンアップ履歴
  compile.txt  MML2MIDソースプログラムのコンパイル方法
  mml2mid.txt  MML2MIDの使い方
  coolgs.txt   MML2MIDのテクニック集（GS音源用）
  coolxg.txt   MML2MIDのテクニック集（XG音源用）
  coolsg.txt   MML2MIDのテクニック集（XG音源+PLG100-SGボード用）
  command.txt  MMLコマンド早見表

  mmldef.txt   VZエディタユーザのための説明書
  mml2mid.def  VZエディタ用defファイル
  tr-rack.mml  KORG TR-RackユーザのためのサンプルExclusive

bin_dos ディレクトリ  ... DOS用実行可能ファイル

  mml2mid.exe  MML2MID本体（MS-DOS用実行ファイル）

bin_w32 ディレクトリ  ... Win32用実行可能ファイル

  mml2mid.exe  MML2MID本体（Win32コンソール版実行ファイル）

sample ディレクトリ .... サンプル音楽データ

  000readme.txt サンプル曲リスト
  *.mml    曲データ（MMLデータ）
  *.txt    曲データの説明
  *.mid    曲データ（標準MIDIファイル）

src ディレクトリ .... ソースプログラム

  makefile	…UNIX用makefile
  makefile.bcc	…BCC用makefile
  makefile.egc	…Win32上のegcs用makefile(Win32コンソール版を作成)
  charproc.c
  charproc.h
  file.asm
  file.c
  file.h
  mml2mid.1
  mml2mid.c
  mml2mid.h
  mmlproc.c
  mmlproc.h
  note.c
  win.h

mmlpp ディレクトリ .... mml2mid用プリプロセッサ

  mmlpp.pl     ... perlスクリプト
  mmlppdoc.txt ... プリプロセッサの説明書

tk ディレクトリ .... mml2mid用グラフィカル・インタフェースtkmml2mid

  tkmml2mid.tcl ... Tcl/Tkスクリプト
  tkmml2mid.txt ... tkmml2midの説明書

----------------------------------------------------
連絡先：

御意見，質問等は，下記のMML2MID Web Pageにあります問い合わせフォーム
までお寄せ下さい．

MML2MID Web Pageは， http://hpc.jp/~mml2mid/ です．なお、UNIX向けアー
カイブは、ftp://hayabusa.ics.nara-wu.ac.jp/pub/nide/misc/mml530b.tgz
でも配布しています。(アーカイブ形式がtar+gzip、ソースやドキュメントの文
字コードが日本語EUC、改行コードがLFになっている他は、収録内容はWindows
向けアーカイブと同じで、Windows用やDOS用のバイナリも同梱しています)
