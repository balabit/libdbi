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
#define DBD_CUSTOM_FUNCTIONS { NULL }

/* Reserved Words */
#define DBD_RESERVED_WORDS { "ACTION", "ADD", "AGGREGATE", "ALL", "ALTER", "AFTER", "AND", "AS", "ASC", "AVG", "AVG_ROW_LENGTH", "AUTO_INCREMENT", "BETWEEN", "BIGINT", "BIT", "BINARY", "BLOB", "BOOL", "BOTH", "BY", "CASCADE", "CASE", "CHAR", "CHARACTER", "CHANGE", "CHECK", "CHECKSUM", "COLUMN", "COLUMNS", "COMMENT", "CONSTRAINT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "DATA", "DATABASE", "DATABASES", "DATE", "DATETIME", "DAY", "DAY_HOUR", "DAY_MINUTE", "DAY_SECOND", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "DEC", "DECIMAL", "DEFAULT", "DELAYED", "DELAY_KEY_WRITE", "DELETE", "DESC", "DESCRIBE", "DISTINCT", "DISTINCTROW", "DOUBLE", "DROP", "END", "ELSE", "ESCAPE", "ESCAPED", "ENCLOSED", "ENUM", "EXPLAIN", "EXISTS", "FIELDS", "FILE", "FIRST", "FLOAT", "FLOAT4", "FLOAT8", "FLUSH", "FOREIGN", "FROM", "FOR", "FULL", "FUNCTION", "GLOBAL", "GRANT", "GRANTS", "GROUP", "HAVING", "HEAP", "HIGH_PRIORITY", "HOUR", "HOUR_MINUTE", "HOUR_SECOND", "HOSTS", "IDENTIFIED", "IGNORE", "IN", "INDEX", "INFILE", "INNER", "INSERT", "INSERT_ID", "INT", "INTEGER", "INTERVAL", "INT1", "INT2", "INT3", "INT4", "INT8", "INTO", "IF", "IS", "ISAM", "JOIN", "KEY", "KEYS", "KILL", "LAST_INSERT_ID", "LEADING", "LEFT", "LENGTH", "LIKE", "LINES", "LIMIT", "LOAD", "LOCAL", "LOCK", "LOGS", "LONG", "LONGBLOB", "LONGTEXT", "LOW_PRIORITY", "MAX", "MAX_ROWS", "MATCH", "MEDIUMBLOB", "MEDIUMTEXT", "MEDIUMINT", "MIDDLEINT", "MIN_ROWS", "MINUTE", "MINUTE_SECOND", "MODIFY", "MONTH", "MONTHNAME", "MYISAM", "NATURAL", "NUMERIC", "NO", "NOT", "NULL", "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUTER", "OUTFILE", "PACK_KEYS", "PARTIAL", "PASSWORD", "PRECISION", "PRIMARY", "PROCEDURE", "PROCESS", "PROCESSLIST", "PRIVILEGES", "READ", "REAL", "REFERENCES", "RELOAD", "REGEXP", "RENAME", "REPLACE", "RESTRICT", "RETURNS", "REVOKE", "RLIKE", "ROW", "ROWS", "SECOND", "SELECT", "SET", "SHOW", "SHUTDOWN", "SMALLINT", "SONAME", "SQL_BIG_TABLES", "SQL_BIG_SELECTS", "SQL_LOW_PRIORITY_UPDATES", "SQL_LOG_OFF", "SQL_LOG_UPDATE", "SQL_SELECT_LIMIT", "SQL_SMALL_RESULT", "SQL_BIG_RESULT", "SQL_WARNINGS", "STRAIGHT_JOIN", "STARTING", "STATUS", "STRING", "TABLE", "TABLES", "TEMPORARY", "TERMINATED", "TEXT", "THEN", "TIME", "TIMESTAMP", "TINYBLOB", "TINYTEXT", "TINYINT", "TRAILING", "TO", "TYPE", "USE", "USING", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE", "VALUES", "VARCHAR", "VARIABLES", "VARYING", "VARBINARY", "WITH", "WRITE", "WHEN", "WHERE", "YEAR", "YEAR_MONTH", "ZEROFILL", NULL }

/* Information About The Plugin */
/* plugin name
 * plugin description
 * plugin author
 * plugin URL
 * plugin version
 * compile date
 */
dbi_info_t dbd_template_info = {
        "mysql",
        "Wrapper for libmysql-client for use with MySQL servers",
        "Mark M. Tobenkin <mark@brentwoodradio.com>",
        "http://libdbi.sourceforge.net/plugins/lookup.php?name=mysql",
        "mysql v0.0.1",
        __DATE__
};

/*****************************************************************************/
/* STRCPY_SAFE                                                               */
/*****************************************************************************/
/*
 * Precondtion: str2 != NULL
 * Postcondition: str1 == str2, any memory is allocated
 * Returns: new string
 */

char *strcpy_safe(char *str1, char *str2)
{
	char *final=NULL;

	if(str1) free(str1);

	final = (char*) malloc( sizeof(char) * (1 + strlen(str2)) );
	strcpy(final, str2);

	return final;
}

/*****************************************************************************/
/* FREE_ROW                                                                  */
/*****************************************************************************/
/*
 * Precondition: row != NULL
 * Postcondition: free's all memory for fields_*, and frees row
 * Returns: none.
 */

void free_row( dbi_row_t *row)
{
	/* Needs to loop for each row, and loop for each field_* */
	free(row->field_values);
	free(row->field_names);
	free(row->field_types);
	free(row->field_type_attributes);

	free(row);
}

/*****************************************************************************/
/* DBD_GET_INFO                                                              */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

dbi_info_t *dbd_get_info()
{
	return &dbd_mysql_info;
}

/*****************************************************************************/
/* DBD_GET_CUSTOM_FUNCTIONS                                                  */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

char **dbd_get_custom_functions()
{
	return DBD_CUSTOM_FUNCTIONS;
}

/*****************************************************************************/
/* DBD_GET_RESERVED_WORDS                                                    */
/*****************************************************************************/
/*
 * Precondition: 
 * Postcondition: 
 * Returns:
 */

char **dbd_get_reserved_words()
{
	return DBD_RESERVED_WORDS;
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
/* DBD_CONNECT                                                               */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: set driver's connection to MYSQL connection,
 *	set error_string on failure
 * Returns: 0 on success, -1 on failure
 */

int dbd_connect( dbi_driver_t *driver )
{
	MYSQL *mycon; /* The Connection To The MySQL Server */

	/* These Are Variables Preloaded Into The Driver  */
	char *host = dbi_get_option(myself, "host");
        char *username = dbi_get_option(myself, "username");
        char *password = dbi_get_option(myself, "password");
        char *database = dbi_get_option(myself, "database");
        int port = dbi_get_option_numeric(myself, "port");

	/* Initialize Connection */
	mycon = mysql_init(NULL);

	if(mycon == NULL){ /* Failure, Memory Problems */
		//driver->error_string = strcpy_safe(driver->error_string, "Not Enough Memory");
		return -1;
	}

	/* Attempt To Make Connection, Give Error On Failure */
	if( mysql_real_connect(mycon, host, username, password, database, port, NULL, 0) ){
		driver->connection = (void *) mycon;
		
		driver->currentdb = strcpy_safe(driver->currentdb, database);

		return 0;
	} else {
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));

		//driver->error_number = mysql_errno(con);

		mysql_close(mycon);

		return -1;
	}
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

		//driver->error_string = strcpy_safe(driver->error_string, error);
		
		return -1;
	}
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
 * Precondition:
 * Postcondition:
 * Returns:
 */

dbi_result_t *dbd_list_tables( dbi_driver-t *driver )
{
	return dbd_query(driver, "show tables");
}

/*****************************************************************************/
/* DBD_SELECT_DB                                                             */
/*****************************************************************************/
/*
 * Precondition: driver != NULL, database != NULL
 * Postcondition: connection set to new database, driver's currentdb set to new
 *	database, set's error string/number on failure.
 * Returns: 0 on success, -1 on failure
 */

int dbd_select_db( dbi_driver_t *driver, char *database)
{
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */

	if(mysql_select_db(mycon, database)){ /* In Case Of Error */
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
		//driver->error_number = mysql_errno(con);

		return -1;
	}

	/* Update driver */
	driver->currentdb = strcpy_safe(driver->currentdb, database);

	return 0;
}

/*****************************************************************************/
/* DBD_QUERY                                                                 */
/*****************************************************************************/
/*
 * Precondition: driver != NULL, statement != NULL
 * Postcondition: query's server and creates dbi_result_t
 * Returns: dbi_result_t on success, sets driver's error_string on failure
 *	and returns NULL
 */

dbi_result_t *dbd_query( dbi_driver_t *driver, char *statement )
{
	MYSQL *mycon = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *myres; /* MySQL's internal result type */
	dbi_result_t *result; /* DBI's internal result type*/

	/* Query, On Failure Return NULL */
	if(mysql_query(mycon, statement)){
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
		//driver->error_number = mysql_errno(con);
	
		return NULL;
	}

	myres = mysql_store_result(mycon); /* Grab Result*/

	result = (dbi_result_t*) malloc(sizeof(dbi_result_t));
	result->result_handle = (void*) myres;
	result->driver = driver;
	result->numrows_changed = mysql_affected_rows(mycon);
	result->numrows_matched = mysql_num_rows(myres);
	result->row = NULL;

	return result;
}

/*****************************************************************************/
/* DBD_EFFICIENT_QUERY                                                       */
/*****************************************************************************/
/*
 * TODO
 */

dbi_result_t *dbd_efficient_query( dbi_driver_t *driver, char *statement )
{

}

/*****************************************************************************/
/* DBD_GOTO_ROW                                                              */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: next fetched row will be the rowth row
 * Returns:
 */

int dbi_goto_row(dbi_result_t *result, unsigned int row)
{
	MYSQL_RES *myres = (MYSQL_RES *)result->result_handle;
	
	mysql_row_seek(myres, (MYSQL_FIELD_OFFSET) row);

	return 0;
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

	return NULL;
	
	/* Either No More Rows, Or Error*/
	if(*myrow == NULL){

		if( errno = mysql_errno(mycon) ){ /* In Case Of Error */
			//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
			//driver->error_number = errno;

			return -1;
		}

		return 0;
	}

	/* Create Row */
	row = (dbi_row_t*) malloc(sizeof(dbi_row_t));
	row->row_handle = (void*) myrow;
	row->next = NULL;
	row->numfields = mysql_num_fields(myres);

	/*********************************************************************\
	* TODO: Interpret MYSQL_FIELDs to find row->field_names,              *
	*         row->field_types(very tedious), row->field_values           *
	*         (just as tedious, if not more so :)                         *
	\*********************************************************************/

	/*********************************************************************\
	* Psuedo Code For Rest Of Function:                                   *
	* 	1) Allocate memory for field_names, field_types_*.            *
	*	2) Loop for numfields number of times, grabbing MYSQL_FIELDs  *
	*	   2.1) strcpy_safe column names to field names               *
	*          2.2) switch() statement to find corrent field_type and     *
	*	            field_type_attributes.                            *
	*          2.3) find correct field length                             *
	*               2.3.1) copy string to seperate buffer                 *
	*               2.3.2) allocate memory for field_value                *
	*	        2.3.3) make correct string conversion, atof, atoi etc.*
	*       3) Free all result's previous row's.                          *
	*       4) Set result's rows, return 0.                               *
	* NOTE: field_names and field_types_* should both be part of the      *
	*         dbi_result_t, not the dbi_row_t.                            *
	\*********************************************************************/

	row->field_names = (char **) malloc(sizeof(char*) * (row->numfields + 1)); 
	row->field_types = (int *) malloc(sizeof(int) * (row->numfields + 1));
	row->field_type_attributes = (int *) malloc(sizeof(int) * (row->numfields + 1));
	row->field_values = (void**) malloc(sizeof(void*) * (row->numfields + 1));
	
	for(i = 0; i < row->numfields; i++){
		myfield = mysql_fetch_field(myres);

		row->field_names[i] = strcpy_safe(NULL, myfield->name);
		row->field_types[i] = 0;
		row->field_type_attributes[i] = 0;
		
		switch(myfield->type){
			case FIELD_TYPE_YEAR:
			case FIELD_TYPE_NULL:
				row->field_type_attributes[i] |= DBI_INTEGER_UNSIGNED;
			case FIELD_TYPE_TINY:
			case FIELD_TYPE_SHORT:
				row->field_type_attributes[i] |= DBI_INTEGER_SIZE4;
				row->field_types[i] = DBI_TYPE_INTEGER;
			break;

			case FIELD_TYPE_LONG:
			case FIELD_TYPE_INT24:
			case FIELD_TYPE_TIMESTAMP:
				row->field_type_attributes[i] |= DBI_INTEGER_SIZE8;
				row->field_types[i] = DBI_TYPE_INTEGER;
			break;
			
			case FIELD_TYPE_FLOAT:
			case FIELD_TYPE_DOUBLE:
			case FIELD_TYPE_DECIMAL:
			case FIELD_TYPE_LONGLONG: /* Unforturnately, C can only see longlong's as doubles*/
				row->field_types[i] = DBI_TYPE_DECIMAL;
			break;
			
			case FIELD_TYPE_DATE:
			case FIELD_TYPE_TIME:
			case FIELD_TYPE_DATETIME:
			case FIELD_TYPE_STRING:
			case FIELD_TYPE_BLOB:
			case FIELD_TYPE_SET:
			case FIELD_TYPE_ENUM:
				row->field_types[i] = DBI_TYPE_STRING;
			break;
		}
		/* In all code, replace len with myfield->length */

		string = (char*) malloc(sizeof(char) * (len[i] + 1));
		if(!string) return -1;

		string[len[i]] = '\0';
		memcpy((void*)string, (void*)myrow,len[i][i]);
		myrow += len[i];

		if(row->field_types[i] == DBI_TYPE_INTEGER){
			if(row->field_type_attributes[i] & DBI_INTEGER_SIZE4){
				row->field_values[i] = malloc(sizeof(short));

				if(row->field_type_attributes[i] & DBI_INTEGER_UNSIGNED)
					*(row->field_values[i]) = (unsigned short) atoi(string);
				else
					*(row->field_values[i]) = (short) atoi(string);
			} else {
				row->field_values[i] = malloc(sizeof(long));
				*(row->field_values[i]) = (long) atol(string);
			}
		}else if(row->field_types[i] == DBI_TYPE_DECIMAL){
			
			row->field_values[i] = malloc(sizeof(double));
			*(row->field_values[i]) = (double) atof(string);
			
		} else if(row->field_types[i] == DBI_TYPE_STRING){
			
			if(index(string, (int)'\0') != string[len[i]]){/* beware of OBOEs */
				
				field_types[i] = DBI_TYPE_BINARY;
				field_values[i] = malloc(sizeof(char*))
				
				*(field_values[i]) = malloc(sizeof(char) * len[i]);
				
				memcpy((void*)*(field_values[i]), (void*)string, len[i]);
				field_type_attributes[i] = len[i];
			} else {
				field_values[i] = malloc(sizeof(char*));
				*(field_values[i]) = malloc(sizeof(char) *(len[i]+1));
				strcpy((char*)*(field_values[i]), string);
			}
		}
		free(string);

	}
	/* am i supposed to free the rows??*/

	row->field_names[i] = NULL;
	row->field_types[i] = NULL;
	row->field_type_attributes[i] = NULL;
	row->field_values[i] = NULL;

	free_row(result->row);

	result->row = row;

	return 0;
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
	
	dbi_driver_t *driver = result->driver; /* Our Driver */
	dbi_row_t *row = result->row; /* Our Row */
	
	int i;

	for(i = 0; i < row->numfields; i++){
		if(!strcmp(row->field_names[i], key))
			break;
	}

	if(i == row->numfields){
		//driver->error_string = strcpy_safe(driver->error_string, "Field Does Not Exist");

		return -1;
	}

	/* Find Field's Type, Allocate Memory And Copy  */
	if(field_types[i] == DBI_TYPE_INTEGER){
		
		if(field_type_attributes[i] & DBI_INTEGER_SIZE3)
			*dest = malloc(sizeof(char));
		
		else if(field_type_attributes[i] & DBI_INTEGER_SIZE4)
			*dest = malloc(sizeof(short));
		
		else(field_type_attributes[i] & DBI_INTEGER_SIZE8)
			*dest = malloc(sizeof(long));

		**dest = field_values[i];
	} else if(field_types[i] == DBI_TYPE_DECIMAL) {
		
		if(field_type_attributes[i] & DBI_DECIMAL_SIZE4)
			*dest = malloc(sizeof(float));
		
		else(field_type_attributes[i] & DBI_DECIMAL_SIZE8)
			*dest = malloc(sizeof(double));
		
		**dest = field_values[i];
	} else if(field_types[i] == DBI_TYPE_STRING){
		*dest = malloc(sizeof(char*));
		**dest = malloc (sizeof(char) * (1 + strlen((char*)field_values[i])) );
		**dest = strcpy_safe(NULL, (char*)field_values[i]);
	}

	if(*dest == NULL) return -1;

	return 0;
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
	dbi_row_t *row = result->row;
	
	/* Free All Result Rows  */
	while(row){
		result->row = result->row->next;
		free_row(row);
		row = result->row;
	}

	if(result->result_handle) free(result->result_handle);

	free(result);

	return 0;
}

/*****************************************************************************/
/* DBD_NUM_ROWS                                                              */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: none.
 * Returns: returns result->num_rows_matched
 */

unsigned int dbd_num_rows( dbi_result_t *result )
{
	return result->num_rows_matched;
}

/*****************************************************************************/
/* DBD_NUM_ROWS_AFFECTED                                                     */
/*****************************************************************************/
/*
 * Precondition: result != NULL
 * Postcondition: none.
 * Returns: result->num_rows_changed
 */

unsigned int dbd_num_rows_affected( dbi_result_t *result )
{
	return result->num_rows_changed;
}

/*****************************************************************************/
/* DBD_ERRSTR                                                                */
/*****************************************************************************/
/*
 * Precondition: driver != NULL
 * Postcondition: updates driver->error_string
 * Returns: driver->error_string
 */

char *dbd_errstr( dbi_driver_t *driver )
{
	MYSQL *mycon = (MYSQL*)driver->connection;
	
	driver->error_string = strcpy_safe(driver->error_string, mysql_error(mycon));

	return driver->error_string;
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
	
	driver->error_number = mysql_errno(mycon);

	return driver->error_number;
}
