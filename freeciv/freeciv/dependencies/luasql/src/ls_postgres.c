/*
** LuaSQL, PostgreSQL driver
** Authors: Pedro Rabinovitch, Roberto Ierusalimschy, Carlos Cassino
** Tomas Guisasola, Eduardo Quintao
** See Copyright Notice in license.html
** $Id: ls_postgres.c,v 1.11 2009/02/07 23:16:23 tomas Exp $
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "libpq-fe.h"

#include "lua.h"
#include "lauxlib.h"


#include "luasql.h"

#define LUASQL_ENVIRONMENT_PG "PostgreSQL environment"
#define LUASQL_CONNECTION_PG "PostgreSQL connection"
#define LUASQL_CURSOR_PG "PostgreSQL cursor"

typedef struct {
	short      closed;
} env_data;


typedef struct {
	short      closed;
	int        env;                /* reference to environment */
	int        auto_commit;        /* 0 for manual commit */
	PGconn    *pg_conn;
} conn_data;


typedef struct {
	short      closed;
	int        conn;               /* reference to connection */
	int        numcols;            /* number of columns */
	int        colnames, coltypes; /* reference to column information tables */
	int        curr_tuple;         /* next tuple to be read */
	PGresult  *pg_res;
} cur_data;


typedef void (*creator) (lua_State *L, cur_data *cur);


LUASQL_API int luaopen_luasql_postgres(lua_State *L);


/*
** Check for valid environment.
*/
static env_data *getenvironment (lua_State *L) {
	env_data *env = (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_PG);
	luaL_argcheck (L, env != NULL, 1, LUASQL_PREFIX"environment expected");
	luaL_argcheck (L, !env->closed, 1, LUASQL_PREFIX"environment is closed");
	return env;
}


/*
** Check for valid connection.
*/
static conn_data *getconnection (lua_State *L) {
	conn_data *conn = (conn_data *)luaL_checkudata (L, 1, LUASQL_CONNECTION_PG);
	luaL_argcheck (L, conn != NULL, 1, LUASQL_PREFIX"connection expected");
	luaL_argcheck (L, !conn->closed, 1, LUASQL_PREFIX"connection is closed");
	return conn;
}


/*
** Check for valid cursor.
*/
static cur_data *getcursor (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_PG);
	luaL_argcheck (L, cur != NULL, 1, LUASQL_PREFIX"cursor expected");
	luaL_argcheck (L, !cur->closed, 1, LUASQL_PREFIX"cursor is closed");
	return cur;
}


/*
** Push the value of #i field of #tuple row.
*/
static void pushvalue (lua_State *L, PGresult *res, int tuple, int i) {
	if (PQgetisnull (res, tuple, i-1))
		lua_pushnil (L);
	else
		lua_pushstring (L, PQgetvalue (res, tuple, i-1));
}


/*
** Closes the cursor and nullify all structure fields.
*/
static void cur_nullify (lua_State *L, cur_data *cur) {
	/* Nullify structure fields. */
	cur->closed = 1;
	PQclear(cur->pg_res);
	luaL_unref (L, LUA_REGISTRYINDEX, cur->conn);
	luaL_unref (L, LUA_REGISTRYINDEX, cur->colnames);
	luaL_unref (L, LUA_REGISTRYINDEX, cur->coltypes);
}


/*
** Get another row of the given cursor.
*/
static int cur_fetch (lua_State *L) {
	cur_data *cur = getcursor (L);
	PGresult *res = cur->pg_res;
	int tuple = cur->curr_tuple;

	if (tuple >= PQntuples(cur->pg_res)) {
		cur_nullify (L, cur);
		lua_pushnil(L);  /* no more results */
		return 1;
	}

	cur->curr_tuple++;
	if (lua_istable (L, 2)) {
		int i;
		const char *opts = luaL_optstring (L, 3, "n");
		if (strchr (opts, 'n') != NULL)
			/* Copy values to numerical indices */
			for (i = 1; i <= cur->numcols; i++) {
				pushvalue (L, res, tuple, i);
				lua_rawseti (L, 2, i);
			}
		if (strchr (opts, 'a') != NULL)
			/* Copy values to alphanumerical indices */
			for (i = 1; i <= cur->numcols; i++) {
				lua_pushstring (L, PQfname (res, i-1));
				pushvalue (L, res, tuple, i);
				lua_rawset (L, 2);
			}
		lua_pushvalue(L, 2);
		return 1; /* return table */
	}
	else {
		int i;
		luaL_checkstack (L, cur->numcols, LUASQL_PREFIX"too many columns");
		for (i = 1; i <= cur->numcols; i++)
			pushvalue (L, res, tuple, i);
		return cur->numcols; /* return #numcols values */
	}
}


/*
** Cursor object collector function
*/
static int cur_gc (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_PG);
	if (cur != NULL && !(cur->closed))
		cur_nullify (L, cur);
	return 0;
}


/*
** Closes the cursor on top of the stack.
** Returns true in case of success, or false in case the cursor was
** already closed.
** Throws an error if the argument is not a cursor.
*/
static int cur_close (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_PG);
	luaL_argcheck (L, cur != NULL, 1, LUASQL_PREFIX"cursor expected");
	if (cur->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	cur_nullify (L, cur); /* == cur_gc (L); */
	lua_pushboolean (L, 1);
	return 1;
}


/*
** Get the internal database type of the given column.
*/
static char *getcolumntype (PGconn *conn, PGresult *result, int i, char *buff) {
	Oid codigo = PQftype (result, i);
	char stmt[100];
	PGresult *res;

	sprintf (stmt, "select typname from pg_type where oid = %d", codigo);
	res = PQexec(conn, stmt);
	strcpy (buff, "undefined");

	if (PQresultStatus (res) == PGRES_TUPLES_OK) {
		if (PQntuples(res) > 0) {
			char *name = PQgetvalue(res, 0, 0);
			if (strcmp (name, "bpchar")==0 || strcmp (name, "varchar")==0) {
				int modifier = PQfmod (result, i) - 4;
				sprintf (buff, "%.20s (%d)", name, modifier);
			}
			else
				strncpy (buff, name, 20);
		}
	}
	PQclear(res);
	return buff;
}


/*
** Creates the list of fields names and pushes it on top of the stack.
*/
static void create_colnames (lua_State *L, cur_data *cur) {
	PGresult *result = cur->pg_res;
	int i;
	lua_newtable (L);
	for (i = 1; i <= cur->numcols; i++) {
		lua_pushstring (L, PQfname (result, i-1));
		lua_rawseti (L, -2, i);
	}
}


/*
** Creates the list of fields types and pushes it on top of the stack.
*/
static void create_coltypes (lua_State *L, cur_data *cur) {
	PGresult *result = cur->pg_res;
	conn_data *conn;
	char typename[100];
	int i;
	lua_rawgeti (L, LUA_REGISTRYINDEX, cur->conn);
	if (!lua_isuserdata (L, -1))
		luaL_error (L, LUASQL_PREFIX"invalid connection");
	conn = (conn_data *)lua_touserdata (L, -1);
	lua_newtable (L);
	for (i = 1; i <= cur->numcols; i++) {
		lua_pushstring(L, getcolumntype (conn->pg_conn, result, i-1, typename));
		lua_rawseti (L, -2, i);
	}
}


/*
** Pushes a column information table on top of the stack.
** If the table isn't built yet, call the creator function and stores
** a reference to it on the cursor structure.
*/
static void _pushtable (lua_State *L, cur_data *cur, size_t off, creator func) {
	int *ref = (int *)((char *)cur + off);
	if (*ref != LUA_NOREF)
		lua_rawgeti (L, LUA_REGISTRYINDEX, *ref);
	else {
		func (L, cur);
		/* Stores a reference to it on the cursor structure */
		lua_pushvalue (L, -1);
		*ref = luaL_ref (L, LUA_REGISTRYINDEX);
	}
}
#define pushtable(L,c,m,f) (_pushtable(L,c,offsetof(cur_data,m),f))


/*
** Return the list of field names.
*/
static int cur_getcolnames (lua_State *L) {
	pushtable (L, getcursor(L), colnames, create_colnames);
	return 1;
}


/*
** Return the list of field types.
*/
static int cur_getcoltypes (lua_State *L) {
	pushtable (L, getcursor(L), coltypes, create_coltypes);
	return 1;
}


/*
** Push the number of rows.
*/
static int cur_numrows (lua_State *L) {
	lua_pushnumber (L, PQntuples (getcursor(L)->pg_res));
	return 1;
}


/*
** Create a new Cursor object and push it on top of the stack.
*/
static int create_cursor (lua_State *L, int conn, PGresult *result) {
	cur_data *cur = (cur_data *)lua_newuserdata(L, sizeof(cur_data));
	luasql_setmeta (L, LUASQL_CURSOR_PG);

	/* fill in structure */
	cur->closed = 0;
	cur->conn = LUA_NOREF;
	cur->numcols = PQnfields(result);
	cur->colnames = LUA_NOREF;
	cur->coltypes = LUA_NOREF;
	cur->curr_tuple = 0;
	cur->pg_res = result;
	lua_pushvalue (L, conn);
	cur->conn = luaL_ref (L, LUA_REGISTRYINDEX);

	return 1;
}


static void sql_commit(conn_data *conn) {
	PQclear(PQexec(conn->pg_conn, "COMMIT"));
}


static void sql_begin(conn_data *conn) {
	PQclear(PQexec(conn->pg_conn, "BEGIN"));
}


static void sql_rollback(conn_data *conn) {
	PQclear(PQexec(conn->pg_conn, "ROLLBACK"));
}


/*
** Connection object collector function
*/
static int conn_gc (lua_State *L) {
	conn_data *conn = (conn_data *)luaL_checkudata (L, 1, LUASQL_CONNECTION_PG);
	if (conn != NULL && !(conn->closed)) {
		/* Nullify structure fields. */
		conn->closed = 1;
		luaL_unref (L, LUA_REGISTRYINDEX, conn->env);
		PQfinish (conn->pg_conn);
	}
	return 0;
}


/*
** Closes the connection on top of the stack.
** Returns true in case of success, or false in case the connection was
** already closed.
** Throws an error if the argument is not a connection.
*/
static int conn_close (lua_State *L) {
	conn_data *conn = (conn_data *)luaL_checkudata (L, 1, LUASQL_CONNECTION_PG);
	luaL_argcheck (L, conn != NULL, 1, LUASQL_PREFIX"connection expected");
	if (conn->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	conn_gc (L);
	lua_pushboolean (L, 1);
	return 1;
}


/*
** Escapes a string for use within an SQL statement.
** Returns a string with the escaped string.
*/
static int conn_escape (lua_State *L) {
	conn_data *conn = getconnection (L);
	size_t len;
	const char *from = luaL_checklstring (L, 2, &len);
	int error;
	int ret = 1;
	luaL_Buffer b;
	char *to;
#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM == 501)
	/* Lua 5.0 and 5.1 */
	luaL_buffinit (L, &b);
	do {
		int max = LUAL_BUFFERSIZE / 2;
		size_t bytes_copied;
		size_t this_len = (len > max) ? max : len;
		to = luaL_prepbuffer (&b);
		bytes_copied = PQescapeStringConn (conn->pg_conn, to, from, this_len, &error);
		if (error != 0) { /* failed ! */
			return luasql_failmsg (L, "cannot escape string. PostgreSQL: ", PQerrorMessage (conn->pg_conn));
		}
		luaL_addsize (&b, bytes_copied);
		from += this_len;
		len -= this_len;
	} while (len > 0);
	luaL_pushresult (&b);
#else
	/* Lua 5.2 and 5.3 */
	to = luaL_buffinitsize (L, &b, 2*len+1);
	len = PQescapeStringConn (conn->pg_conn, to, from, len, &error);
	if (error == 0) { /* success ! */
		luaL_pushresultsize (&b, len);
	} else {
		ret = luasql_failmsg (L, "cannot escape string. PostgreSQL: ", PQerrorMessage (conn->pg_conn));
	}
#endif
	return ret;
}


/*
** Execute an SQL statement.
** Return a Cursor object if the statement is a query, otherwise
** return the number of tuples affected by the statement.
*/
static int conn_execute (lua_State *L) {
	conn_data *conn = getconnection (L);
	const char *statement = luaL_checkstring (L, 2);
	PGresult *res = PQexec(conn->pg_conn, statement);
	if (res && PQresultStatus(res)==PGRES_COMMAND_OK) {
		/* no tuples returned */
		lua_pushnumber(L, atof(PQcmdTuples(res)));
		PQclear (res);
		return 1;
	}
	else if (res && PQresultStatus(res)==PGRES_TUPLES_OK)
		/* tuples returned */
		return create_cursor (L, 1, res);
	else {
		/* error */
		PQclear (res);
		return luasql_failmsg(L, "error executing statement. PostgreSQL: ", PQerrorMessage(conn->pg_conn));
	}
}


/*
** Commit the current transaction.
*/
static int conn_commit (lua_State *L) {
	conn_data *conn = getconnection (L);
	sql_commit(conn);
	if (conn->auto_commit == 0) {
		sql_begin(conn);
		lua_pushboolean (L, 1);
	} else
		lua_pushboolean (L, 0);
	return 1;
}


/*
** Rollback the current transaction.
*/
static int conn_rollback (lua_State *L) {
	conn_data *conn = getconnection (L);
	sql_rollback(conn);
	if (conn->auto_commit == 0) {
		sql_begin(conn);
		lua_pushboolean (L, 1);
	} else
		lua_pushboolean (L, 0);
	return 1;
}


/*
** Set "auto commit" property of the connection.
** If 'true', then rollback current transaction.
** If 'false', then start a new transaction.
*/
static int conn_setautocommit (lua_State *L) {
	conn_data *conn = getconnection (L);
	if (lua_toboolean (L, 2)) {
		conn->auto_commit = 1;
		sql_rollback(conn); /* Undo active transaction. */
	}
	else {
		conn->auto_commit = 0;
		sql_begin(conn);
	}
	lua_pushboolean(L, 1);
	return 1;
}


/*
** Create a new Connection object and push it on top of the stack.
*/
static int create_connection (lua_State *L, int env, PGconn *const pg_conn) {
	conn_data *conn = (conn_data *)lua_newuserdata(L, sizeof(conn_data));
	luasql_setmeta (L, LUASQL_CONNECTION_PG);

	/* fill in structure */
	conn->closed = 0;
	conn->env = LUA_NOREF;
	conn->auto_commit = 1;
	conn->pg_conn = pg_conn;
	lua_pushvalue (L, env);
	conn->env = luaL_ref (L, LUA_REGISTRYINDEX);
	return 1;
}


static void notice_processor (void *arg, const char *message) {
	(void)arg; (void)message;
	/* arg == NULL */
}


/*
** Connects to a data source.
*/
static int env_connect (lua_State *L) {
	const char *sourcename = luaL_checkstring(L, 2);
	const char *username = luaL_optstring(L, 3, NULL);
	const char *password = luaL_optstring(L, 4, NULL);
	const char *pghost = luaL_optstring(L, 5, NULL);
	const char *pgport = luaL_optstring(L, 6, NULL);
	PGconn *conn;
	getenvironment (L);	/* validate environment */
	conn = PQsetdbLogin(pghost, pgport, NULL, NULL, sourcename, username, password);

	if (PQstatus(conn) == CONNECTION_BAD) {
		int rc = luasql_failmsg(L, "error connecting to database. PostgreSQL: ", PQerrorMessage(conn));
		PQfinish(conn);
		return rc;
	}
	PQsetNoticeProcessor(conn, notice_processor, NULL);
	return create_connection(L, 1, conn);
}


/*
** Environment object collector function.
*/
static int env_gc (lua_State *L) {
	env_data *env = (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_PG);
	if (env != NULL && !(env->closed))
		env->closed = 1;
	return 0;
}


/*
** Closes the environment on top of the stack.
** Returns true in case of success, or false in case the environment was
** already closed.
** Throws an error if the argument is not an environment.
*/
static int env_close (lua_State *L) {
	env_data *env = (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_PG);
	luaL_argcheck (L, env != NULL, 1, LUASQL_PREFIX"environment expected");
	if (env->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	env_gc (L);
	lua_pushboolean (L, 1);
	return 1;
}



/*
** Create metatables for each class of object.
*/
static void create_metatables (lua_State *L) {
	struct luaL_Reg environment_methods[] = {
		{"__gc",    env_gc},
		{"close",   env_close},
		{"connect", env_connect},
		{NULL, NULL},
	};
	struct luaL_Reg connection_methods[] = {
		{"__gc",          conn_gc},
		{"close",         conn_close},
		{"escape",        conn_escape},
		{"execute",       conn_execute},
		{"commit",        conn_commit},
		{"rollback",      conn_rollback},
		{"setautocommit", conn_setautocommit},
		{NULL, NULL},
	};
	struct luaL_Reg cursor_methods[] = {
		{"__gc",        cur_gc},
		{"close",       cur_close},
		{"getcolnames", cur_getcolnames},
		{"getcoltypes", cur_getcoltypes},
		{"fetch",       cur_fetch},
		{"numrows",     cur_numrows},
		{NULL, NULL},
	};
	luasql_createmeta (L, LUASQL_ENVIRONMENT_PG, environment_methods);
	luasql_createmeta (L, LUASQL_CONNECTION_PG, connection_methods);
	luasql_createmeta (L, LUASQL_CURSOR_PG, cursor_methods);
	lua_pop (L, 3);
}

/*
** Creates an Environment and returns it.
*/
static int create_environment (lua_State *L) {
	env_data *env = (env_data *)lua_newuserdata(L, sizeof(env_data));
	luasql_setmeta (L, LUASQL_ENVIRONMENT_PG);

	/* fill in structure */
	env->closed = 0;
	return 1;
}


/*
** Creates the metatables for the objects and registers the
** driver open method.
*/
LUASQL_API int luaopen_luasql_postgres (lua_State *L) {
	struct luaL_Reg driver[] = {
		{"postgres", create_environment},
		{NULL, NULL},
	};
	create_metatables (L);
	lua_newtable (L);
	luaL_setfuncs (L, driver, 0);
	luasql_set_info (L);
#if defined(PQlibVersion)
	lua_pushliteral (L, "_CLIENTVERSION");
	lua_pushinteger (L, PQlibVersion());
	lua_settable (L, -3);
#endif
	return 1;
}
