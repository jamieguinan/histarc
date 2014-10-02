# histarc support
# HISTARC_SESSIONID is a unique value so that sessions can be extracted
# from the database.  It is probably overkill, but harmless.
HISTARC_SESSIONID=$(hostname).$(tty).${PPID}.${BASHPID}.$(date +%Y%m%d%H%M%S)
# HISTARC_LASTWD is used to record to place a command started from, so
# that the record still makes sense for "cd".  And since it is initially
# unset, it can be used to avoid recording the last command from .bash_history
# from the previous session.
unset HISTARC_LASTWD
function histarc_update {
  #echo histarc_update HISTARC_LASTWD=$HISTARC_LASTWD NOHISTARC=$NOHISTARC
  if [ x"${HISTARC_LASTWD}" != x ]
  then
    if [ x$NOHISTARC != x1 ]
    then
      # There is a minor issue here in that the $(history 1) expands
      # globs, I'd like to fix that. 
      # 2014-Sep-15: Fixed, use a subshell and call "set -f".
      ( set -f; histarc record ${HISTARC_SESSIONID} "$(echo $(history 1))" )
    else
      true
    fi
  fi
  export HISTARC_LASTWD=$(pwd)
}
