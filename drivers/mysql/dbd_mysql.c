/*
 * Data types:
 * 	TINYINT		-- short
 * 	SMALLINT	-- short
 * 	YEAR		-- unsigned short
 * 	NULL		-- unsigned short
 * 	INTEGER		-- long
 * 	MEDIUMINT	-- long
 * 	TIMESTAMP	-- long
 * 	BIGINT		-- double
 * 	DECIMAL		-- double
 * 	FLOAT		-- double
 * 	DOUBLE		-- double
 * 	DATE		-- char*
 * 	TIME		-- char*
 * 	DATETIME	-- char*
 * 	STRING		-- char*
 * 	BLOB		-- char*
 * 	SET		-- char*
 * 	ENUM		-- char*
 */

/* Standard Libraries  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* For return/parameter Types  */
#include <dbi/dbi.h>

/* Database Specific Libraries  */
#include <mysql/mysql.h>


/* No Custom Functions */
static const char *custom_function_list[] = {NULL};

/* Reserved Words */
static const char *reserved_word_list[] = { "ACTION", "ADD", "AGGREGATE", "ALL", "ALTER", "AFTER", "AND", "AS", "ASC", "AVG", "AVG_ROW_LENGTH", "AUTO_INCREMENT", "BETWEEN", "BIGINT", "BIT", "BINARY", "BLOB", "BOOL", "BOTH", "BY", "CASCADE", "CASE", "CHAR", "CHARACTER", "CHANGE", "CHECK", "CHECKSUM", "COLUMN", "COLUMNS", "COMMENT", "CONSTRAINT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "DATA", "DATABASE", "DATABASES", "DATE", "DATETIME", "DAY", "DAY_HOUR", "DAY_MINUTE", "DAY_SECOND", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "DEC", "DECIMAL", "DEFAULT", "DELAYED", "DELAY_KEY_WRITE", "DELETE", "DESC", "DESCRIBE", "DISTINCT", "DISTINCTROW", "DOUBLE", "DROP", "END", "ELSE", "ESCAPE", "ESCAPED", "ENCLOSED", "ENUM", "EXPLAIN", "EXISTS", "FIELDS", "FILE", "FIRST", "FLOAT", "FLOAT4", "FLOAT8", "FLUSH", "FOREIGN", "FROM", "FOR", "FULL", "FUNCTION", "GLOBAL", "GRANT", "GRANTS", "GROUP", "HAVING", "HEAP", "HIGH_PRIORITY", "HOUR", "HOUR_MINUTE", "HOUR_SECOND", "HOSTS", "IDENTIFIED", "IGNORE", "IN", "INDEX", "INFILE", "INNER", "INSERT", "INSERT_ID", "INT", "INTEGER", "INTERVAL", "INT1", "INT2", "INT3", "INT4", "INT8", "INTO", "IF", "IS", "ISAM", "JOIN", "KEY", "KEYS", "KILL", "LAST_INSERT_ID", "LEADING", "LEFT", "LENGTH", "LIKE", "LINES", "LIMIT", "LOAD", "LOCAL", "LOCK", "LOGS", "LONG", "LONGBLOB", "LONGTEXT", "LOW_PRIORITY", "MAX", "MAX_ROWS", "MATCH", "MEDIUMBLOB", "MEDIUMTEXT", "MEDIUMINT", "MIDDLEINT", "MIN_ROWS", "MINUTE", "MINUTE_SECOND", "MODIFY", "MONTH", "MONTHNAME", "MYISAM", "NATURAL", "NUMERIC", "NO", "NOT", "NULL", "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUTER", "OUTFILE", "PACK_KEYS", "PARTIAL", "PASSWORD", "PRECISION", "PRIMARY", "PROCEDURE", "PROCESS", "PROCESSLIST", "PRIVILEGES", "READ", "REAL", "REFERENCES", "RELOAD", "REGEXP", "RENAME", "REPLACE", "RESTRICT", "RETURNS", "REVOKE", "RLIKE", "ROW", "ROWS", "SECOND", "SELECT", "SET", "SHOW", "SHUTDOWN", "SMALLINT", "SONAME", "SQL_BIG_TABLES", "SQL_BIG_SELECTS", "SQL_LOW_PRIORITY_UPDATES", "SQL_LOG_OFF", "SQL_LOG_UPDATE", "SQL_SELECT_LIMIT", "SQL_SMALL_RESULT", "SQL_BIG_RESULT", "SQL_WARNINGS", "STRAIGHT_JOIN", "STARTING", "STATUS", "STRING", "TABLE", "TABLES", "TEMPORARY", "TERMINATED", "TEXT", "THEN", "TIME", "TIMESTAMP", "TINYBLOB", "TINYTEXT", "TINYINT", "TRAILING", "TO", "TYPE", "USE", "USING", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE", "VALUES", "VARCHAR", "VARIABLES", "VARYING", "VARBINARY", "WITH", "WRITE", "WHEN", "WHERE", "YEAR", "YEAR_MONTH", "ZEROFILL", NULL };

/* Information About The Plugin */
/* plugin name
 * plugin description
 * plugin author
 * plugin URL
 * plugin version
 * compile date
 */
dbi_info_t plugin_info = {
        "mysql",
        "Wrapper for libmysql-client for use with MySQL servers",
        "Mark M. Tobenkin <mark@brentwoodradio.com>",
        "http://libdbi.sourceforge.net/plugins/lookup.php?name=mysql",
        "mysql v0.0.1",
        __DATE__
};


/*****************************************************************************/
/* FUNCTION PROTOTYPES                                                       */
/*****************************************************************************/

 
/* Internal Functions *\
\**********************/

unsigned short _map_type(enum enum_field_types mytype);
unsigned short _map_type_attributes(enum enum_field_types mytype);

char *_strcpy_safe(char *str1, const char *str2);

dbi_row_t *_row_new(MYSQL_ROW *myrow);

/* DBI Plugin Standard Functions *\
\*********************************/

int dbd_connect( dbi_driver_t *driver );

int dbd_disconnect( dbi_driver_t *driver );

dbi_result_t *dbd_efficient_query( dbi_driver_t *driver, char *statement );
int dbd_errno( dbi_driver_t *driver );
char *dbd_errstr( dbi_driver_t *driver );

int dbd_fetch_field( dbi_result_t *result, const char *key, void **dest );
int dbd_fetch_field_raw( dbi_result_t *result, const char *key, void **dest);
int dbd_fetch_row( dbi_result_t *result );
int dbd_free_query( dbi_result_t *result);
	
const char **dbd_get_custom_functions_list();
const dbi_info_t *dbd_get_info();
const char **dbd_get_reserved_words_list();
int dbd_goto_row(dbi_result_t *result, unsigned int row);

int dbd_initialize( dbi_plugin_t *plugin );

dbi_result_t *dbd_list_dbs( dbi_driver_t *driver );
dbi_result_t *dbd_list_tables( dbi_driver_t *driver );
	
/* Not sure if these two do what they're supposed to */
unsigned int dbd_num_rows( dbi_result_t *result );
unsigned int dbd_num_rows_affected( dbi_result_t *result );

dbi_result_t *dbd_query( dbi_driver_t *driver, char *statement );

int dbd_select_db( dbi_driver_t *driver, char *database);



/*****************************************************************************/
/* _MAP_TYPE                                                                 */
/*****************************************************************************/
/*
 * Precondition: none.
 * Postcondition: none.
 * Returns: The DBI type associated with MySQL type mytype.
 */

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
			type = DBI_TYPE_INTEGER;
		break;
		
		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
		case FIELD_TYPE_LONGLONG: /* Unforturnately, C can only see longlong's as doubles*/
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
			attb |= DBI_INTEGER_SIZE4;
		break;

		case FIELD_TYPE_LONG:
		case FIELD_TYPE_INT24:
		case FIELD_TYPE_TIMESTAMP:
			attb |= DBI_INTEGER_SIZE8;
		break;

		case FIELD_TYPE_FLOAT:
			attb |= DBI_DECIMAL_SIZE4;
		break;
		
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
		case FIELD_TYPE_LONGLONG: /* Unforturnately, C can only see longlong's as doubles*/
			attb |= DBI_DECIMAL_SIZE8;
		break;
		
		/* Add Decimal Sizes Later */
		default:
	}

	return attb;
}

/*****************************************************************************/
/* _STRCPY_SAFE                                                              */
/*****************************************************************************/
/*
 * Precondtion: str2 != NULL
 * Postcondition: str1 is freed if not null
 * Returns: A newly allocated copy of str2
 */

char *_strcpy_safe(char *str1, const char *str2)
{
	char *final=NULL;

	if(str1) free(str1);

	final = (char*) malloc( sizeof(char) * (1 + strlen(str2)) );
	strcpy(final, str2);

	return final;
}


/*****************************************************************************/
/* DBD_CONNECT                                                               */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: set driver's connection to MYSQL connection,
 *	set error_message on failure
 * Returns: 0 on success, -1 on failure
 */

int dbd_connect( dbi_driver_t *driver )
{
	MYSQL *mycon; /* The Connection To The MySQL Server */

	/* These Are Variables Preloaded Into The Driver  */
	const char *host = dbi_get_option(driver, "host");
        const char *username = dbi_get_option(driver, "username");
        const char *password = dbi_get_option(driver, "password");
        const char *database = dbi_get_option(driver, "database");
        int port = dbi_get_option_numeric(driver, "port");

	/* Initialize Connection */
	mycon = mysql_init(NULL);

	if(mycon == NULL){ /* Failure, Memory Problems */
		/*driver->error_message = _strcpy_safe(driver->error_message, "Not Enough Memory");*/
		return -1;
	}

	/* Attempt To Make Connection, Give Error On Failure */
	if( mysql_real_connect(mycon, host, username, password, database, port, NULL, 0) ){
		driver->connection = (void *) mycon;
		
		driver->current_db = _strcpy_safe(driver->current_db, database);

		return 0;
	} else {
		/*driver->error_message = _strcpy_safe(driver->error_message, mysql_error(mycon));*/

		/*driver->error_number = mysql_errno(con);*/

		mysql_close(mycon);

		return -1;
	}
}

/*****************************************************************************/
/* _ROW_NEW                                                                  */
/*****************************************************************************/
/*
 * Precondition: myrow != NULL
 * Postcondition: none.
 * Returns: fully allocated dbi_row_t.
 */

dbi_row_t *_row_new( MYSQL_ROW *myrow, int nfields )
{
	dbi_row_t *row;
	
	row = (dbi_row_t*) malloc(sizeof(dbi_row_t));
	row->row_handle = (void*) myrow;
	row->next = NULL;
	row->numfields = nfields;

	row->field_names = (char **) malloc(sizeof(char*) * (row->numfields )); 
	row->field_types = (unsigned short*) malloc(sizeof(unsigned short) * (row->numfields ));
	row->field_type_attributes = (unsigned short*) malloc(sizeof(unsigned short) * (row->numfields ));
	row->field_values = (void**) malloc(sizeof(void*) * (row->numfields ));
	
	return row;
}

/*****************************************************************************/
/* DBD_DISCONNECT                                                            */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: driver's connection (if any) disconnected, or error message
 *	set
 * Returns: 0 on success, -1 on failure
 */

int dbd_disconnect( dbi_driver_t *driver )
{
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */

	if(mycon){
		mysql_close(mycon);
		driver->connection = NULL;
		
		return 0;
	} else {

		/*driver->error_message = _strcpy_safe(driver->error_message, error);*/
		
		return -1;
	}
}

/*****************************************************************************/
/* DBD_EFFICIENT_QUERY                                                       */
/*****************************************************************************/
/*
 * TODO
 */

dbi_result_t *dbd_efficient_query( dbi_driver_t *driver, char *statement )
{
	return NULL;
}

/*****************************************************************************/
/* DBD_ERRNO                                                                 */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: updates driver->error_number
 * Returns: driver->error_number
 */

int dbd_errno( dbi_driver_t *driver )
{
	MYSQL *mycon = (MYSQL*)driver->connection;
	
	if(!driver->error_number)
		driver->error_number = mysql_errno(mycon);

	return driver->error_number;
}

/*****************************************************************************/
/* DBD_ERRSTR                                                                */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: updates driver->error_message
 * Returns: driver->error_message
 */

char *dbd_errstr( dbi_driver_t *driver )
{
	MYSQL *mycon = (MYSQL*)driver->connection;
	
	if(!driver->error_message)
		driver->error_message = _strcpy_safe(driver->error_message, mysql_error(mycon));

	return driver->error_message;
}

/*****************************************************************************/
/* DBD_FETCH_FIELD                                                           */
/*****************************************************************************/
/*
 * Precondition: result != NULL, result->row != NULL
 * Postcondition: dest = fetched field
 * Returns: 0 on success, -1 on failure
 */

int dbd_fetch_field( dbi_result_t *result, const char *key, void **dest ){
	
	/*dbi_driver_t *driver = result->driver;  Our Driver */
	dbi_row_t *row = result->row; /* Our Row */
	
	int i;

	for(i = 0; i < row->numfields; i++){
		if(!strcmp(row->field_names[i], key))
			break;
	}

	if(i == row->numfields){
		/* driver->error_message = _strcpy_safe(driver->error_message, "Field Does Not Exist"); */

		return -1;
	}

	/* Find Field's Type, Allocate Memory And Copy  */
	if(row->field_types[i] == DBI_TYPE_INTEGER){
		
		if(row->field_type_attributes[i] & DBI_INTEGER_SIZE3)
			*( (char*) *dest ) = *( (char*) row->field_values[i]);

		else if(row->field_type_attributes[i] & DBI_INTEGER_SIZE4)
			*( (short*) *dest ) = *( (short*) row->field_values[i] );

		else if(row->field_type_attributes[i] & DBI_INTEGER_SIZE8)
			*( (long*) *dest) = *( (long*) row->field_values[i]);

	} else if(row->field_types[i] == DBI_TYPE_DECIMAL) {
		
		if(row->field_type_attributes[i] & DBI_DECIMAL_SIZE4)
			*( (float*) *dest) = *( (float*) row->field_values[i]);
		
		else if(row->field_type_attributes[i] & DBI_DECIMAL_SIZE8)
			*( (double*) *dest) = *( (double*) row->field_values[i]);

	} else if(row->field_types[i] == DBI_TYPE_STRING){
		*((char**)*dest) = _strcpy_safe(NULL, *((char**)row->field_values[i]));
	}

	return 0;
}

/*****************************************************************************/
/* DBD_FETCH_FIELD_RAW (DEPRECATED)                                          */
/*****************************************************************************/
/*
 * Precondition:
 * Postcondition:
 * Returns:
 */
int dbd_fetch_field_raw(dbi_result_t *result, const char *key, void **dest)
{
	*dest = NULL;

	return -1;
}

/*****************************************************************************/
/* DBD_FETCH_ROW                                                             */
/*****************************************************************************/
/*
 * Precondition: result != NULL, result->driver != NULL,
 *	result->driver->connection != NULL
 * Postcondition: sets result's row to next row, or NULL if empty
 * Returns: 1 on success, -1 on failure, 0 if no rows
 */

int dbd_fetch_row( dbi_result_t *result )
{
	dbi_driver_t *driver = result->driver; /* Our Driver */
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *myres = (MYSQL_RES*) result->result_handle; /* Our Result */

	dbi_row_t *row = NULL; /* Will Become result->row */
	MYSQL_ROW *myrow = NULL; /* Will Become row->row_handle */
	MYSQL_FIELD *myfield = NULL; /* For Iterations To Find row->field_* */

	/* Temporary Storage For Errors */
	int errno=0;

	/* Incrementor */
	int i;
	char *string=NULL;
	long *len=NULL;

	/* Grab Row */
	myrow = (MYSQL_ROW*) malloc(sizeof(MYSQL_ROW));
	*myrow = mysql_fetch_row(myres);

	len = mysql_fetch_lengths(myres);

	
	/* Either No More Rows, Or Error*/
	if(*myrow == NULL){

		if( errno = mysql_errno(mycon) ){ /* In Case Of Error */
			/* driver->error_message = _strcpy_safe(driver->error_message, mysql_error(con)); */
			/* driver->error_number = errno; */

			return -1;
		}

		return 0;
	}

	row = _row_new(myrow, mysql_num_fields(myres));

	mysql_field_seek(myres, 0);

	for(i = 0; i < row->numfields; i++){

		myfield = mysql_fetch_field(myres);

		row->field_names[i] = _strcpy_safe(NULL, myfield->name);
		row->field_types[i] = 0;
		row->field_type_attributes[i] = 0;
		
		string = (char*) malloc(sizeof(char) * (len[i] + 1));
		if(!string) return -1;

		string[len[i]] = '\0';
		if(len[i] != 0)
			memcpy((void*)string, *(myrow)[i], len[i]);

		fprintf(stderr, "Debugging Statement (sorry): String Value [%s]\n",
				string);

		row->field_types[i] = _map_type(myfield->type);
		row->field_type_attributes[i] = _map_type_attributes(myfield->type);

		if(row->field_types[i] == DBI_TYPE_INTEGER){
			if(row->field_type_attributes[i] & DBI_INTEGER_SIZE4){
				row->field_values[i] = malloc(sizeof(short));

				if(row->field_type_attributes[i] & DBI_INTEGER_UNSIGNED)
					*((unsigned short*)row->field_values[i]) = (unsigned short) atoi(string);
				else
					*((short*)row->field_values[i]) = (short) atoi(string);
			} else {
				row->field_values[i] = malloc(sizeof(long));
				*((long*)row->field_values[i]) = (long) atol(string);
			}
		}else if(row->field_types[i] == DBI_TYPE_DECIMAL){
			if(row->field_type_attributes[i] & DBI_DECIMAL_SIZE4){	
				row->field_values[i] = malloc(sizeof(float));
				*((float*)row->field_values[i]) = atof(string);
			}else{
				row->field_values[i] = malloc(sizeof(double));
				*((double*)row->field_values[i]) = atof(string);
			}
			
		} else if(row->field_types[i] == DBI_TYPE_STRING){
			if(*(index(string, (int)'\0')) != string[len[i]]){
				
				row->field_types[i] = DBI_TYPE_BINARY;
				row->field_values[i] = malloc(sizeof(char*));
				
				*((void**)row->field_values[i]) = malloc(sizeof(char) * len[i]);
				
				memcpy( *( (void**) row->field_values[i] ), (void*)string, len[i]);
				row->field_type_attributes[i] = len[i];
			} else {
				row->field_values[i] = malloc(sizeof(char*));
				*((char**)row->field_values[i]) = malloc(sizeof(char) *(len[i]+1));
				strcpy(*((char**)row->field_values[i]), string);
			}
		}
		free(string);

	}

	result->row = row;

	return 1;
}

/*****************************************************************************/
/* DBD_FREE_QUERY                                                            */
/*****************************************************************************/
/*
 * Precondition: result != NULL, result->result_handle != NULL
 * Postcondition: result free()'ed
 * Returns: 0 on success, -1 on failure
 */

int dbd_free_query( dbi_result_t *result)
{
	/* Free All Result Rows  */

	if(result->result_handle) free(result->result_handle);

	return 0;
}

/*****************************************************************************/
/* DBD_GET_CUSTOM_FUNCTIONS                                                  */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

const char **dbd_get_custom_functions_list()
{
	return custom_function_list;
}

/*****************************************************************************/
/* DBD_GET_INFO                                                              */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

const dbi_info_t *dbd_get_info()
{
	return &plugin_info;
}


/*****************************************************************************/
/* DBD_GET_RESERVED_WORDS                                                    */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

const char **dbd_get_reserved_words_list()
{
	return reserved_word_list;
}

/*****************************************************************************/
/* DBD_GOTO_ROW                                                              */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: next fetched row will be the rowth row
 * Returns:
 */

int dbd_goto_row(dbi_result_t *result, unsigned int row)
{
	MYSQL_RES *myres = (MYSQL_RES *)result->result_handle;
	
	mysql_row_seek(myres, (MYSQL_FIELD_OFFSET) row);

	return 0;
}


/*****************************************************************************/
/* DBD_INITIALIZE                                                            */
/*****************************************************************************/
/*
 * Precondition: plugin != NULL
 * Postcondition: plugin ready for usage
 * Returns: 0 on success, -1 on failure
 */

int dbd_initialize( dbi_plugin_t *plugin )
{
	/*
	plugin->info = &dbd_template_info;
	plugin->custom_function_list = DBD_CUSTOM_FUNCTIONS;
	*/
	return 0;
}

/*****************************************************************************/
/* DBD_LIST_DBS                                                              */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: none.
 * Returns: list of db names
 */

dbi_result_t *dbd_list_dbs( dbi_driver_t *driver )
{
	return dbd_query(driver, "show databases");
}

/*****************************************************************************/
/* DBD_LIST_TABLES                                                           */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: none.
 * Returns: result storing list of table names
 */

dbi_result_t *dbd_list_tables( dbi_driver_t *driver )
{
	return dbd_query(driver, "show tables");
}

/*****************************************************************************/
/* DBD_NUM_ROWS                                                              */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: none.
 * Returns: returns result->numrows_matched
 */

unsigned int dbd_num_rows( dbi_result_t *result )
{
	return result->numrows_matched;
}

/*****************************************************************************/
/* DBD_NUM_ROWS_AFFECTED                                                     */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: none.
 * Returns: result->numrows_affected
 */

unsigned int dbd_num_rows_affected( dbi_result_t *result )
{
	return result->numrows_affected;
}


/*****************************************************************************/
/* DBD_QUERY                                                                 */
/*****************************************************************************/
/*
 * Precondition: driver != NULL, statement != NULL
 * Postcondition: query's server and creates dbi_result_t
 * Returns: dbi_result_t on success, sets driver's error_message on failure
 *	and returns NULL
 */

dbi_result_t *dbd_query( dbi_driver_t *driver, char *statement )
{
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *myres; /* MySQL's internal result type */
	dbi_result_t *result; /* DBI's internal result type*/

	/* Query, On Failure Return NULL */
	if(mysql_query(mycon, statement)){
		/* driver->error_message = _strcpy_safe(driver->error_message, mysql_error(con)); */
		/* driver->error_number = mysql_errno(con); */
	
		return NULL;
	}

	myres = mysql_store_result(mycon); /* Grab Result*/

	result = (dbi_result_t*) malloc(sizeof(dbi_result_t));
	result->result_handle = (void*) myres;
	result->driver = driver;
	result->numrows_affected = mysql_affected_rows(mycon);
	result->numrows_matched = mysql_num_rows(myres);
	result->row = NULL;

	return result;
}



/*****************************************************************************/
/* DBD_SELECT_DB                                                             */
/*****************************************************************************/
/*
 * Precondition: driver != NULL, database != NULL
 * Postcondition: connection set to new database, driver's current_db set to new
 *	database, set's error string/number on failure.
 * Returns: 0 on success, -1 on failure
 */

int dbd_select_db( dbi_driver_t *driver, char *database)
{
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */

	if(mysql_select_db(mycon, database)){ /* In Case Of Error */
		/* driver->error_message = _strcpy_safe(driver->error_message, mysql_error(con)); */
		/* driver->error_number = mysql_errno(con); */

		return -1;
	}

	/* Update driver */
	driver->current_db = _strcpy_safe(driver->current_db, database);

	return 0;
}

