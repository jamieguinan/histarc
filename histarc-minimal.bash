# History archiving. Save every command to a file for later searching.
histarc_update ()
{
    echo $(date +%Y-%m-%d_%H.%M.%S) $HOSTNAME $(pwd) "$(history 1)" >> ~/.histarc-${USER}
}

PROMPT_COMMAND=histarc_update

# History query
hq ()
{
    grep -h "$*" ~/.histarc-${USER}
}

histarc_disable()
{
    unset PROMPT_COMMAND
    export HISTFILE=/dev/null
}
