# Histarc

`histarc` is an attempt to archive *all* my bash command history for
later reference. I started this in 2014, and it has been useful on
many occasions to recall obscure command options, or remember the
context around certain commands.

This relieves me from worrying about remembering particular command
invocations or writing them down. I know the history is there, and I
can probably find what I need with a few simple queries in less than a
minute, even if its something I did years ago.

Based on my nominal rate of command-line interaction, even if I had
started such an effort in 1982 on my VIC-20, I should be able to fit
my entire lifetime's worth of command-line interaction in tens or
hundreds of MB.

## C/Sqlite version

The main version of `histarc` is written in C and uses an sqlite
database to record command history.

`histarc.bash` defines a shell function `histarc_update` which calls
the `histarc` binary via `PROMPT_COMMAND`. In my bash interactive
config files (.bashrc) I set it up like this,

    # histarc support.
    HISTARCBASH=${HOME}/projects/histarc/histarc.bash
    if [ -f $HISTARCBASH ]
    then
      source $HISTARCBASH
    fi

histarc uses a few files from my
[CTI](https://github.com/jamieguinan/cti) project. Clone that in an
adjacent folder if you want to build histarc.

`hq` is a shell function convenience wrapper around `histarc query`.
Note that it uses sqlite glob patterns and not regexes.

## Minimal edition

The C version with its sqlite database has its advantages. It is
robust, it includes a useful `merge` subcommand to merge databases
from other systems, and there is no temptation to manually edit the
database outside of sqlite.

But I realized that it a simpler version is
possible. `histarc-minimal.bash` contains an alternative
implementation that simply appends a text file in the users home
folder. For this version, `hq` uses `grep` to query, so it uses
regexes, which is different from the C version which uses glob
patterns.


## Common features

Both versions include a `histarc_disable` shell function to disable
history saving and set the HISTFILE to `/dev/null`, which is useful in
cases like entering passwords or other sensitive information. It might
be a good idea to audit and scrub history archives periodically.

## Similar efforts

"There is nothing new under the sun."

[How (and Why) to Log Your Entire Bash History](https://spin.atomicobject.com/2016/05/28/log-bash-history/)

[BASH history forever.](https://debian-administration.org/article/175/BASH_history_forever.)
