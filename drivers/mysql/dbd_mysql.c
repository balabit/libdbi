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
 * dbd_mysql.c: MySQL database support (using libpq)
 * Copyright (C) 2001, Mark Tobenkin <mark@brentwoodradio.com>
 * http://libdbi.sourceforge.net
 * 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE /* we need asprintf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include <mysql/mysql.h>
#include "mysql-stuff.h"

static const dbi_info_t driver_info = {
	"mysql",
	"MySQL database support (using libmysqlclient6)",
	"Mark M. Tobenkin <mark@brentwoodradio.com>",
	"http://libdbi.sourceforge.net",
	"dbd_mysql v" VERSION,
	__DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = MYSQL_RESERVED_WORDS;

void _translate_mysql_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs);
void _get_field_info(dbi_result_t *result);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx);
time_t _parse_datetime(const char *raw, unsigned long attribs);

void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, const char ***_reserved_words) {
	/* this is the first function called after the driver module is loaded into memory */
	*_driver_info = &driver_info;
	*_custom_functions = custom_functions;
	*_reserved_words = reserved_words;
}

int dbd_initialize(dbi_driver_t *driver) {
	/* perform any database-specific server initialization.
	 * this is called right after dbd_register_driver().
	 * return -1 on error, 0 on success. if -1 is returned, the driver will not
	 * be added to the list of available drivers. */
	
	return 0;
}

int dbd_connect(dbi_conn_t *conn) {
	MYSQL *mycon;
	
	const char *host = dbi_conn_get_option(conn, "host");
	const char *username = dbi_conn_get_option(conn, "username");
	const char *password = dbi_conn_get_option(conn, "password");
	const char *dbname = dbi_conn_get_option(conn, "dbname");
	int port = dbi_conn_get_option_numeric(conn, "port");
	/* mysql specific options */
	const char *unix_socket = dbi_conn_get_option(conn, "mysql_unix_socket");
	int compression = dbi_conn_get_option_numeric(conn, "mysql_compression");

	int _compression = (compression > 0) ? CLIENT_COMPRESS : 0;
	
	if (port == -1) port = 0;

	mycon = mysql_init(NULL);
	if (!mycon || !mysql_real_connect(mycon , host, username, password, dbname, port, unix_socket, _compression)) {
		_dbd_internal_error_handler(conn, "Unable to connect to database server", 0);
		mysql_close(mycon);
		return -1;
	}
	else {
		conn->connection = (void *)mycon;
		if (dbname) conn->current_db = strdup(dbname);
	}
	
	return 0;
}

int dbd_disconnect(dbi_conn_t *conn) {
	if (conn->connection) mysql_close((MYSQL *)conn->connection);
	return 0;
}

int dbd_fetch_row(dbi_result_t *result, unsigned int rownum) {
	dbi_row_t *row = NULL;

	if (result->result_state == NOTHING_RETURNED) return -1;
	
	if (result->result_state == ROWS_RETURNED) {
		/* this is the first time we've been here */
		_dbd_result_set_numfields(result, mysql_num_fields((MYSQL_RES *)result->result_handle));
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
	if (result->result_handle) mysql_free_result((MYSQL_RES *)result->result_handle);
	return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned int row) {
	// XXX TODO: kosherize this, handle efficient queries.
	mysql_data_seek((MYSQL_RES *)result->result_handle, row);
	return 1;
}

int dbd_get_socket(dbi_conn_t *conn){
	MYSQL *mycon = (MYSQL*)conn->connection;

	if(!mycon) return -1;

	return (int)mycon->net.fd;
}

dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern) {
	dbi_result_t *res;
	char *sql_cmd;

	if (pattern == NULL) {
		return dbd_query(conn, "SHOW DATABASES");
	}
	else {
		asprintf(&sql_cmd, "SHOW DATABASES LIKE '%s'", pattern);
		res = dbd_query(conn, sql_cmd);
		free(sql_cmd);
		return res;
	}
}

dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern) {
	dbi_result_t *res;
	char *sql_cmd;

	if (db == NULL || db[0] == '\0') {
		return dbd_query(conn, "SHOW TABLES");
	}

	if (pattern == NULL) {
		asprintf(&sql_cmd, "SHOW TABLES FROM %s", db);
		res = dbd_query(conn, sql_cmd);
		free(sql_cmd);
		return res;
	}
	else {
		asprintf(&sql_cmd, "SHOW TABLES FROM %s LIKE '%s'", db, pattern);
		res = dbd_query(conn, sql_cmd);
		free(sql_cmd);
		return res;
	}
}

int dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest) {
	/* foo's -> 'foo\'s' */
	unsigned int len;
	
	strcpy(dest, "'");
	len = mysql_escape_string(dest, orig, strlen(orig));	
	strcat(dest, "'");
	
	return len+2;
}

dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */
	
	dbi_result_t *result;
	MYSQL_RES *res;
	
	if (mysql_query((MYSQL *)conn->connection, statement)) {
		_error_handler(conn);
		return NULL;
	}
	
	res = mysql_store_result((MYSQL *)conn->connection);
	
	if (!res) {
		_error_handler(conn);
		return NULL;
	}

	result = _dbd_result_create(conn, (void *)res, mysql_num_rows(res), mysql_affected_rows((MYSQL *)conn->connection));

	return result;
}

dbi_result_t *dbd_query_null(dbi_conn_t *conn, const unsigned char *statement, unsigned long st_length) {
	dbi_result_t *result;
	MYSQL_RES *res;
	
	if (mysql_real_query((MYSQL *)conn->connection, statement, st_length)) {
		_error_handler(conn);
		return NULL;
	}
	
	res = mysql_store_result((MYSQL *)conn->connection);
	
	if (!res) {
		_error_handler(conn);
		return NULL;
	}

	result = _dbd_result_create(conn, (void *)res, mysql_num_rows(res), mysql_affected_rows((MYSQL *)conn->connection));

	return result;
}

char *dbd_select_db(dbi_conn_t *conn, const char *db) {
	if (mysql_select_db((MYSQL *)conn->connection, db)) {
		_error_handler(conn);
		return "";
	}

	return (char *)db;
}

int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr) {
	/* put error number into errno, error string into errstr
	 * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */

	if (strcmp("",mysql_error((MYSQL *)conn->connection)) == 0) {
		return -1;
	}
	
	if (!conn->connection) {
		_dbd_internal_error_handler(conn, "No connection found", 0);
		return 2;
	}
	
	*errno = mysql_errno((MYSQL *)conn->connection);
	*errstr = strdup(mysql_error((MYSQL *)conn->connection));
	return 3;
}

/* CORE MYSQL DATA FETCHING STUFF */

void _translate_mysql_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs) {
	unsigned int _type = 0;
	unsigned int _attribs = 0;

	switch (fieldtype) {
		case FIELD_TYPE_TINY:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE1;
			break;
		case FIELD_TYPE_YEAR:
			_attribs |= DBI_INTEGER_UNSIGNED;
		case FIELD_TYPE_SHORT:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE2;
			break;
		case FIELD_TYPE_INT24:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE3;
			break;
		case FIELD_TYPE_LONG:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE4;
			break;
		case FIELD_TYPE_LONGLONG:
			_type = DBI_TYPE_INTEGER;
			_attribs |= DBI_INTEGER_SIZE8;
			break;
			
		case FIELD_TYPE_FLOAT:
			_type = DBI_TYPE_DECIMAL;
			_attribs |= DBI_DECIMAL_SIZE4;
			break;
		case FIELD_TYPE_DOUBLE:
			_type = DBI_TYPE_DECIMAL;
			_attribs |= DBI_DECIMAL_SIZE8;
			break;
			
		case FIELD_TYPE_DATE: /* TODO parse n stuph to native DBI unixtime type. for now, string */
			_type = DBI_TYPE_DATETIME;
			_attribs |= DBI_DATETIME_DATE;
			break;
		case FIELD_TYPE_TIME:
			_type = DBI_TYPE_DATETIME;
			_attribs |= DBI_DATETIME_TIME;
			break;
		case FIELD_TYPE_DATETIME:
		case FIELD_TYPE_TIMESTAMP:
			_type = DBI_TYPE_DATETIME;
			_attribs |= DBI_DATETIME_DATE;
			_attribs |= DBI_DATETIME_TIME;
			break;
			
		case FIELD_TYPE_DECIMAL: /* decimal is actually a string, has arbitrary precision, no floating point rounding */
		case FIELD_TYPE_ENUM:
		case FIELD_TYPE_SET:
		case FIELD_TYPE_VAR_STRING:
		case FIELD_TYPE_STRING:
			_type = DBI_TYPE_STRING;
			break;
			
		case FIELD_TYPE_TINY_BLOB:
		case FIELD_TYPE_MEDIUM_BLOB:
		case FIELD_TYPE_LONG_BLOB:
		case FIELD_TYPE_BLOB:
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
	MYSQL_FIELD *field;
	unsigned short fieldtype;
	unsigned int fieldattribs;

	field = mysql_fetch_fields((MYSQL_RES *)result->result_handle);
	
	while (idx < result->numfields) {
		_translate_mysql_type(field[idx].type, &fieldtype, &fieldattribs);
		if ((fieldtype == DBI_TYPE_INTEGER) && (field->flags & UNSIGNED_FLAG)) fieldattribs |= DBI_INTEGER_UNSIGNED;
		_dbd_result_add_field(result, idx, field[idx].name, fieldtype, fieldattribs);
		idx++;
	}
}

void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx) {
	MYSQL_RES *_res = result->result_handle;
	MYSQL_ROW _row;
	
	int curfield = 0;
	char *raw = NULL;
	unsigned long *strsizes = NULL;
	unsigned long sizeattrib;
	dbi_data_t *data;

	_row = mysql_fetch_row(_res);
	strsizes = mysql_fetch_lengths(_res);

	while (curfield < result->numfields) {
		raw = _row[curfield];
		data = &row->field_values[curfield];

		if (strsizes[curfield] == 0) {
			row->field_sizes[curfield] = 0;
			curfield++;
			continue;
		}
		else {
			row->field_sizes[curfield] = -1;
			/* this will be set to the string size later on if the field is indeed a string */
		}
		
		switch (result->field_types[curfield]) {
			case DBI_TYPE_INTEGER:
				sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);
				switch (sizeattrib) {
					case DBI_INTEGER_SIZE1:
						data->d_char = (char) atol(raw); break;
					case DBI_INTEGER_SIZE2:
						data->d_short = (short) atol(raw); break;
					case DBI_INTEGER_SIZE3:
					case DBI_INTEGER_SIZE4:
						data->d_long = (long) atol(raw); break;
					case DBI_INTEGER_SIZE8:
						data->d_longlong = (long long) atoll(raw); break;
					default:
						break;
				}
				break;
			case DBI_TYPE_DECIMAL:
				sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);
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
				row->field_sizes[curfield] = strsizes[curfield];
				break;
			case DBI_TYPE_BINARY:
				row->field_sizes[curfield] = strsizes[curfield];
				memcpy(data->d_string, raw, strsizes[curfield]);
				break;
			case DBI_TYPE_DATETIME:
				sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_DATETIME_DATE, DBI_DATETIME_TIME);
				data->d_datetime = _parse_datetime(raw, sizeattrib);
				break;
				
			case DBI_TYPE_ENUM:
			case DBI_TYPE_SET:
			default:
				data->d_string = strdup(raw);
				row->field_sizes[curfield] = strsizes[curfield];
				break;
		}
		
		curfield++;
	}
}

time_t _parse_datetime(const char *raw, unsigned long attribs) {
	struct tm unixtime;
	char *unparsed = strdup(raw);
	char *cur = unparsed;

	unixtime.tm_sec = unixtime.tm_min = unixtime.tm_hour = 0;
	unixtime.tm_mday = 1;
	unixtime.tm_mon = 0;
	unixtime.tm_year = 70;
	unixtime.tm_isdst = -1;
	
	if (attribs & DBI_DATETIME_DATE) {
		cur[4] = '\0';
		cur[7] = '\0';
		cur[10] = '\0';
		unixtime.tm_year = atoi(cur)-1900;
		unixtime.tm_mon = atoi(cur+5)-1;
		unixtime.tm_mday = atoi(cur+8);
		if (attribs & DBI_DATETIME_TIME) cur = cur+11;
	}
	
	if (attribs & DBI_DATETIME_TIME) {
		cur[2] = '\0';
		cur[5] = '\0';
		unixtime.tm_hour = atoi(cur);
		unixtime.tm_min = atoi(cur+3);
		unixtime.tm_sec = atoi(cur+6);
	}

	free(unparsed);
	return mktime(&unixtime);
}

