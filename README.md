`histarc` is an attempt to archive *all* my bash command history for
later reference.  It uses an sqlite database to record command
history. `histarc.bash` defines a function `histarc_update` that
provides an interface to the `histarc` program. In my bash interactive
config files (.bashrc) I set it up like this,

    # histarc support.
    HISTARCBASH=${HOME}/projects/histarc/histarc.bash
    if [ -f $HISTARCBASH ]
    then
      source $HISTARCBASH
      PROMPT_COMMAND=histarc_update
    fi

histarc uses a few files from my
[CTI](https://github.com/jamieguinan/cti) project, clone that in an
adjacent folder if you want to build histarc.

