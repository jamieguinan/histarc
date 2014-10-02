/* 
  This is an attempt to archive all my bash command history for later
  reference.  It uses an sqlite database to record all the history.
  It requires a special setting of PROMPT_COMMAND and a special bash
  function called histupdate, usually sourced by or added to ~/.bashrc.
  See "./histarc.bash"
 */

#include <stdio.h>		/* fprintf, NULL */
#include <unistd.h>		/* gethostname */
#include <stdlib.h>		/* getenv */
#include <string.h>		/* strlen */
#include <time.h>		/* time(), localtime() */
#include "sqlite3.h"
#include "../cti/String.h"
#include "../cti/Signals.h"

#define no_callback NULL
#define no_errmsg NULL

sqlite3 *db;
static int done=0;

static void close_handler(int signo)
{
  done=1;
}

/* A previous version of histarc that used a long-running process
   supported ignoring indentical commands.  Not a big deal... */
// static char *lastcmd;

static void record(char * buffer)
{
  int rc;
  // char *errmsg = NULL;
  int i;
  char *wd;
  char timedate[8+6+32] = {};
  time_t tnow;
  struct tm * tmnow;

  wd = getenv("HISTARC_LASTWD");
  if (!wd) {
    fprintf(stderr, "%s: HISTARC_LASTWD not set in environment\n", __func__);
    return;
  }

  tnow = time(NULL);
  tmnow = localtime(&tnow);
  if (!tmnow) {
    fprintf(stderr, "%s: localtime() or time() error\n", __func__);    
    return;
  }

  if (strftime(timedate, sizeof(timedate)-1, "%Y%m%d%H%M%S", tmnow) == 0) {
    fprintf(stderr, "%s: strftime() error\n", __func__);    
    return;
  }

  String_list *tokens = String_split_s(buffer, " ");
  char *sessionid = s(String_list_get(tokens, 0));
  char *seq = s(String_list_get(tokens, 1));

  /* Command starts after 2nd space. */
  char *cmdstart = strstr(strstr(buffer, " ")+1, " ")+1;
  if (!cmdstart) {
    fprintf(stderr, "separator not found\n");
    goto out;
  }

  /* Trim newline. */
  char *newline = strchr(cmdstart, '\n');
  if (newline) { *newline = 0; }

#if 0
  /* Compare to last command. */
  if (!lastcmd) {
    lastcmd = strdup(cmdstart);
  }
  else { 
    if (streq(lastcmd, cmdstart)) {
      /* Skip sqlite action if same command. */
      goto out;
    }
    else {
      free(lastcmd);
      lastcmd = strdup(cmdstart);
    }
  }
#endif

  if (0) printf("sessionid=%s\ntimedate=%s\nseq=%s\nwd=%s\ncmd=%s\n",
	 sessionid,
	 timedate,
	 seq,
	 wd,
	 cmdstart);

  if (1) { 
    /* Begin sqlite action */
    char *q = sqlite3_mprintf("INSERT INTO histarc VALUES(%Q, %Q, %Q, %Q, %Q)", 
			      sessionid,
			      timedate,
			      seq,
			      wd,
			      cmdstart);

  retry:
    rc = sqlite3_exec(db, q, no_callback, 0, no_errmsg);
    if (rc == 0) {
      // printf("insert Ok\n");
      sqlite3_exec(db, "COMMIT;", no_callback, 0, no_errmsg);
    }
    else {
      fprintf(stderr, "%s\nerror %d: %s\n", q, rc, sqlite3_errmsg(db));
      if (rc == 5) {
	goto retry;
      }
    }
    sqlite3_free(q);
    /* End sqlite action */
  }

 out:
  if (!String_list_is_none(tokens)) {
    String_list_free(&tokens);
  }
}


static int query_callback(void *i_ptr, 
			  int num_columns, char **column_strings, char **column_headers) 
{
  int expected_columns = 5;
  if (num_columns != expected_columns) {
    fprintf(stderr, "%s: expected %d columns in result\n", __func__, expected_columns);
    return 1;
  }
  
  int *pi = i_ptr;
  if (*pi == 0) {
  }
  (*pi) += 1;

  // column_headers[3] is "wd"
  // column_headers[4] is "cwd"

  printf("\n[%s]\n%s\n", 
	 column_strings[3], 
	 column_strings[4]);
  return 0;
}


void query(const char *text)
{
  // char *q = sqlite3_mprintf("SELECT * from histarc where cmd like '%%%s%%'", text);
  char *q = sqlite3_mprintf("SELECT * from histarc where cmd glob '*%s*'", text);
  /* "i" is passed along to the callback, and can be used for numbering output. 
     It will also hold the number of rows returned when the query is complete. */
  int i=0;			
  int rc = sqlite3_exec(db, q, query_callback, (void*)&i, no_errmsg);
  if (rc != 0) {
    fprintf(stderr, "%s\nerror %d: %s\n", q, rc, sqlite3_errmsg(db));
  }
  sqlite3_free(q);
}


void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [command [args...]]\n", argv0);
  fprintf(stderr, "Available commands:\n");
  fprintf(stderr, "  record <command line ...>\n");
  fprintf(stderr, "  query <pattern...>\n");
}

enum { HISTARC_RECORD, HISTARC_QUERY};
int mode = HISTARC_RECORD;

int main(int argc, char *argv[])
{
  int rc;
  char *q = NULL;		/* const string querys */
  char *HOME = getenv("HOME");
  if (!HOME) {
    perror("getenv HOME");
    return 1;
  }

  if (argc == 1) {
    usage(argv[0]);
    return 1;
  }

  if (streq(argv[1], "record")) {
    mode = HISTARC_RECORD;
  }
  else if (streq(argv[1], "query")) {
    mode = HISTARC_QUERY;
  }
  else {
    usage(argv[0]);
    return 1;
  }

  /* Handle some signals */
  //Signals_handle(1, close_handler);
  //Signals_handle(2, close_handler);
  //Signals_handle(3, close_handler);
  //Signals_handle(6, close_handler);
  //Signals_handle(7, close_handler);
  //Signals_handle(13, close_handler);

  /* Figure out database path. */
  char hostname[256] = {};
  gethostname(hostname, sizeof(hostname)-1);
  String * dbname = String_sprintf("%s/.histarc-%s", HOME, hostname);

  /* Open database. */
  rc = sqlite3_open(s(dbname), &db);
  if(rc != 0) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  /* Create table if it doesn't already exists. */
  q = "CREATE TABLE IF NOT EXISTS histarc(sessionid, datetime, seq, wd, cmd);";
  rc = sqlite3_exec(db, q, no_callback, 0, no_errmsg);
  if(rc != 0) {
    fprintf(stderr, "%s\nerror %d: %s\n", q, rc, sqlite3_errmsg(db));
      return 1;
  }

  /* COMMIT operations will trigger a Linux VFS sync/flush, which
     slows down each command by about 0.2 seconds, which is 
     palpable and annoying.  Use a PRAGMA to turn this off. */
  q = "PRAGMA synchronous = 0;";
  rc = sqlite3_exec(db, q, no_callback, 0, no_errmsg);
  if(rc != 0) {
    fprintf(stderr, "%s\nerror %d: %s\n", q, rc, sqlite3_errmsg(db));
      return 1;
  }
  

  if (mode == HISTARC_RECORD && argc == 2) {
    fprintf(stderr, "Please supply command(s) to be recorded.\n");
  }
  else if (mode == HISTARC_RECORD && argc >= 3) {
    /* I think the usual operation will be for the entire command-line
       to be supplied in argv[2], but I want to allow for separate
       tokens in argv[3...]. */
    String * buffer = String_new(argv[2]);
    int argn;
    for (argn=3; argn < argc; argn++) {
      /* Add additional arguments with space separation. */
      String_cat2(buffer, " ", argv[argn]);
    }
    
    record(s(buffer));
  }
  else if (mode == HISTARC_QUERY && argc == 2) {
    fprintf(stderr, "Please supply query text.\n");
  }
  else if (mode == HISTARC_QUERY && argc >= 3) {
    /* FIXME: Concatenate additional args if supplied. */
    query(argv[2]);
  }

  sqlite3_close(db);

  return 0;
}
