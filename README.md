## Introduction

This program replaces (La)TeX commands in its input with Unicode
characters, according to a customizable set of rules. It addresses
these problems:

+ Correctly convert partially converted text. For example, if unitex is
  supplied with the rules `\in` -> `∈` and `\not\in` -> `∉`, then
  it knows to convert `\not∈` to `∉` (Such situations can occur if
  one edits the resulting document of unitex's conversion or inserts
  some Unicode characters directly).

+ Properly handle grouped superscript/subscripts. For example, if
  unitex is supplied with the rules `^\alpha` -> `ᵅ` and `^\beta` ->
  `ᵝ`, then it is able to convert `x^{\alpha\beta}` to `xᵅᵝ` and
  `xᵅᵝ` back to `x^{\alpha\beta}`. Moreover, in a situation like
  `xᵅᵝ^\gamma`, where suppose there are rules for Greek letters and
  for `^\alpha` and `^\beta`, but lacks a rule for `^\gamma`, unitex
  will convert it to `x^{αβγ}`.

Yet the program is fairly fast. On my Core i5 machine I ran

    time unitex test.tex >/dev/null

where `unitex` is supplied with over 1000 conversion rules from its
default rules file and `test.tex` is a LaTeX document of over 2000 lines,
here is the average statistics:

    0.01s user 0.00s system 96% cpu 0.013 total

A main purpose of this program is integrating with vim (see the vim plugin
[vim-unitex](https://github.com/juiyung/vim-unitex)) to help provide a
better edition experience of TeX documents than vim's syntax concealment
alone does, which attaches much importance to what's listed above.

## Usage

Options overview:

    usage: unitex [-r|-h] [-u rules_file]... [-f rules_files]... [input_files...]
    options:
      -r         convert in reverse
      -u <file>  specify the rules file to use
      -f <file>  specify an additional rules file
      -v         print version number and exit
      -h         print this help and exit

UTF-8 encoding is assumed for input files and rules files.

Unitex reads from the specified input files (can be `-` for standard
input), or standard input if no input file is given, and prints the
result of conversion to standard output.

<a id="defrules">
Unitex determines a path of its default rules file, which contains
conversion rules it uses, as follows: If the environment variable
`UNITEX_RULES_FILE` is set and not empty the value is used,
otherwise the XDG Base Directory Speicification is followed and
`$XDG_CONFIG_HOME/unitex/rules.tsv` is used, where if the environment
variable `XDG_CONFIG_HOME` is not set or is empty `$HOME/.config` is
used as its value. If the file does exist, it is added to an internal
list of rules files for unitex to read from.
</a>

The `-u` and `f` options can be used to add to the list of rules files,
`-u` would empty the list first (mainly useful for skipping default
rules), whereas `-f` doesn't.

A detailed description of rules files can be found in a [later
section](#rules).

## Installation

Requirements (Most users of Unix-like systems don't need to worry
about these):
+ A C99 compiler using ASCII compatible character encoding.
+ A POSIX compliant system providing POSIX.1-2008 interface.
+ A POSIX compliant make.

With the requirements met, execute:

    git clone https://github.com/juiyung/unitex.git
    cd unitex
    sudo make install

This installs the executable `unitex` under `/usr/local/bin`. In case
you want another installation location you could change the `PREFIX`
variable in the Makefile.

The repository contains a file named `rules.tsv`, which is an example
rules file, you could copy it to a suitable place to make it a default
rules file for `unitex` (refer to the [Usage](#defrules) section for
where to put it or how to customize the location).

## <a id="rules">The Rules File</a>

Unitex reads rules files containing Tab-separated values, in each line
the first field is a substituend consisting of one or more TeX tokens and
the second field is the corresponding Unicode character.  Character in
the second field can contain one or more combining characters. Empty
lines are ignored, lines beginning with a `#` symbol are ignored as
comments. More fields in addition to the first two are ignored ([Extending
Unitex](#ext) gives an example of how additional fields may be utilized
to select rules.).

Unescaped spaces, both in conversion rules and in input, have no
significance when matching substituends, so `\.A` in a rule can match
`\. A` in input and vice versa. When performing Unicode-to-TeX conversion
(as instructed by the `-r` option), however, spaces in the middle of
the first field would be insert as they are given in the rules file.

If multiple rules map the same substituend to different characters, the
rule read last overwrites the former, this in particular enables a further
specified rules file to overwrite rules from the default rules file. If
multiple rules map different substituends to the same character, then,
when performing Unicode-to-TeX conversion, the one read last is preferred.
Different substituends being mapped to the same character leads to
losses of information in conversion, on the other hand finding a suitable
different character for each can be difficult, for solving this problem
a general method is suggested in [Resolving Conflicting Rules](#conf).

The rules in the example rules file `rules.tsv` contained in this
repository are majorly extracted from the `syntax/tex.vim` file
shipped with vim. The third field is the syntax file's classification
of concealment characters (you can see `:help g:tex_conceal` in vim for
details), the ones adopted are a - accents/ligatures, m - math symbols,
g - Greek, s - superscripts/subscripts, and S - special characters.

## Compiling Resulting Documents

Unicode characters in the document usually either cause the compiler to
issue errors or are ignored by the compiler, the recommended solution is
to simply convert the document back before compilation. The associated vim
plugin [vim-unitex](https://github.com/juiyung/vim-unitex) is useful for
this purpose, it employs this approach though in a more subtle manner:
The plugin performs conversion in the buffer and handles buffer writing so
that when the buffer is written to a disk file what's actually written
is the restored buffer content, this way a vim user would feel like
editing a converted file while compiler sees a restored file.

There is a somehow more direct approach: You can set up Unicode
characters for the compiler to understand, by means of, e.g., using the
`\newunicodechar` command provided by the "newunicodechar" package. The
[`unitex.sh` script](#ext) provides a relevant option `-s`. Compared
with the former one, the major deficiency of this method is that you
can't use combining characters, which is particularly useful in avoiding
conflicting rules, another problem is that it's possible for conversion
to be carried out in somewhere like a label or a verbatim environment,
in such situations the latter approach wouldn't work, whereas the former
approach likely would.

## <a id="conf">Resolving Conflicting Rules</a>

Demonstrated here is how to avoid multiple substituends being mapped to
the same character, serving as a tip if you want to write a rules file
yourself or change the provided one.

Some cases like `\not=`, `\neq`, and `\ne` all being mapped to `≠` as
far as I am concerned don't matter and are needless to avoid. It's more
of a concern when both `\.{a}` and `\dot{a}` are mapped to `ȧ`, `\.` is a
text mode command and `\dot` is a math mode command, improperly restoring
the character to one of them causes compilation problems. What's done
in the provided `rules.tsv` file is that the corresponding character to
`\.{a}` is the single Unicode character ȧ (U+0227) and the correspsonding
character to `\dot{a}` is letter "a" combined with U+0307, the character
"combining dot above".

There are also cases like `\implies` and `\Rightarrow`, a symbol other
than `⇒` is hard to be found suitable for either, but the two commands
do make a difference in typesetting. In the `rules.tsv` a combining
grapheme joiner (CGJ, U+034F) is (ab)used to mark the distinction, `\implies`
is assigned the single character `⇒` (U+21D2) and `\Rightarrow` is
assigned U+21D2 + U+034F, the same symbol attached by a CGJ. Where it's
supported, CGJ should be invisible.

## <a id="ext">Extending Unitex</a>

In the repository there is a shell script `unitex.sh`. It's a POSIX shell
script wrapper around `unitex` that extends `unitex` with more features.
Here is the usage overview:

    usage: ./unitex.sh [-r|-i|-h] [-u rules-files]...  [-f rules-files]... [-m n,pattern]... [-s style-file] [-S sed-script] [input_files...]
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

The `-r`, `-u`, and `-f` options works the same as unitex's. The `-i`
option mimics the GNU extension to `sed`.

The `-m` option accepts a number and a BRE pattern separated by a comma,
the pattern is matched against the whole field (so anchoring with `^` or
`$` probably won't do what's intended). For example with the `rules.tsv`
provided, using `-m 3,'[mg]'` selects rules for math symbols and Greek
letters and excludes the rest. The `-M` option does the inverse of
`-m`. Prof. Yossi Gil ([acknowledgement](#ack)) once required there to
be more fields including code points etc. by which one can select rules.

The `-s` and `-S` options work according to rules specified and selected
with other options. The `-s` option creates a style file usable in both
Unicode and 8-bit TeX engines that assigns meaning to Unicode characters
according to the rules, so that this style file can be used to enable
correct compilation of a converted document, hopefully. The `-S` options
creates a sed script that attempts to do the same job as unitex does.
A big portion of the shell script is dedicated for generating the
sed script, and the resulting sed script is made highly effective in
practice. Prof. Yossi Gil at the beginning suggested the project to
be about a program that's able to generate such sed commands or vim
substitution commands and also able to generate such a style file,
that's where the features come from.

I more or less have redefined the project since it was started, also I
tend to make the program do one thing and do it well, so some original
ideas might not seem very relevant especially after the long period
during which I wrote the program on my own, yet those ideas still set
up good examples.

It's interesting to note that the ability to generate a sed script doing
the conversion makes it possible for the shell script to do this: If
the shell script can't find the `unitex` executable, it would generate
a sed script and run them with `sed` on the input, and therefore acts
as a standalone converter relying only on standard POSIX utilities. So
to be accurate the `unitex.sh` shell script is a wrapper of `unitex`
only when `unitex` is available and a substitute otherwise. It would
still be sufficiently effective in many cases, though much less efficient.

## <a id="ack">Acknowledgement</a>

Credits to CS professor Yossi Gil in Israel Institute of Technology,
who initiated this project. It was in the process of having discussions
with him that I developed some of the most important ideas that make
the program what it is. I appreciate his efforts and contributions,
notably his suggestion of the name "unitex".
