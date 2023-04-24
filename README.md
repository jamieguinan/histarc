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

## Minimal edition

I had a C version that used an sqlite database, but it had problems,
including that piping "hq query | less" would lock the database and
freeze all other running bash sessions that tried to update it.

The simpler version `histarc-minimal.bash` simply appends a text file
in the users home folder. `hq` uses `grep` to query.

A `histarc_disable` shell function disables history saving and sets
the HISTFILE to `/dev/null` for the duration of the shell session,
which is useful in cases like entering passwords or other sensitive
information.

## Similar efforts

"There is nothing new under the sun."

[How (and Why) to Log Your Entire Bash History](https://spin.atomicobject.com/2016/05/28/log-bash-history/)

[BASH history forever.](https://debian-administration.org/article/175/BASH_history_forever.)
