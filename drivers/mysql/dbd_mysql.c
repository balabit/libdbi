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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include <mysql/mysql.h>

static const dbi_info_t plugin_info = {
	"mysql",
	"MySQL database support (using libmysql-dev)",
	"Mark M. Tobenkin <mark@brentwoodradio.com>",
	"http://libdbi.sourceforge.net",
	"dbd_mysql v0.01",
	__DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = { "ACTION", "ADD", "AGGREGATE", "ALL", "ALTER", "AFTER", "AND", "AS", "ASC", "AVG", "AVG_ROW_LENGTH", "AUTO_INCREMENT", "BETWEEN", "BIGINT", "BIT", "BINARY", "BLOB", "BOOL", "BOTH", "BY", "CASCADE", "CASE", "CHAR", "CHARACTER", "CHANGE", "CHECK", "CHECKSUM", "COLUMN", "COLUMNS", "COMMENT", "CONSTRAINT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "DATA", "DATABASE", "DATABASES", "DATE", "DATETIME", "DAY", "DAY_HOUR", "DAY_MINUTE", "DAY_SECOND", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "DEC", "DECIMAL", "DEFAULT", "DELAYED", "DELAY_KEY_WRITE", "DELETE", "DESC", "DESCRIBE", "DISTINCT", "DISTINCTROW", "DOUBLE", "DROP", "END", "ELSE", "ESCAPE", "ESCAPED", "ENCLOSED", "ENUM", "EXPLAIN", "EXISTS", "FIELDS", "FILE", "FIRST", "FLOAT", "FLOAT4", "FLOAT8", "FLUSH", "FOREIGN", "FROM", "FOR", "FULL", "FUNCTION", "GLOBAL", "GRANT", "GRANTS", "GROUP", "HAVING", "HEAP", "HIGH_PRIORITY", "HOUR", "HOUR_MINUTE", "HOUR_SECOND", "HOSTS", "IDENTIFIED", "IGNORE", "IN", "INDEX", "INFILE", "INNER", "INSERT", "INSERT_ID", "INT", "INTEGER", "INTERVAL", "INT1", "INT2", "INT3", "INT4", "INT8", "INTO", "IF", "IS", "ISAM", "JOIN", "KEY", "KEYS", "KILL", "LAST_INSERT_ID", "LEADING", "LEFT", "LENGTH", "LIKE", "LINES", "LIMIT", "LOAD", "LOCAL", "LOCK", "LOGS", "LONG", "LONGBLOB", "LONGTEXT", "LOW_PRIORITY", "MAX", "MAX_ROWS", "MATCH", "MEDIUMBLOB", "MEDIUMTEXT", "MEDIUMINT", "MIDDLEINT", "MIN_ROWS", "MINUTE", "MINUTE_SECOND", "MODIFY", "MONTH", "MONTHNAME", "MYISAM", "NATURAL", "NUMERIC", "NO", "NOT", "NULL", "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUTER", "OUTFILE", "PACK_KEYS", "PARTIAL", "PASSWORD", "PRECISION", "PRIMARY", "PROCEDURE", "PROCESS", "PROCESSLIST", "PRIVILEGES", "READ", "REAL", "REFERENCES", "RELOAD", "REGEXP", "RENAME", "REPLACE", "RESTRICT", "RETURNS", "REVOKE", "RLIKE", "ROW", "ROWS", "SECOND", "SELECT", "SET", "SHOW", "SHUTDOWN", "SMALLINT", "SONAME", "SQL_BIG_TABLES", "SQL_BIG_SELECTS", "SQL_LOW_PRIORITY_UPDATES", "SQL_LOG_OFF", "SQL_LOG_UPDATE", "SQL_SELECT_LIMIT", "SQL_SMALL_RESULT", "SQL_BIG_RESULT", "SQL_WARNINGS", "STRAIGHT_JOIN", "STARTING", "STATUS", "STRING", "TABLE", "TABLES", "TEMPORARY", "TERMINATED", "TEXT", "THEN", "TIME", "TIMESTAMP", "TINYBLOB", "TINYTEXT", "TINYINT", "TRAILING", "TO", "TYPE", "USE", "USING", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE", "VALUES", "VARCHAR", "VARIABLES", "VARYING", "VARBINARY", "WITH", "WRITE", "WHEN", "WHERE", "YEAR", "YEAR_MONTH", "ZEROFILL", NULL };

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
	MYSQL *mycon; /* The Connection To The MySQL Server */

	/* These Are Variables Preloaded Into The Driver  */
	const char *host = dbi_driver_get_option(driver, "host");
        const char *username = dbi_driver_get_option(driver, "username");
        const char *password = dbi_driver_get_option(driver, "password");
        const char *database = dbi_driver_get_option(driver, "database");
        int port = dbi_driver_get_option_numeric(driver, "port");

	/* Initialize Connection */
	mycon = mysql_init(NULL);

	if(mycon == NULL){ /* Failure, Memory Problems */
		_specific_error_handler(driver, "Not Enough Memory");
		return -1;
	}

	/* Attempt To Make Connection, Give Error On Failure */
	if( mysql_real_connect(mycon, host, username, password, database, port, NULL, 0) ){
		driver->connection = (void *) mycon;
		
		driver->current_db = strdup(driver->current_db);

		return 0;
	} else {
		_error_handler(driver);

		mysql_close(mycon);

		return -1;
	}
}

int dbd_disconnect(dbi_driver_t *driver) {
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */

	if(mycon){
		mysql_close(mycon);
		driver->connection = NULL;
		
		return 0;
	} else {

		_specific_error_handler(driver, "Invalid MySQL Identifier");
		
		return -1;
	}
}

int dbd_fetch_row(dbi_result_t *result, unsigned int rownum) {
	dbi_row_t *row = NULL;

	if (result->result_state == NOTHING_RETURNED) return -1;
	
	if (result->result_state == ROWS_RETURNED) {
		/* this is the first time we've been here */
		_dbd_result_set_numfields(result, mysql_num_fields((MYSQL_RES*)result->result_handle));
		_get_field_info(result);
		result->result_state = GETTING_ROWS;
	}

	/* get row here */
	row = _dbd_row_allocate(result->numfields, result->has_string_fields);
	_get_row_data(result, row, rownum);
	_dbd_row_finalize(result, row, rownum);
	
	return 1; /* 0 on error, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	if(result->result_handle) mysql_free_result((MYSQL_RES*)result->result_handle);

	return 0;
}

int dbd_goto_row(dbi_driver_t *driver, unsigned int row) {
	/* libpq doesn't have to do anything, the row index is specified when
	 * fetching fields */
	return 1;
}

dbi_result_t *dbd_list_dbs(dbi_driver_t *driver) {
	return dbd_query(driver, "show databases");
}

dbi_result_t *dbd_list_tables(dbi_driver_t *driver, const char *db) {
	/*return (dbi_result_t *)dbi_driver_query((dbi_driver)driver, "SELECT relname AS tablename FROM pg_class WHERE relname !~ '^pg_' AND relkind = 'r' AND relowner = (SELECT datdba FROM pg_database WHERE datname = '%s') ORDER BY relname", db);*/
	return dbd_query(driver, "show tables");
	/* my (mmt) thought is to add a function:
	 * 	char *dbi_result_get_column(result, int); which will return
	 * 		the name of a certain column, then use this:
	 * 	dbi_result_get_string(result,
	 * 			      dbi_result_get_column(result, 0))
	 *	rather than having to rename the column heading*/
}

dbi_result_t *dbd_query(dbi_driver_t *driver, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */

	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *myres; /* MySQL's internal result type */
	dbi_result_t *result; /* DBI's internal result type*/

	/* Query, On Failure Return NULL */
	if(mysql_query(mycon, statement)){
		/* driver->error_message = strdup(driver->error_message, mysql_error(con)); */
		/* driver->error_number = mysql_errno(con); */
		_error_handler(driver);
	
		return NULL;
	}

	myres = mysql_store_result(mycon); /* Grab Result*/

	result = _dbd_result_create(driver, (void *)myres,
			mysql_num_rows(myres),
			mysql_affected_rows(mycon));

	return result;
}

char *dbd_select_db(dbi_driver_t *driver, const char *db) {
	/* postgresql doesn't support switching databases without reconnecting */
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */

	if(mysql_select_db(mycon, db)){ /* In Case Of Error */
		_error_handler(driver);

		return "";
	}


	return (char*)db;

}

int dbd_geterror(dbi_driver_t *driver, int *errno, char **errstr) {
	/* put error number into errno, error string into errstr
	 * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */
	*errno = 0;
	
	*errstr = strdup(mysql_error((MYSQL*)driver->connection));

	return 2;
}

unsigned short _map_type( enum enum_field_types mytype)
{
	unsigned short type = 0;
	
	switch(mytype){
		case FIELD_TYPE_YEAR:
		case FIELD_TYPE_NULL:
		case FIELD_TYPE_TINY:
		case FIELD_TYPE_SHORT:
		case FIELD_TYPE_LONG:
		case FIELD_TYPE_INT24:
		case FIELD_TYPE_TIMESTAMP:
		case FIELD_TYPE_LONGLONG: 
			type = DBI_TYPE_INTEGER;
		break;
		
		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
			type = DBI_TYPE_DECIMAL;
		break;
		
		case FIELD_TYPE_DATE:
		case FIELD_TYPE_NEWDATE:
		case FIELD_TYPE_TIME:
		case FIELD_TYPE_DATETIME:
		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
		case FIELD_TYPE_BLOB:
		case FIELD_TYPE_TINY_BLOB:
		case FIELD_TYPE_MEDIUM_BLOB:
		case FIELD_TYPE_LONG_BLOB:
		case FIELD_TYPE_SET:
		case FIELD_TYPE_ENUM:
			type = DBI_TYPE_STRING;
		break;

	}

	return type;
}

/*****************************************************************************/
/* _MAP_TYPE_ATTRIBUTES                                                      */
/*****************************************************************************/
/*
 * Precondition: none.
 * Postcondition: none.
 * Returns: The DBI type attributes associated with MySQL type mytype.
 */

unsigned short _map_type_attributes( enum enum_field_types mytype )
{
	unsigned short attb=0;

	switch(mytype){
		case FIELD_TYPE_YEAR:
		case FIELD_TYPE_NULL:
			attb |= DBI_INTEGER_UNSIGNED;
		case FIELD_TYPE_TINY:
		case FIELD_TYPE_SHORT:
			attb |= DBI_INTEGER_SIZE2;
		break;

		case FIELD_TYPE_LONG:
		case FIELD_TYPE_INT24:
		case FIELD_TYPE_TIMESTAMP:
			attb |= DBI_INTEGER_SIZE4;
		break;

		case FIELD_TYPE_LONGLONG:
			attb |= DBI_INTEGER_SIZE8;
		break;

		case FIELD_TYPE_FLOAT:
			attb |= DBI_DECIMAL_SIZE4;
		break;
		
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
			attb |= DBI_DECIMAL_SIZE8;
		break;
		
		/* Add Decimal Sizes Later */
		default:
	}

	return attb;
}

void _translate_mysql_type(enum enum_field_types mytype, unsigned short *type, unsigned int *attb) {
	*type = _map_type(mytype);
	*attb = _map_type_attributes(mytype);
}

void _get_field_info(dbi_result_t *result) {
	unsigned int idx = 0;
	MYSQL_FIELD *myfield;
	char *fieldname;
	unsigned short fieldtype;
	unsigned int fieldattribs;

	myfield = mysql_fetch_fields((MYSQL_RES*) result->result_handle);
	
	while (idx < result->numfields) {
		fieldname = myfield[idx].name;
		_translate_mysql_type(myfield[idx].type, &fieldtype, &fieldattribs);
		_dbd_result_add_field(result, idx, fieldname, fieldtype, fieldattribs);
		idx++;
	}
}

void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx) {
	dbi_driver_t *driver = result->driver;
	MYSQL *mycon = (MYSQL*) driver->connection;
	MYSQL_RES *myres = (MYSQL_RES *) result->result_handle;
	MYSQL_ROW *myrow = NULL;

	long *len = NULL;
	char *string = NULL;
	int i = 0;
	dbi_data_t *data;

	myrow = (MYSQL_ROW*) malloc(sizeof(MYSQL_ROW));
	*myrow = mysql_fetch_row(myres);

	len = mysql_fetch_lengths(myres);

	while ( i< result->numrows_matched) {
		string = (char*) malloc(sizeof(char) * (len[i] + 1));

		string[len[i]] = '\0';

		memcpy((void*)string, *(myrow)[i], len[i]);

		data = &row->field_values[i];
	
		switch (result->field_types[i]) {
			case DBI_TYPE_INTEGER:
				switch (result->field_attribs[i]) {
					case DBI_INTEGER_SIZE1:
						data->d_char = (char) atol(string); break;
					case DBI_INTEGER_SIZE2:
						data->d_short = (short) atol(string); break;
					case DBI_INTEGER_SIZE3:
					case DBI_INTEGER_SIZE4:
						data->d_long = (long) atol(string); break;
					case DBI_INTEGER_SIZE8:
						data->d_longlong = (long long) atoll(string); break; /* hah, wonder if that'll work */
					default:
						break;
				}
				break;
			case DBI_TYPE_DECIMAL:
				switch (result->field_attribs[i]) {
					case DBI_DECIMAL_SIZE4:
						data->d_float = (float) strtod(string, NULL); break;
					case DBI_DECIMAL_SIZE8:
						data->d_double = (double) strtod(string, NULL); break;
					default:
						break;
				}
				break;
			case DBI_TYPE_STRING:
				data->d_string = strdup(string);
				if (row->field_sizes) row->field_sizes[i] = len[i];
				break;
			case DBI_TYPE_BINARY:
				if (row->field_sizes) row->field_sizes[i] = len[i];
				memcpy(data->d_string, string, len[i]);
				break;
				
			case DBI_TYPE_ENUM:
			case DBI_TYPE_SET:
			default:
				break;
		}

		free(string);

		i++;
	}
}

