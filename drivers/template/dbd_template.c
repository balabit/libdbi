/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2002, David Parker and Mark Tobenkin.
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
 * dbd_template.c: Example template plugin for libdbi.
 * Copyright (C) 2001, Herbert Z. Bartholemew <hbz@bombdiggity.net>.
 * http://www.bombdiggity.net/~hzb/dbd_template/
 * 
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include "dbd_template.h"

dbi_info_t driver_info = {
	/* short name, used for loading drivers by name */
	"template",
	/* short desc, no more than two sentences, no newlines */
	"Example template plugin for FooBlah database server",
	/* plugin author/maintainer, email address */
	"Herbert Z. Bartholemew <hzb@bomb-diggity.net>",
	/* URL of plugin, if maintained by a third party */
	"http://www.bomb-diggity.net/~hzb/dbd_template/",
	/* plugin version */
	"dbd_template v0.00",
	/* compilation date */
	__DATE__
};

static const char *custom_functions_list[] = { "template_frob", "template_bork", NULL };
/* actual functions must be called dbd_template_frob and dbd_template_bork, but
 * this is how they are referred to when using dbi_custom_function() */

static const char *reserved_words_list[] = TEMPLATE_RESERVED_WORDS; /* this is defined in dbd_template.h */

void _translate_template_type(const int fieldtype, unsigned short *type, unsigned int *attribs);
void _get_field_info(dbi_result_t *result);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx);
time_t _parse_datetime(const char *raw, unsigned long attribs);


void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, const char ***_reserved_words) {
	/* this is the first function called after the plugin module is loaded into memory */
	*_plugin_info = &plugin_info;
	*_custom_functions = custom_functions;
	*_reserved_words = reserved_words;
}

int dbd_initialize(dbi_driver_t *driver) {
	/* perform any database-specific server initialization.
	 * this is called right after dbd_register_plugin().
	 * return -1 on error, 0 on success. if -1 is returned, the plugin will not
	 * be added to the list of available plugins. */
	
	return 0;
}

int dbd_connect(dbi_conn_t *conn) {
	/* void *conn; */
	
	const char *host = dbi_driver_get_option(driver, "host");
	const char *username = dbi_driver_get_option(driver, "username");
	const char *password = dbi_driver_get_option(driver, "password");
	const char *dbname = dbi_driver_get_option(driver, "dbname");
	int port = dbi_driver_get_option_numeric(driver, "port");
	
	/* plugin specific options */
	const char *option1 = dbi_driver_get_option(driver, "template_option1");
	int option2 = dbi_driver_get_option_numeric(driver, "template_option2");

	if (port == -1) port = 0; /* port not specified, use default */

	/*
	conn = XXX_open_connection(host, username, password, dbname, port, option1, option2);
	if (!conn) {
		_error_handler(driver);
		XXX_close_connection(conn);
		return -1;
	}
	else {
		driver->connection = (void *) conn;
		if (dbname) driver->current_db = strdup(dbname);
	}
	*/
	
	return 0;
}

int dbd_disconnect(dbi_conn_t *conn) {
	/*
	if (driver->connection) XXX_close_connection(driver->connection);
	*/
	
	return 0;
}

int dbd_fetch_row(dbi_result_t *result, unsigned int rownum) {
	dbi_row_t *row = NULL;

	if (result->result_state == NOTHING_RETURNED) return -1;
	
	if (result->result_state == ROWS_RETURNED) {
		/* this is the first time we've been here */
		
		/*
		_dbd_result_set_numfields(result, XXX_get_num_fields(result->result_handle));
		*/
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
	/*
	if (result->result_handle) XXX_free_result(result->result_handle);
	*/
	
	return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned int row) {
	/*
	XXX_seek_row(result->result_handle, row);
	*/
	return 1; /* return 0 if failed, or not supported */
}

int dbd_get_socket(dbi_conn_t *conn){

	return -1;
}

dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern) {
	/*
	return dbd_query(driver, "SELECT dbname FROM dbs");
	*/
	return NULL;
}

dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern) {
	/*
	return dbi_driver_query((dbi_driver)driver, "SELECT tablename FROM tables WHERE db = '%s'", db);
	*/
	return NULL;
}

int dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest) {
	/* foo's -> 'foo\'s' */
	
	/* dest is already allocated as (strlen(orig)*2)+4+1
	 * worst case, each char of orig will be escaped, with two quote characters
	 * added to both the beginning and end of ths string, plus one for the NULL */
	
	unsigned int len;
	
	strcpy(dest, "'");
	/*
	len = XXX_escape_string(dest, orig, strlen(orig));
	(if your database provides an escape function)
	
	OR (if it doesn't):
	
	const char *escaped = "\'\"\\"; XXX XXX XXX XXX XXX
	len = _dbd_escape_chars(dest, orig, escaped);
	*/
	strcat(dest, "'");
	
	return len+2;
}

dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */
	
	dbi_result_t *result;
	/* void *res; */
	
	/*
	if (XXX_query(driver->connection, statement)) {
		_error_handler(driver);
		return NULL;
	}
	*/
	
	/*
	res = XXX_store_result(driver->connection);
	*/
	
	if (!res) {
		_error_handler(driver);
		return NULL;
	}

	/*
	result = _dbd_result_create(driver, (void *)res, XXX_num_rows(res), XXX_affected_rows(res));
	*/

	return result;
}

dbi_result_t *dbd_query_null(dbi_conn_t *conn, const unsigned char *statement, unsigned long st_length) {
	return NULL;
}

char *dbd_select_db(dbi_conn_t *conn, const char *db) {
	/* if not supported, return NULL.
	 * if supported but there's an error, return "". */

	/*
	if (XXX_select_db(driver->connection, db)) {
		_error_handler(driver);
		return ""; 
	}
	*/

	return (char *)db;
}

int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr) {
	/* put error number into errno, error string into errstr
	 * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */
	
	if (!driver->connection) {
		*errno = 0;
		*errstr = strdup("Unable to connect to database");
		return 2;
	}
	
	/*

	*errno = XXX_errno(driver->connection);
	*errstr = strdup(XXX_error(driver->connection));

	*/
	
	return 3;
}

unsigned long long dbd_get_seq_last(dbi_conn_t *conn, const char *sequence) {
	return 0; 
}


unsigned long long dbd_get_seq_next(dbi_conn_t *conn, const char *sequence) {
	return 0;
}


/* CORE TEMPLATE-SPECIFIC FUNCTIONS */

void _translate_template_type(const int fieldtype, unsigned short *type, unsigned int *attribs) {
	unsigned int _type = 0;
	unsigned int _attribs = 0;

	/*
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
			
		case FIELD_TYPE_DATE:
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
			
		case FIELD_TYPE_DECIMAL:
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
	*/
	
	*type = _type;
	*attribs = _attribs;
}

void _get_field_info(dbi_result_t *result) {
	unsigned int idx = 0;
	/* void *field; */
	unsigned short fieldtype;
	unsigned int fieldattribs;

	/*
	field = XXX_fetch_fields(result->result_handle);
	*/
	
	while (idx < result->numfields) {
		_translate_template_type(field[idx].type, &fieldtype, &fieldattribs);
		if ((fieldtype == DBI_TYPE_INTEGER) && (field->flags & UNSIGNED_FLAG)) fieldattribs |= DBI_INTEGER_UNSIGNED;
		_dbd_result_add_field(result, idx, field[idx].name, fieldtype, fieldattribs);
		idx++;
	}
}

void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx) {
	/* void *_res = result->result_handle; */
	/* void *_row; */
	
	int curfield = 0;
	char *raw = NULL;
	unsigned long *strsizes = NULL;
	unsigned long sizeattrib;
	dbi_data_t *data;

	/*
	_row = XXX_fetch_row(_res);
	strsizes = XXX_fetch_lengths(_res);
	*/

	while (curfield < result->numfields) {
		raw = _row[curfield];
		data = &row->field_values[curfield];

		if (strsizes[curfield] == 0) {
			/* value is NULL, keep track of that and go to next field */
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
	unixtime.tm_mday = unixtime.tm_mon = unixtime.tm_year = 0;
	unixtime.tm_isdst = -1;
	
	if (attribs & DBI_DATETIME_DATE) {
		cur[4] = '\0';
		cur[7] = '\0';
		cur[10] = '\0';
		unixtime.tm_year = atoi(cur);
		unixtime.tm_mon = atoi(cur+5);
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

/**************************************
 * CUSTOM DATABASE-SPECIFIC FUNCTIONS *
 **************************************/

int dbd_template_frob() {
	/* custom database-specific function */
	return 123;
}

int dbd_template_bork() {
	/* custom database-specific function */
	return 321;
}

