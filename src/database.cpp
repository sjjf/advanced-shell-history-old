#include <sys/stat.h>  /* for stat */
#include <stdio.h>     /* for fopen */
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include "ash_log.hpp"
#include "sqlite3.h"

// TODO(cpa): instead of exiting, throw an exception where i'm currently exiting.
// TODO(cpa): create an include file that holds all the queries I want to use???


using namespace ash;
using namespace std;


Database::Database(const char * filename)
  :db_filename(filename), db(0)
{
  struct stat file;
  // Test that the file exists, if not, create it.
  if (stat(db_filename, &file)) {
    FILE * created_file = fopen(db_filename, "w+e");
    if (!created_file) {
      cout << "ERROR: failed to create new file: " << db_filename << endl;
      exit(1);
    }
    fclose(created_file);
  }

  // Open the DB, if failure, abort.
  if (sqlite3_open(db_filename, &db)) {
    cout << "Failed to open " << db_filename << "\nError: " << sqlite3_errmsg(db) << endl;
    exit(1);
  }

  // Init the DB if it is missing the main tables.
// TODO(cpa): this query can be built from the names of classes extending DBObject...
  char query[] = "select count(*) from sqlite_master where tbl_name in (\"sessions\", \"commands\");";
  if (select_int(query) != 2) {
    init_db();
  }
  // TODO(cpa): maybe emit a warning if there are more tables than just the two...
}


Database::~Database() {
  if (db) {
    sqlite3_close(db);
    db = 0;
  }
}


int CreateTables(void * ignored, int rows, char ** cols, char ** col_names) {
  return 0;
}


void Database::init_db() {
  sqlite3_exec(db, DBObject::get_create_tables().c_str(), CreateTables, 0, 0);
}


// Callback used in Database::select_int
int SelectInt(void * result, int rows, char ** cols, char ** column_names) {
  *((int*) result) = atoi(cols[0]);
  return 0;
}


int Database::select_int(const char * query) const {
  int result = -1;
  if (sqlite3_exec(db, query, SelectInt, &result, 0)) {
    cout << sqlite3_errmsg(db) << endl << query << endl;
    exit(1);
  }
  return result;
}


void Database::exec(const char * query) const {
  if (sqlite3_exec(db, query, CreateTables, 0, 0)) {
    cout << sqlite3_errmsg(db) << endl << query << endl;
    exit(1);
  }
}


int Database::get_session_id() {
  stringstream ss;

  // TODO(cpa): move this method into the Session ctor.
  // First check the environment, return that if present.
  char * id = getenv("AH_SESSION_ID");
  if (id) {
    ss << "select count(*) from sessions where id = " << id << ";";
    if (select_int(ss.str().c_str()) == 0) {
      cout << "ERROR: session_id(" << id << ") not found, creating new session." << endl << ss.str() << endl;
      id = 0;
    }
    ss.str("");

    // TODO(cpa): check that the session is still ongoing - basic sanity check

    if (id) {
      return atoi(id);
    }
  }

  Session session;
  int session_id = select_int(session.get_sql().c_str());
  // TODO(cpa): execute the SQL to insert the session and return the new ID.
  // cout << "session sql = " << session.get_sql() << endl;
  return session_id;
}
