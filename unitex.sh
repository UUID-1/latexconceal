#!/usr/bin/sh

inplace=false
reverse=false
rulesfiles=''
rulesfilter=''
styfile=''
sedscript=''

default_rulesfile="${UNITEX_RULES_FILE:-${XDG_CONFIG_HOME:-~/.config}/unitex/rules.tsv}"

if test -e "${default_rulesfile}" ; then
	rulesfiles="'${default_rulesfile}'"
fi

usage() {
	cat << EOF
usage: $0 [-r|-i|-h] [-u rules-files]... [-f rules-files]... [-m n,pattern]... [-M n,pattern]... [-s style-file] [-S sed-script] [input_files...]
options:
  -r                convert in reverse
  -i                change input files in place
  -u <file>         specify the rules file to use
  -f <file>         specify an additional rules file
  -m <n>,<pattern>  use rules whose <n>th field match <pattern>
  -M <n>,<pattern>  use rules whose <n>th field doesn't match <pattern>
  -s <file>         generated a style file and exit
  -S <file>         generate a sed script and exit
  -h                print this help and exit
EOF
}

while getopts s:m:M:S:iru:f:h opt ; do
	case "${opt}" in
	r)
		reverse=true
		;;
	i)
		inplace=true
		;;
	u)
		rulesfiles="'${OPTARG}'"
		;;
	f)
		rulesfiles="${rulesfiles} '${OPTARG}'"
		;;
	s)
		styfile="${OPTARG}"
		;;
	S)
		sedscript="${OPTARG}"
		;;
	m|M)
		if echo "${OPTARG}" | grep -q '^[1-9][0-9]*,.*$' ; then
			n="${OPTARG%%,*}"
			pat=$( printf '^\\([^\t]\\+\t\\)\\{%d\\}%s\\(\t.*\\)\\?$' $(( n - 1 )) "${OPTARG#*,}" )
		else
			echo "invalid argument to -m: ${OPTARG}" >&2
			exit 1
		fi
		if test "${opt}" = 'm'; then
			rulesfilter="${rulesfilter} | grep '${pat}'"
		else
			rulesfilter="${rulesfilter} | grep -v '${pat}'"
		fi
		;;
	h|\?)
		if test "${opt}" = h ; then
			usage
			exit
		else
			usage >&2
			exit 1
		fi
	;;
	esac
done

if test -z "${rulesfiles}" ; then
	echo 'found no rules file' >&2
	exit 1
fi

tmprulesfile="/tmp/unitex.$$.tmprulesfile"
trap 'rm -f "${tmprulesfile}"' EXIT

eval "grep -v '^#' ${rulesfiles} ${rulesfilter}" | cut -f 1,2 >"${tmprulesfile}"

if ! test -s "${tmprulesfile}" ; then
	echo 'no rule matched' >&2
	exit 1
fi

shift $(( OPTIND - 1 ))

gensty() {
	cat <<EOF
\makeatletter

% This part is based on the newunicodechar LaTeX package
\begingroup
\edef\ext{\@gobble^^^^0021}
\expandafter\endgroup
\ifx\ext\@empty
	\chardef\unitex@atcode=\catcode\`\~
	\catcode\`\~=\active
	\def\unitex@nuc#1#2{
		\catcode\`#1=\active
		\begingroup\lccode\`\~=\`#1
		\lowercase{\endgroup\protected\def~}{#2}
	}
	\catcode\`\~=\unitex@atcode
\else
	\RequirePackage[utf8]{inputenc}
	\def\unitex@nuc#1#2{
		\edef\unitex@temp{\detokenize{#1}}
		\@namedef{u8:\unitex@temp}{#2}
	}
\fi

$( sort -t '	' -k 2,2 -u "${tmprulesfile}" | sed -n 's/^\([^	]\+\)	\(.\)$/\\unitex@nuc{\2}{\1}/p' )

\makeatother
EOF
}

gensed() {

	gensed_tmpfile1="/tmp/unitex.$$.gensed.tmpfile1"
	gensed_tmpfile2="/tmp/unitex.$$.gensed.tmpfile2"

	lnum=1
	while read -r line ; do
		field1="${line%	*}"
		field2="${line#*	}"
		printf '%s\t%s\t%d\t%d\t%d\n' "${field1}" "${field2}" ${#field1} ${#field2} ${lnum}
		lnum=$(( lnum + 1 ))
	done <"${tmprulesfile}" >"${gensed_tmpfile1}"

	sort -nr -t '	' -k 4,4 -k 5,5 "${gensed_tmpfile1}" | cut -f 1,2 \
		| sed -n -e 's![][*.$^\/&]!\\&!g' \
			 -e 's!^\([^	]\+\)	\([^	]\+\)$!s/\2/\1/g!p'

	pat='\(\\\([[:alpha:]]\+\|[^[:alpha:]]\)\({[^{}]*}\)*\|[^\{}]\)' # 3 captures
	patb="\(${pat}[[:blank:]]*\)" # 4 captures
	for leader in '_:SUBS' '\^:SUPS' ; do
		label="MERGE${leader#*:}"
		leader="${leader%:*}"
		cat <<EOF
:${label}
s/\([^\]\)${leader}${patb}${leader}${pat}/\1${leader}{\2\6}/g
s/\([^\]\)${leader}${patb}${leader}{\(${patb}\+\)}/\1${leader}{\2\6}/g
s/\([^\]\)${leader}{\(${patb}\+\)}\([[:blank:]]*\)${leader}${pat}/\1${leader}{\2\7\8}/g
s/\([^\]\)${leader}{\(${patb}\+\)}\([[:blank:]]*\)${leader}{\(${patb}\+\)}/\1${leader}{\2\7\8}/g
t ${label}
EOF
	done

	if ! $reverse ; then
		printf '%s\n' 's/\\[[:alpha:]]\+/&/g'

		sort -nr -t ' ' -k 3,3 -k 5,5 "${gensed_tmpfile1}" | cut -f 1,2 \
			| sed -e 's/\\[[:alpha:]]\+/&/g' \
			      -e 's!^\([_^]\){\([^{}]\+\)}\t!\1\2\t!g' \
			      -e 's![][*.$^\/]!\\&!g' >"${gensed_tmpfile2}"

		for leader in '_:SUBS' '\^:SUPS' ; do
			label="BREAK${leader#*:}"
			leader="${leader%:*}"
			if test "${leader}" = '_' ; then
				escleader='_'
			else
				escleader='\\\^'
			fi
			pat="$( sed -n 's!^'"${escleader}"'\(.\+\)\t.*$!\1!p' "${gensed_tmpfile2}" \
				| sed -e ':L' \
				      -e '$s/\n/\\|/g' \
				      -e 'N' \
				      -e 'b L' )"
			pat="\(\(${pat}\)[[:blank:]]*\)" # 2 captures
			cat <<EOF
:${label}
s/\([^\]\)${leader}{${pat}\(${pat}\+\)}/\1${leader}\2${leader}{\4}/g
t ${label}
s/\([^\]\)${leader}{${pat}}/\1${leader}\2/g
EOF
		done

		sed -n 's!^\([^	]\+\)	\([^	]\+\)$!s/\1/\2/g!p' "${gensed_tmpfile2}"

		echo 's///g'
	fi

	rm -f "${gensed_tmpfile1}" "${gensed_tmpfile2}"
}

if test "${styfile}" ; then
	if test "${styfile}" = '-' ; then
		gensty
	else
		gensty >"${styfile}"
	fi
fi

if test "${sedscript}" ; then
	if test "${sedscript}" = '-' ; then
		gensed
	else
		gensed >"${sedscript}"
	fi
fi

if test \( "${styfile}" -o "${sedscript}" \) -a -z "$@" ; then
	exit
fi

filter() {
	if command -v unitex >/dev/null ; then
		unitex $(if ${reverse} ; then echo '-r' ; fi) -u "${tmprulesfile}" "$@"
	else
		if test "${sedscript}" ; then
			sed -f "${sedscript}" "$@"
		else
			tmpsedscript="unitex.$$.tmpsedscript"
			gensed >"${tmpsedscript}"
			sed -f "${tmpsedscript}" "$@"
			rm -f "${tmpsedscript}"
		fi
	fi
}

if ${inplace} ; then
	for file in "$@" ; do
		tmpfile="unitex.$$.tmpfile"
		filter "${file}" >"${tmpfile}" && mv -f "${tmpfile}" "${file}"
		if test $? -ne 0 ; then
			rm -f "${tmpfile}"
			exit 1
		fi
	done
else
	filter "$@"
fi
