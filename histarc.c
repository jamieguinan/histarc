/* 
  This is an attempt to archive all my bash command history for later
  reference.  It uses an sqlite database to record all the history.
  "histarc.bash" defines a function `histarc_update` that provides
  an interface to the `histarc` program. In my bash interactive
  config files I use the bash setting,
    PROMPT_COMMAND=histarc_update
 */

#include <stdio.h>		/* fprintf, NULL */
#include <sys/types.h>		/* getpid */
#include <unistd.h>		/* gethostname, getpid */
#include <stdlib.h>		/* getenv, srand, rand */
#include <string.h>		/* strlen */
#include <time.h>		/* time, localtime, nanosleep */
#include "sqlite3.h"
#include "../cti/String.h"
#include "../cti/Signals.h"
#include "../cti/dbutil.h"
#include "../cti/CTI.h"
#include "../cti/localptr.h"

#define no_callback NULL
#define no_errmsg NULL

sqlite3 *db;
static int done=0;


SchemaColumn histarc_schema[] = {
  [0] = { "sessionid", ""},
  [1] = { "datetime", ""},
  [2] = { "seq", ""},
  [3] = { "wd", ""},
  [4] = { "cmd", ""},
};

static void close_handler(int signo)
{
  done=1;
}

int busy_handler(void * ptr, int retry_sequence)
{
  /*
   * https://www.sqlite.org/faq.html#q5
   * https://www.sqlite.org/c3ref/busy_handler.html 
   */

  /* I don't care about the args. */

  // printf("%s %d...\n", __func__, retry_sequence); fflush(stdout);

  /* Sleep up to 1/4 second. */
  struct timespec ts;
  srand(getpid());
  ts.tv_sec = 0;
  ts.tv_nsec = 
    (999999999L 		/* max nanoseconds */
     * rand()) 			/* times 0..RAND_MAX */
    / RAND_MAX 			/* scale back to nanoseconds */
    / 4 ;			/* divide to 1/4 second max */
  nanosleep(&ts, NULL);

  return 1;			/* Always retry. */
}


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

  if (0) printf("sessionid=%s\ntimedate=%s\nseq=%s\nwd=%s\ncmd=%s\n",
	 sessionid,
	 timedate,
	 seq,
	 wd,
	 cmdstart);

  do { 
    rc = sql_exec_free_query(db, sqlite3_mprintf("INSERT INTO histarc VALUES(%Q, %Q, %Q, %Q, %Q)", 
						 sessionid,
						 timedate,
						 seq,
						 wd,
						 cmdstart),
			     no_callback, 0, no_errmsg);
    if (rc == 0) {
      // printf("insert Ok\n");
      sqlite3_exec(db, "COMMIT;", no_callback, 0, no_errmsg);
    }
  } while (rc == 5);

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

  char * d = column_strings[1];
  if (strlen(d) != 14) {
    fprintf(stderr, "bad date string in entry: %s\n", d);
    return 1;
  }

  localptr(String, datestr) = String_sprintf("%c%c%c%c-%c%c-%c%c %c%c:%c%c:%c%c"
                                             , d[0], d[1], d[2], d[3]
                                             , d[4], d[5]
                                             , d[6], d[7]
                                             , d[8], d[9]
                                             , d[10], d[11]
                                             , d[12], d[13]
                                             );

  printf("\n[%s] %s\n%s\n"
	 , column_strings[3]
         , s(datestr)
	 , column_strings[4]
         );
  return 0;
}


void query(const char *text)
{
  // char *q = sqlite3_mprintf("SELECT * from histarc where cmd like '%%%s%%'", text);
  /* "i" is passed along to the callback, and can be used for numbering output. 
     It will also hold the number of rows returned when the query is complete. */
  int i=0;
  int rc = sql_exec_free_query(db, sqlite3_mprintf("SELECT * from histarc where cmd glob '*%s*'", text),
			   query_callback, (void*)&i, no_errmsg);
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
  if (rc != 0) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_busy_handler(db, busy_handler, NULL);
  if (rc != 0) {
    fprintf(stderr, "Busy handler registration failed.\n");
    return 1;
  }

  /* Create table if it doesn't already exists. */
  rc = db_check(db, "histarc", histarc_schema, cti_table_size(histarc_schema), "");
  if (rc != 0) {
    fprintf(stderr, "%s\nerror %d: %s\n", q, rc, sqlite3_errmsg(db));
    return 1;
  }

  /* COMMIT operations will trigger a Linux VFS sync/flush, which
     slows down each command by about 0.2 seconds, which is 
     palpable and annoying.  Use a PRAGMA to turn this off. */
  rc = sql_exec_free_query(db, sqlite3_mprintf("PRAGMA synchronous = 0;"), no_callback, 0, no_errmsg);
  if (rc != 0) {
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
