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
 * Copyright (C) 2001, David A. Parker <david@neongoat.com>.
 * http://libdbi.sourceforge.net
 * 
 * $Id$
 */

#define _GNU_SOURCE /* we need asprintf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include <libpq-fe.h>
#include "pgsql-stuff.h"

static const dbi_info_t plugin_info = {
	"pgsql",
	"PostgreSQL database support (using libpq)",
	"David A. Parker <david@neongoat.com>",
	"http://libdbi.sourceforge.net",
	"dbd_pgsql v0.02",
	__DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = PGSQL_RESERVED_WORDS;

void _translate_postgresql_type(unsigned int oid, unsigned short *type, unsigned int *attribs);
void _get_field_info(dbi_result_t *result);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx);

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
	char *conninfo_kludge;

	if (port > 0) asprintf(&port_str, "%d", port);
	else port_str = NULL;

	/* YUCK YUCK YUCK YUCK YUCK. stupid libpq. */
	if (host && port_str) asprintf(&conninfo_kludge, "host='%s' port='%s'", host, port_str);
	else if (host) asprintf(&conninfo_kludge, "host='%s'", host);
	else if (port_str) asprintf(&conninfo_kludge, "port='%s'", port_str);
	else conninfo_kludge = NULL;
	
	asprintf(&conninfo, "%s dbname='%s' user='%s' password='%s' options='%s' tty='%s'",
		conninfo_kludge ? conninfo_kludge : "", /* if we pass a NULL directly to the %s it will show up as "(null)" */
		dbname ? dbname : "",
		username ? username : "",
		password ? password : "",
		options ? options : "",
		tty ? tty : "");

	conn = PQconnectdb(conninfo);
	if (!conn) return -1;

	if (PQstatus(conn) == CONNECTION_BAD) {
		_error_handler(driver);
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
	dbi_row_t *row = NULL;

	if (result->result_state == NOTHING_RETURNED) return -1;
	
	if (result->result_state == ROWS_RETURNED) {
		/* this is the first time we've been here */
		_dbd_result_set_numfields(result, PQnfields((PGresult *)result->result_handle));
		_get_field_info(result);
		result->result_state = GETTING_ROWS;
	}

	/* get row here */
	row = _dbd_row_allocate(result->numfields);
	_get_row_data(result, row, rownum);
	_dbd_row_finalize(result, row, rownum);
	
	return 1; /* 0 on error, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	PQclear((PGresult *)result->result_handle);
	return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned int row) {
	/* libpq doesn't have to do anything, the row index is specified when
	 * fetching fields */
	return 1;
}

dbi_result_t *dbd_list_dbs(dbi_driver_t *driver) {
	return dbd_query(driver, "SELECT datname AS dbname FROM pg_database");
}

dbi_result_t *dbd_list_tables(dbi_driver_t *driver, const char *db) {
	return (dbi_result_t *)dbi_driver_query((dbi_driver)driver, "SELECT relname AS tablename FROM pg_class WHERE relname !~ '^pg_' AND relkind = 'r' AND relowner = (SELECT datdba FROM pg_database WHERE datname = '%s') ORDER BY relname", db);
}

int dbd_quote_string(dbi_plugin_t *plugin, const char *orig, char *dest) {
	/* foo's -> 'foo\'s' */
	const char *escaped = "'\"\\"; // XXX TODO check if this is right
	char *curdest = dest;
	const char *curorig = orig;
	const char *curescaped;
	
	strcpy(dest, "'");
	strcat(dest, orig);

	while (curorig) {
		curescaped = escaped;
		while (curescaped) {
			if (*curorig == *curescaped) {
				memmove(curdest+1, curorig, strlen(curorig)+1);
				*curdest = '\\';
				curdest++;
				continue;
			}
			curescaped++;
		}
		curorig++;
		curdest++;
	}

	strcat(dest, "'");
	
	return strlen(dest);
}

dbi_result_t *dbd_query(dbi_driver_t *driver, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */
	
	dbi_result_t *result;
	PGresult *res;
	int resstatus;
	
	res = PQexec((PGconn *)driver->connection, statement);
	if (res) resstatus = PQresultStatus(res);
	if (!res || ((resstatus != PGRES_COMMAND_OK) && (resstatus != PGRES_TUPLES_OK))) {
		PQclear(res);
		return NULL;
	}

	result = _dbd_result_create(driver, (void *)res, PQntuples(res), atol(PQcmdTuples(res)));

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
	if (!driver->connection) *errstr = strdup("Unable to connect to database");
	else *errstr = strdup(PQerrorMessage((PGconn *)driver->connection));
	
	return 2;
}

/* CORE POSTGRESQL DATA FETCHING STUFF */

void _translate_postgresql_type(unsigned int oid, unsigned short *type, unsigned int *attribs) {
	unsigned int _type = 0;
	unsigned int _attribs = 0;
	
	switch (oid) {
		case PG_TYPE_CHAR:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE1;
			break;
		case PG_TYPE_INT2:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE2;
			break;
		case PG_TYPE_INT4:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE4;
			break;
		case PG_TYPE_INT8:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE8;
			break;
		case PG_TYPE_OID:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE8;
			_attribs |= DBI_INTEGER_UNSIGNED;
			break;
			
		case PG_TYPE_FLOAT4:
			_type = DBI_TYPE_DECIMAL;
			_attribs |= DBI_DECIMAL_SIZE4;
			break;
		case PG_TYPE_FLOAT8:
			_type = DBI_TYPE_DECIMAL;
			_attribs |= DBI_DECIMAL_SIZE8;
			break;

		case PG_TYPE_NAME:
		case PG_TYPE_TEXT:
		case PG_TYPE_CHAR2:
		case PG_TYPE_CHAR4:
		case PG_TYPE_CHAR8:
		case PG_TYPE_BPCHAR:
		case PG_TYPE_VARCHAR:
			_type = DBI_TYPE_STRING;
			break;

		case PG_TYPE_BYTEA:
			_type = DBI_TYPE_BINARY;
			break;
			
		default:
			_type = DBI_TYPE_STRING;
			break;
	}
	
	*type = _type;
	*attribs = _attribs;
}

void _get_field_info(dbi_result_t *result) {
	unsigned int idx = 0;
	unsigned int pgOID = 0;
	char *fieldname;
	unsigned short fieldtype;
	unsigned int fieldattribs;
	
	while (idx < result->numfields) {
		pgOID = PQftype((PGresult *)result->result_handle, idx);
		fieldname = PQfname((PGresult *)result->result_handle, idx);
		_translate_postgresql_type(pgOID, &fieldtype, &fieldattribs);
		_dbd_result_add_field(result, idx, fieldname, fieldtype, fieldattribs);
		idx++;
	}
}

void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx) {
	int curfield = 0;
	char *raw = NULL;
	int strsize = 0;
	unsigned long sizeattrib;
	dbi_data_t *data;

	while (curfield < result->numfields) {
		raw = PQgetvalue((PGresult *)result->result_handle, rowidx, curfield);
		strsize = PQfmod((PGresult *)result->result_handle, curfield);
		data = &row->field_values[curfield];

		if (PQgetisnull((PGresult *)result->result_handle, rowidx, curfield) == 1) {
			row->field_sizes[curfield] = -1;
			curfield++;
			continue;
		}			
		
		switch (result->field_types[curfield]) {
			case DBI_TYPE_INTEGER:
				sizeattrib = _dbd_isolate_attrib(result->field_attribs[curfield], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);
				switch (sizeattrib) {
					case DBI_INTEGER_SIZE1:
						data->d_char = (char) atol(raw); break;
					case DBI_INTEGER_SIZE2:
						data->d_short = (short) atol(raw); break;
					case DBI_INTEGER_SIZE3:
					case DBI_INTEGER_SIZE4:
						data->d_long = (long) atol(raw); break;
					case DBI_INTEGER_SIZE8:
						data->d_longlong = (long long) atoll(raw); break; /* hah, wonder if that'll work */
					default:
						break;
				}
				break;
			case DBI_TYPE_DECIMAL:
				sizeattrib = _dbd_isolate_attrib(result->field_attribs[curfield], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);
				switch (sizeattrib) {
					case DBI_DECIMAL_SIZE4:
						data->d_float = (float) strtod(raw, NULL); break;
					case DBI_DECIMAL_SIZE8:
						data->d_double = (double) strtod(raw, NULL); break;
					default:
						break;
				}
				break;
			case DBI_TYPE_STRING:
				data->d_string = strdup(raw);
				row->field_sizes[curfield] = strsize;
				break;
			case DBI_TYPE_BINARY:
				row->field_sizes[curfield] = strsize;
				memcpy(data->d_string, raw, strsize);
				break;
				
			case DBI_TYPE_ENUM:
			case DBI_TYPE_SET:
			case DBI_TYPE_DATETIME:
			default:
				break;
		}
		
		curfield++;
	}
}

