
# perl�ǽ񤤤��¸�Ū��mml2mid�ѥץ�ץ��å� mmlpp.pl v1.03
# ���ߤΤȤ���¸��Ǥ��Ƥ���Τ�{Ĺ��̾����,��������}�ޥ������
# by nide@ics.nara-wu.ac.jp

# v1.02 Modified by Takanori YOSHIMURA (tak-y@geocities.co.jp)
#  �ԥå��٥���ͷ׻���ǽ���դ�����
# v1.03 Modified by NIDE Naoyuki (nide@ics.nara-wu.ac.jp)
#  open������Υե�����̾����Ƭ�������˶�����|�פʤɤ���������н�

$REM = ';.*|/\*([^\*]|\*[^/])*\*?(\*/|$)';
	# �����Ȥ˥ޥå���������ɽ�����Ĥ��Ƥʤ��Τ�ޤ�(���ӥ��顼�����å�)
$STR = '"([^"\\\\\n]|\\\\(.|\n))*("|\n)';
	# ʸ����(��³�Թ�θ)�˥ޥå���������ɽ�����Ĥ��Ƥʤ��Τ�
	# �ޤ�(���ӥ��顼�����å�)
#----
$BEND_RANGE= 2;
	# �٥�ɥ�󥸡��ǥե���Ȥ�2��
#----

sub eval_str{ # ʸ������Ρ�\�פ�ɾ��
	local($_) = $_[0];

	s/\\(x[0-9a-fA-F]{1,2}|[0-7]{1,3}|.)/
		$1 eq 'v' ? "\013" : eval('"' . $& . '"')/eg;
	 # perl4.036�Ǥϡ�eval("\"$&\")�פ��Ȥʤ������ޤ��Ԥ��ʤ�
	$_;
}

sub repl_sub{ # ���������������֤���ʸ����Ϥ��Τޤ�
	local($_) = $_[0];

	/^"/ ? (/\n$/ ? "\032\001" : $_) :
	       (/\n$/ ? "\032\002" : (y/\n//cd, $_))
	# "\032\001"��ʸ�����Ĥ��Ƥʤ���"\032\002"�ϥ����Ȥ�
	# �Ĥ��Ƥʤ����顼��ɽ�������ȤǤ���餬���Ĥ���Х��顼��λ
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

	$fnm = "./$fnm" if $fnm !~ /^\S/;	# '-'��quote����ʤ�
	open(IN, "< $fnm\0") || die "$0: Can't " .
		($incl ? 'include' : 'open') . " '$fnm': $!\n";
	$_ = <IN>, close(IN);			# �����Ƥ��ɤ�

	y/\r//d; s/\032(.|\n)*$//;	# CR, ^Z�ʸ����(�ƥ����ȥ⡼��
					# �ʤ����פʤϤ�����ǰ�Τ���)
	s/$/$' eq '' ? "\n" : ''/e;	# �Ǹ�˲��Ԥʤ�����ղ�
	s/$REM|$STR/&repl_sub($&)/oeg;	# ������
	s/(.*\\\n)+(.*[^\\\n])?\n/
		$tmp2 = (($tmp1 = $&) =~ s|\\\n||g), $tmp1 . "\n" x $tmp2
	/eg;				# ��³��
	s/(^|\n)[ \t]+/$1/g;		# ��Ƭ�������

	# ���λ����������³�Ԥ��Ƭ����⼺���Ƥ���

	# perl4.036�Ǥϡ�foreach(@lines=��)�פ��Ȥʤ������ޤ��Ԥ��ʤ�
	# (�⤦1�ս��Ʊ��)
	@lines = split(/\n/);
	foreach(@lines){
		$lineno++;
		if(/\032\001/){
			# �Ĥ��Ƥʤ�ʸ���󤬤����ˤ��ä������顼��λ
			&err('String not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/\032\002/){
			# �Ĥ��Ƥʤ������Ȥ������ˤ��ä������顼��λ
			&err('Comment not closed', $lineno, $fnm, $vfnm);
		}
		elsif(/^#\s*(include|\d+)\s*($STR|)\s*$/o){
			$tmp2 = substr($2, 1, length($2) - 2);
			if($1 ne 'include'){	# ��# �� "�ե�����̾"��
				$vfnm = $tmp2;
				$lineno = $1 - 1;
				$_ = sprintf("# %d \"%s\"", $1,
					$vfnm eq '' ? $fnm : $vfnm);
			} else {		# ��#include "�ե�����̾"��
				$_ = sprintf("# 1 \"%s\"\n%s# %d \"%s\"",
					$tmp2, &load(&eval_str($tmp2), 1),
					$lineno + 1,
					$vfnm eq '' ? $fnm : $vfnm);
			}
		}
		elsif(/^\$(\{\w+\}|\d?[a-z])/){	# �ޥ��������
			$tmp1 = $1, $tmp2 = $';
			$tmp1 =~ s/[^\w]//g;
			$tmp2 =~ s/^[ \t]+//;
			$macro{$tmp1} = $tmp2;	# �ޥ����������Ǽ
			#----
			if($tmp1 eq "_bend_range_") # ͽ��ޥ���
			{
				$BEND_RANGE= $tmp2;
			}
			#----
			$_ = '';
		}
	}

	# ���λ����ǡ�#include�פȥޥ�������Ԥϼ����Ƥ��ꡢ�Ĥ��Ƥʤ�
	# ʸ����䥳���ȤΥ��顼�����å���Ѥ�

	@lines = split(/\n/, join("\n", @lines));
	$lineno = 0, $vfnm = '';
	foreach(@lines){
		$lineno++;
		if(/^#\s*(\d+)\s*($STR|)\s*$/o){
		 # ��# �� "�ե�����̾"�פιԡ����顼ɽ���Ѥˤ��ξ���ɬ��
			$vfnm = substr($2, 1, length($2) - 2);
			$lineno = $1 - 1;
		 	next;
		}
		elsif(!/^#/){	# �ޥ����Ÿ��
			$_ = &macroexp($_, $lineno, $fnm, $vfnm);
			#----
			$_ = &bendexp($_, $lineno, $fnm, $vfnm);
			#----
		}
	}

	join("\n", @lines) . "\n";
}

sub macroexp{ # ����Υޥ����Ÿ�������֤�
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $mcvar, $mcval, @mcarg, $tmp);

	while(/\$(\{\w+[\}:]|\d?[a-z])|$STR/o){
		$_ = $';
		$ret .= $`;
		$ret .= $&, next if substr($&, 0, 1) ne '$'; # ʸ����������

		$mcvar = $1;	# �ޥ���̾
		if($mcvar =~ /:$/){
			# �ޥ���ΰ�����@mcarg������
			@mcarg = &getarg($_, $lineno, $fnm, $vfnm);
			$_ = pop(@mcarg);
		} else {
			@mcarg = ();
		}
		# �ޥ�����ͤ�$mcval������
		$mcvar =~ s/[^\w]//g;
		$mcval = $macro{$mcvar};
		&err("Undefined macro \$\{$mcvar\}", $lineno, $fnm, $vfnm)
			unless defined($mcval);
		# �����Ĥ��ޥ���ξ��ϰ����֤�����
		$mcval =~ s/#(\d|\{\d+\})|$STR/
			substr($&, 0, 1) ne '#' ? $& :
			  (($tmp = $1) =~ s|[^0-9]||g, $mcarg[$tmp - 1])
			# perl4.036�Ǥϡ�\d�פ��Ȥʤ������ޤ��Ԥ��ʤ�
		/ego;
		# �ޥ�����ǥޥ����Ȥ��Ƥ���й���Ÿ��������̤�Ĥʤ���
		$ret .= &macroexp($mcval, $lineno, $fnm, $vfnm);
	}
	$ret . $_;
}

sub getarg{
	# �Ԥ���Ƭ�����ʪ�פΡ�,�פǶ��ڤ�줿����ä�����Ȥ���
	# ľ��Ρ�}�פ������Ĥ�ιԤ�Ǹ�����ǤȤ���������ɲä����֤���
	# ��ʪ�פȤϰʲ��Τ�Τ�Ϣ�ʤ�: ��"{}\,�װʳ���ʸ����
	# ��\��+1����ʸ���󡢤���ӡ�{��+(ʪ����,��)����+��}��
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($arg, $tmp, @ret, @tmp, $flg);

	while(/^(([^"\{\}\\,]|$STR)+|\\.|[\{,])/o){
		$flg = 1, $tmp = $&, $_ = $';
		push(@ret, $arg), $arg = '', next if $tmp eq ','; #��,�פξ��
		$tmp =~ s/^\\//, $arg .= $tmp, next if $tmp ne '{';
		 # ��{�פǻϤޤ��ΰʳ��ξ�硣��\�פ�����н���

		 # ��{�פǻϤޤ��Τξ�硢��Ȥ�&getarg�Ǽ������Ҥ���
		@tmp = &getarg($_, $lineno, $fnm, $vfnm);
		$_ = pop(@tmp);
		$arg .= '{' . join(',', @tmp) . '}';
	}
	&err("Illegal macro usage", $lineno, $fnm, $vfnm) unless s/^\}//;
	push(@ret, $arg) if $flg;
	(@ret, $_);
}

# �٥���ͥޥ����Ÿ�������֤� by YOSHIMURA
# i�ؿ� : 0��64��127 (BS���ޥ����)
# j�ؿ� : -64��0��63 (F,U���ޥ����)
# �٥�ɥ�󥸤�ͽ��ޥ���${_bend_range_}�˥��åȤ��롣
sub bendexp{
	local($_, $lineno, $fnm, $vfnm) = @_;
	local($ret, $val, $tmp);

	while(/([ij])(-?\d+)|$STR/o){
		$_ = $';
		$ret .= $`;
		$tmp= substr($&, 0, 1);
		if(($tmp ne 'i')&&($tmp ne 'j')){
			$ret .= $&;
			next; # ʸ����������
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
