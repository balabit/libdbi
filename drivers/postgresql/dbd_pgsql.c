/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, David Parker and Mark Tobenkin.
 * http://libdbi.sourceforge.net
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * dbd_pgsql.c: PostgreSQL database support (using libpq)
 * Copyright (C) 2001, David Parker <david@neongoat.com>.
 * http://libdbi.sourceforge.net
 * 
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

#include <postgresql/libpq-fe.h>
#include "pgsql-reserved.h"

static const dbi_info_t plugin_info = {
	"pgsql",
	"PostgreSQL database support (using libpq)",
	"David Parker <david@neongoat.com>",
	"http://libdbi.sourceforge.net",
	"dbd_pgsql v0.01",
	__DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = PGSQL_RESERVED_WORDS;

/* function delarations */
void dbd_register_plugin(const dbi_info_t **_plugin_info, const char ***_custom_functions, const char ***_reserved_words);
int dbd_initialize(dbi_plugin_t *plugin);
int dbd_connect(dbi_driver_t *driver);
int dbd_disconnect(dbi_driver_t *driver);
int dbd_fetch_row(dbi_result_t *result, unsigned int rownum);
int dbd_free_query(dbi_result_t *result);
int dbd_goto_row(dbi_driver_t *driver, unsigned int row);
dbi_result_t *dbd_list_dbs(dbi_driver_t *driver);
dbi_result_t *dbd_list_tables(dbi_driver_t *driver, const char *db);
dbi_result_t *dbd_query(dbi_driver_t *driver, const char *statement);
char *dbd_select_db(dbi_driver_t *driver, const char *db);
int dbd_geterror(dbi_driver_t *driver, int *errno, char **errstr);
int dbd_geterror(dbi_driver_t *driver, int *errno, char **errstr);

/* to shut gcc up... */
int asprintf(char **, const char *, ...);


void dbd_register_plugin(const dbi_info_t **_plugin_info, const char ***_custom_functions, const char ***_reserved_words) {
	/* this is the first function called after the plugin module is loaded into memory */
	*_plugin_info = &plugin_info;
	*_custom_functions = custom_functions;
	*_reserved_words = reserved_words;
}

int dbd_initialize(dbi_plugin_t *plugin) {
	/* perform any database-specific server initialization.
	 * this is called right after dbd_register_plugin().
	 * return -1 on error, 0 on success. if -1 is returned, the plugin will not
	 * be added to the list of available plugins. */
	
	return 0;
}

int dbd_connect(dbi_driver_t *driver) {
	const char *host = dbi_driver_get_option(driver, "host");
	const char *username = dbi_driver_get_option(driver, "username");
	const char *password = dbi_driver_get_option(driver, "password");
	const char *dbname = dbi_driver_get_option(driver, "dbname");
	int port = dbi_driver_get_option_numeric(driver, "port");

	/* pgsql specific options */
	const char *options = dbi_driver_get_option(driver, "pgsql_options");
	const char *tty = dbi_driver_get_option(driver, "pgsql_tty");

	PGconn *conn;
	char *port_str;
	char *conninfo;

	if (port) asprintf(&port_str, "%d", port);
	else port_str = NULL;

	asprintf(&conninfo, "host='%s' port='%s' dbname='%s' user='%s' password='%s' options='%s' tty='%s'",
		host ? host : "", /* if we pass a NULL directly to the %s it will show up as "(null)" */
		port_str ? port_str : "",
		dbname ? dbname : "",
		username ? username : "",
		password ? password : "",
		options ? options : "",
		tty ? tty : "");

	conn = PQconnectdb(conninfo);
	if (!conn) return -1;

	if (PQstatus(conn) == CONNECTION_BAD) {
		PQfinish(conn);
		return -1;
	}
	else {
		driver->connection = (void *)conn;
	}
	
	return 0;
}

int dbd_disconnect(dbi_driver_t *driver) {
	if (driver->connection) PQfinish((PGconn *)driver->connection);
	return 0;
}


int dbd_fetch_row(dbi_result_t *result, unsigned int rownum) {
	/* XXX XXX XXX XXX XXX XXX XXX XXX */
	return -1; /* return -1 on error, 0 on no more rows, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	/* do whatever's necessary... */
	PQclear((PGresult *)result->result_handle);
	return 0;
}

int dbd_goto_row(dbi_driver_t *driver, unsigned int row) {
	/* libpq doesn't have to do anything, the row index is specified when
	 * fetching fields */
	return 1;
}

dbi_result_t *dbd_list_dbs(dbi_driver_t *driver) {
	return (dbi_result_t *)dbd_query(driver, "SELECT datname AS dbname FROM pg_database");
}

dbi_result_t *dbd_list_tables(dbi_driver_t *driver, const char *db) {
	return dbi_driver_query((dbi_driver)driver, "SELECT relname AS tablename FROM pg_class WHERE relname !~ '^pg_' AND relkind = 'r' AND relowner = (SELECT datdba FROM pg_database WHERE datname = '%s') ORDER BY relname", db);
}

dbi_result_t *dbd_query(dbi_driver_t *driver, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */
	
	dbi_result_t *result;
	PGresult *res;
	int resstatus;
	
	result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) {
		return NULL;
	}

	res = PQexec((PGconn *)driver->connection, statement);
	if (res) resstatus = PQresultStatus(res);
	if (!res || ((resstatus != PGRES_COMMAND_OK) && (resstatus != PGRES_TUPLES_OK))) {
		PQclear(res);
		free(result);
		return NULL;
	}
	
	result->result_handle = (void *)res;
	result->numrows_matched = PQntuples(res);
	result->numrows_affected = atoi(PQcmdTuples(res));
	
	return result;
}

char *dbd_select_db(dbi_driver_t *driver, const char *db) {
	/* postgresql doesn't support switching databases without reconnecting */
	return NULL;
}

int dbd_geterror(dbi_driver_t *driver, int *errno, char **errstr) {
	/* put error number into errno, error string into errstr
	 * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */
	*errno = 0;
	*errstr = strdup(PQerrorMessage((PGconn *)driver->connection));
	return 2;
}

