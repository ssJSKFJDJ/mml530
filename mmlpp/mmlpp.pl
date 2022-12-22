
# perlで今いた悸赋弄なmml2mid脱プリプロセッサ mmlpp.pl v1.03
# 附哼のところ悸附できているのは{墓い叹涟の,苞眶を艰る}マクロだけ
# by nide@ics.nara-wu.ac.jp

# v1.02 Modified by Takanori YOSHIMURA (tak-y@geocities.co.jp)
#  ピッチベンド猛纷换怠墙を烧けた。
# v1.03 Modified by NIDE Naoyuki (nide@ics.nara-wu.ac.jp)
#  openする箕のファイル叹の黎片や琐萨に鄂球や≈|∽などがある眷圭に滦借

$REM = ';.*|/\*([^\*]|\*[^/])*\*?(\*/|$)';
	# コメントにマッチする赖惮山附。誓じてないのも崔む(侍庞エラ〖チェック)
$STR = '"([^"\\\\\n]|\\\\(.|\n))*("|\n)';
	# 矢机误(费鲁乖雇胃)にマッチする赖惮山附。誓じてないのも
	# 崔む(侍庞エラ〖チェック)
#----
$BEND_RANGE= 2;
	# ベンドレンジ。デフォルトは2。
#----

sub eval_str{ # 矢机误面の≈\∽を删擦
	local($_) = $_[0];

	s/\\(x[0-9a-fA-F]{1,2}|[0-7]{1,3}|.)/
		$1 eq 'v' ? "\013" : eval('"' . $& . '"')/eg;
	 # perl4.036では≈eval("\"$&\")∽だとなぜかうまく乖かない
	$_;
}

sub repl_sub{ # 庙坚近殿稿の虽め琉を手す。矢机误はそのまま
	local($_) = $_[0];

	/^"/ ? (/\n$/ ? "\032\001" : $_) :
	       (/\n$/ ? "\032\002" : (y/\n//cd, $_))
	# "\032\001"は矢机误が誓じてない、"\032\002"はコメントが
	# 誓じてないエラ〖を山す。あとでこれらが斧つかればエラ〖姜位
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
	$_ = <IN>, close(IN);			# 链柒推を粕む

	y/\r//d; s/\032(.|\n)*$//;	# CR, ^Z笆稿近殿(テキストモ〖ド
					# なら稍妥なはずだが前のため)
	s/$/$' eq '' ? "\n" : ''/e;	# 呵稿に猖乖なければ烧裁
	s/$REM|$STR/&repl_sub($&)/oeg;	# 庙坚近殿
	s/(.*\\\n)+(.*[^\\\n])?\n/
		$tmp2 = (($tmp1 = $&) =~ s|\\\n||g), $tmp1 . "\n" x $tmp2
	/eg;				# 费鲁乖
	s/(^|\n)[ \t]+/$1/g;		# 乖片鄂球近殿

	# この箕爬で庙坚も费鲁乖も乖片鄂球も己せている

	# perl4.036では≈foreach(@lines=∧)∽だとなぜかうまく乖かない
	# (もう1舱疥も票じ)
	@lines = split(/\n/);
	foreach(@lines){
		$lineno++;
		if(/\032\001/){
			# 誓じてない矢机误がそこにあった。エラ〖姜位
			&err('String not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/\032\002/){
			# 誓じてないコメントがそこにあった。エラ〖姜位
			&err('Comment not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/^#\s*(include|\d+)\s*($STR|)\s*$/o){
			$tmp2 = substr($2, 1, length($2) - 2);
			if($1 ne 'include'){	# ≈# 眶 "ファイル叹"∽
				$vfnm = $tmp2;
				$lineno = $1 - 1;
				$_ = sprintf("# %d \"%s\"", $1,
					$vfnm eq '' ? $fnm : $vfnm);
			} else {		# ≈#include "ファイル叹"∽
				$_ = sprintf("# 1 \"%s\"\n%s# %d \"%s\"",
					$tmp2, &load(&eval_str($tmp2), 1),
					$lineno + 1,
					$vfnm eq '' ? $fnm : $vfnm);
			}
		}
		elsif(/^\$(\{\w+\}|\d?[a-z])/){	# マクロ年盗乖
			$tmp1 = $1, $tmp2 = $';
			$tmp1 =~ s/[^\w]//g;
			$tmp2 =~ s/^[ \t]+//;
			$macro{$tmp1} = $tmp2;	# マクロの年盗を呈羌
			#----
			if($tmp1 eq "_bend_range_") # 徒腆マクロ
			{
				$BEND_RANGE= $tmp2;
			}
			#----
			$_ = '';
		}
	}

	# この箕爬で≈#include∽とマクロ年盗乖は己せており、誓じてない
	# 矢机误やコメントのエラ〖チェックも貉み

	@lines = split(/\n/, join("\n", @lines));
	$lineno = 0, $vfnm = '';
	foreach(@lines){
		$lineno++;
		if(/^#\s*(\d+)\s*($STR|)\s*$/o){
		 # ≈# 眶 "ファイル叹"∽の乖。エラ〖山绩脱にこの攫鼠が涩妥
			$vfnm = substr($2, 1, length($2) - 2);
			$lineno = $1 - 1;
		 	next;
		}
		elsif(!/^#/){	# マクロの鸥倡
			$_ = &macroexp($_, $lineno, $fnm, $vfnm);
			#----
			$_ = &bendexp($_, $lineno, $fnm, $vfnm);
			#----
		}
	}

	join("\n", @lines) . "\n";
}

sub macroexp{ # 乖面のマクロを鸥倡して手す
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $mcvar, $mcval, @mcarg, $tmp);

	while(/\$(\{\w+[\}:]|\d?[a-z])|$STR/o){
		$_ = $';
		$ret .= $`;
		$ret .= $&, next if substr($&, 0, 1) ne '$'; # 矢机误は润借妄

		$mcvar = $1;	# マクロ叹
		if($mcvar =~ /:$/){
			# マクロの苞眶を@mcargに评る
			@mcarg = &getarg($_, $lineno, $fnm, $vfnm);
			$_ = pop(@mcarg);
		} else {
			@mcarg = ();
		}
		# マクロの猛を$mcvalに评る
		$mcvar =~ s/[^\w]//g;
		$mcval = $macro{$mcvar};
		&err("Undefined macro \$\{$mcvar\}", $lineno, $fnm, $vfnm)
			unless defined($mcval);
		# 苞眶つきマクロの眷圭は苞眶弥き垂え
		$mcval =~ s/#(\d|\{\d+\})|$STR/
			substr($&, 0, 1) ne '#' ? $& :
			  (($tmp = $1) =~ s|[^0-9]||g, $mcarg[$tmp - 1])
			# perl4.036では≈\d∽だとなぜかうまく乖かない
		/ego;
		# マクロ柒でマクロが蝗われていれば构に鸥倡し、冯蔡をつなげる
		$ret .= &macroexp($mcval, $lineno, $fnm, $vfnm);
	}
	$ret . $_;
}

sub getarg{
	# 乖の黎片から≈湿∽の≈,∽で惰磊られた误を艰って芹误とし、
	# 木稿の≈}∽を琼いだ荒りの乖を呵稿の妥燎として芹误に纳裁して手す。
	# ≈湿∽とは笆布のものの息なり: ≈"{}\,∽笆嘲の矢机、
	# ≈\∽+1机、矢机误、および≈{∽+(湿か≈,∽)の误+≈}∽
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($arg, $tmp, @ret, @tmp, $flg);

	while(/^(([^"\{\}\\,]|$STR)+|\\.|[\{,])/o){
		$flg = 1, $tmp = $&, $_ = $';
		push(@ret, $arg), $arg = '', next if $tmp eq ','; #≈,∽の眷圭
		$tmp =~ s/^\\//, $arg .= $tmp, next if $tmp ne '{';
		 # ≈{∽で幌まるもの笆嘲の眷圭。≈\∽があれば近殿

		 # ≈{∽で幌まるものの眷圭、面咳を&getargで艰评し芬げる
		@tmp = &getarg($_, $lineno, $fnm, $vfnm);
		$_ = pop(@tmp);
		$arg .= '{' . join(',', @tmp) . '}';
	}
	&err("Illegal macro usage", $lineno, $fnm, $vfnm) unless s/^\}//;
	push(@ret, $arg) if $flg;
	(@ret, $_);
}

# ベンド猛マクロを鸥倡して手す by YOSHIMURA
# i簇眶 : 0×64×127 (BSコマンド脱)
# j簇眶 : -64×0×63 (F,Uコマンド脱)
# ベンドレンジは徒腆マクロ${_bend_range_}にセットする。
sub bendexp{
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $val, $tmp);

	while(/([ij])(-?\d+)|$STR/o){
		$_ = $';
		$ret .= $`;
		$tmp= substr($&, 0, 1);
		if(($tmp ne 'i')&&($tmp ne 'j')){
			$ret .= $&;
			next; # 矢机误は润借妄
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
