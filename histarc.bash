# histarc support

# HISTARC_SESSIONID is a unique value so that sessions can be extracted
# from the database.  It is probably overkill, but harmless.
HISTARC_SESSIONID=$(hostname).$(tty).${PPID}.${BASHPID}.$(date +%Y%m%d%H%M%S)

# See bash man page about PROMPT_COMMAND.
PROMPT_COMMAND=histarc_update

# HISTARC_LASTWD is used to record the place a command started from, so
# that the record still makes sense for "cd".  And since it is initially
# unset, it can be used to avoid recording the last command from .bash_history
# from the previous session.
unset HISTARC_LASTWD
function histarc_update {
  if [ x"${HISTARC_LASTWD}" != x ]
  then
    ( set -f; histarc record ${HISTARC_SESSIONID} "$(echo $(history 1))" )
  fi
  export HISTARC_LASTWD=$(pwd)
}

function hq() {
    histarc query "$@"
}


function histarc_disable {
    unset PROMPT_COMMAND
    export HISTFILE=/dev/null
}
