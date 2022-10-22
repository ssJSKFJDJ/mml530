
# perlで書いた実験的なmml2mid用プリプロセッサ mmlpp.pl v1.03
# 現在のところ実現できているのは{長い名前の,引数を取る}マクロだけ
# by nide@ics.nara-wu.ac.jp

# v1.02 Modified by Takanori YOSHIMURA (tak-y@geocities.co.jp)
#  ピッチベンド値計算機能を付けた。
# v1.03 Modified by NIDE Naoyuki (nide@ics.nara-wu.ac.jp)
#  openする時のファイル名の先頭や末尾に空白や「|」などがある場合に対処

$REM = ';.*|/\*([^\*]|\*[^/])*\*?(\*/|$)';
	# コメントにマッチする正規表現。閉じてないのも含む(別途エラーチェック)
$STR = '"([^"\\\\\n]|\\\\(.|\n))*("|\n)';
	# 文字列(継続行考慮)にマッチする正規表現。閉じてないのも
	# 含む(別途エラーチェック)
#----
$BEND_RANGE= 2;
	# ベンドレンジ。デフォルトは2。
#----

sub eval_str{ # 文字列中の「\」を評価
	local($_) = $_[0];

	s/\\(x[0-9a-fA-F]{1,2}|[0-7]{1,3}|.)/
		$1 eq 'v' ? "\013" : eval('"' . $& . '"')/eg;
	 # perl4.036では「eval("\"$&\")」だとなぜかうまく行かない
	$_;
}

sub repl_sub{ # 注釈除去後の埋め草を返す。文字列はそのまま
	local($_) = $_[0];

	/^"/ ? (/\n$/ ? "\032\001" : $_) :
	       (/\n$/ ? "\032\002" : (y/\n//cd, $_))
	# "\032\001"は文字列が閉じてない、"\032\002"はコメントが
	# 閉じてないエラーを表す。あとでこれらが見つかればエラー終了
}

sub err{
	local($msg, $lineno, $fnm, $vfnm) = @_;

	$fnm = $vfnm if $vfnm ne '';
	$fnm = '(stdin)' if $fnm eq '-';
	die "$0: $msg in line $lineno" .
		($fnm eq '' ? '' : ", file '$fnm'") . "\n";
}

sub load{
	local($fnm, $incl) = @_;
	local($/, $_) = undef;
	local($tmp1, $tmp2, $vfnm, $lineno, @lines);

	$fnm = "./$fnm" if $fnm !~ /^\S/;	# '-'はquoteされない
	open(IN, "< $fnm\0") || die "$0: Can't " .
		($incl ? 'include' : 'open') . " '$fnm': $!\n";
	$_ = <IN>, close(IN);			# 全内容を読む

	y/\r//d; s/\032(.|\n)*$//;	# CR, ^Z以後除去(テキストモード
					# なら不要なはずだが念のため)
	s/$/$' eq '' ? "\n" : ''/e;	# 最後に改行なければ付加
	s/$REM|$STR/&repl_sub($&)/oeg;	# 注釈除去
	s/(.*\\\n)+(.*[^\\\n])?\n/
		$tmp2 = (($tmp1 = $&) =~ s|\\\n||g), $tmp1 . "\n" x $tmp2
	/eg;				# 継続行
	s/(^|\n)[ \t]+/$1/g;		# 行頭空白除去

	# この時点で注釈も継続行も行頭空白も失せている

	# perl4.036では「foreach(@lines=…)」だとなぜかうまく行かない
	# (もう1箇所も同じ)
	@lines = split(/\n/);
	foreach(@lines){
		$lineno++;
		if(/\032\001/){
			# 閉じてない文字列がそこにあった。エラー終了
			&err('String not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/\032\002/){
			# 閉じてないコメントがそこにあった。エラー終了
			&err('Comment not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/^#\s*(include|\d+)\s*($STR|)\s*$/o){
			$tmp2 = substr($2, 1, length($2) - 2);
			if($1 ne 'include'){	# 「# 数 "ファイル名"」
				$vfnm = $tmp2;
				$lineno = $1 - 1;
				$_ = sprintf("# %d \"%s\"", $1,
					$vfnm eq '' ? $fnm : $vfnm);
			} else {		# 「#include "ファイル名"」
				$_ = sprintf("# 1 \"%s\"\n%s# %d \"%s\"",
					$tmp2, &load(&eval_str($tmp2), 1),
					$lineno + 1,
					$vfnm eq '' ? $fnm : $vfnm);
			}
		}
		elsif(/^\$(\{\w+\}|\d?[a-z])/){	# マクロ定義行
			$tmp1 = $1, $tmp2 = $';
			$tmp1 =~ s/[^\w]//g;
			$tmp2 =~ s/^[ \t]+//;
			$macro{$tmp1} = $tmp2;	# マクロの定義を格納
			#----
			if($tmp1 eq "_bend_range_") # 予約マクロ
			{
				$BEND_RANGE= $tmp2;
			}
			#----
			$_ = '';
		}
	}

	# この時点で「#include」とマクロ定義行は失せており、閉じてない
	# 文字列やコメントのエラーチェックも済み

	@lines = split(/\n/, join("\n", @lines));
	$lineno = 0, $vfnm = '';
	foreach(@lines){
		$lineno++;
		if(/^#\s*(\d+)\s*($STR|)\s*$/o){
		 # 「# 数 "ファイル名"」の行。エラー表示用にこの情報が必要
			$vfnm = substr($2, 1, length($2) - 2);
			$lineno = $1 - 1;
		 	next;
		}
		elsif(!/^#/){	# マクロの展開
			$_ = &macroexp($_, $lineno, $fnm, $vfnm);
			#----
			$_ = &bendexp($_, $lineno, $fnm, $vfnm);
			#----
		}
	}

	join("\n", @lines) . "\n";
}

sub macroexp{ # 行中のマクロを展開して返す
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $mcvar, $mcval, @mcarg, $tmp);

	while(/\$(\{\w+[\}:]|\d?[a-z])|$STR/o){
		$_ = $';
		$ret .= $`;
		$ret .= $&, next if substr($&, 0, 1) ne '$'; # 文字列は非処理

		$mcvar = $1;	# マクロ名
		if($mcvar =~ /:$/){
			# マクロの引数を@mcargに得る
			@mcarg = &getarg($_, $lineno, $fnm, $vfnm);
			$_ = pop(@mcarg);
		} else {
			@mcarg = ();
		}
		# マクロの値を$mcvalに得る
		$mcvar =~ s/[^\w]//g;
		$mcval = $macro{$mcvar};
		&err("Undefined macro \$\{$mcvar\}", $lineno, $fnm, $vfnm)
			unless defined($mcval);
		# 引数つきマクロの場合は引数置き換え
		$mcval =~ s/#(\d|\{\d+\})|$STR/
			substr($&, 0, 1) ne '#' ? $& :
			  (($tmp = $1) =~ s|[^0-9]||g, $mcarg[$tmp - 1])
			# perl4.036では「\d」だとなぜかうまく行かない
		/ego;
		# マクロ内でマクロが使われていれば更に展開し、結果をつなげる
		$ret .= &macroexp($mcval, $lineno, $fnm, $vfnm);
	}
	$ret . $_;
}

sub getarg{
	# 行の先頭から「物」の「,」で区切られた列を取って配列とし、
	# 直後の「}」を剥いだ残りの行を最後の要素として配列に追加して返す。
	# 「物」とは以下のものの連なり: 「"{}\,」以外の文字、
	# 「\」+1字、文字列、および「{」+(物か「,」)の列+「}」
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($arg, $tmp, @ret, @tmp, $flg);

	while(/^(([^"\{\}\\,]|$STR)+|\\.|[\{,])/o){
		$flg = 1, $tmp = $&, $_ = $';
		push(@ret, $arg), $arg = '', next if $tmp eq ','; #「,」の場合
		$tmp =~ s/^\\//, $arg .= $tmp, next if $tmp ne '{';
		 # 「{」で始まるもの以外の場合。「\」があれば除去

		 # 「{」で始まるものの場合、中身を&getargで取得し繋げる
		@tmp = &getarg($_, $lineno, $fnm, $vfnm);
		$_ = pop(@tmp);
		$arg .= '{' . join(',', @tmp) . '}';
	}
	&err("Illegal macro usage", $lineno, $fnm, $vfnm) unless s/^\}//;
	push(@ret, $arg) if $flg;
	(@ret, $_);
}

# ベンド値マクロを展開して返す by YOSHIMURA
# i関数 : 0〜64〜127 (BSコマンド用)
# j関数 : -64〜0〜63 (F,Uコマンド用)
# ベンドレンジは予約マクロ${_bend_range_}にセットする。
sub bendexp{
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $val, $tmp);

	while(/([ij])(-?\d+)|$STR/o){
		$_ = $';
		$ret .= $`;
		$tmp= substr($&, 0, 1);
		if(($tmp ne 'i')&&($tmp ne 'j')){
			$ret .= $&;
			next; # 文字列は非処理
		}

		$val= 64*$2/$BEND_RANGE + 64;
		if($val>127) { $val= 127; }
		elsif($val<0) { $val= 0; }
		
		if($1 eq 'j')
		{
			$ret .= int($val+0.5)-64;
		}
		else
		{
			$ret .= int($val+0.5);
		}
	}
	$ret . $_;
}

if($#ARGV < 0){
	@ARGV = '-';
} elsif($#ARGV > 0){
	die "$0: Usage: [j]perl [-L{latin|euc|sjis}] $0 [MMLfile]\n";
}
print &load($ARGV[0], 0);
