/* Standard Libraries  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* For return/parameter Types  */
#include <dbi/dbi.h>

/* Database Specific Libraries  */
#include <mysql/mysql.h>


/* No Custom Functions */
#define DBD_CUSTOM_FUNCTIONS NULL

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
	free(row->field_values);
	free(row->field_names);
	free(row->field_types);
	free(row->field_type_attributes);

	free(row);
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
	plugin->info = &dbd_template_info;
	plugin->custom_function_list = DBD_CUSTOM_FUNCTIONS;

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
	MYSQL *con; /* The Connection To The MySQL Server */

	/* These Are Variables Preloaded Into The Driver  */
	char *host = dbi_get_option(myself, "host");
        char *username = dbi_get_option(myself, "username");
        char *password = dbi_get_option(myself, "password");
        char *database = dbi_get_option(myself, "database");
        int port = dbi_get_option_numeric(myself, "port");

	/* Initialize Connection */
	con = mysql_init(NULL);

	if(con == NULL){ /* Failure, Memory Problems */
		//driver->error_string = strcpy_safe(driver->error_string, "Not Enough Memory");
		return -1;
	}

	/* Attempt To Make Connection, Give Error On Failure */
	if( mysql_real_connect(con, host, username, password, database, port, NULL, 0) ){
		driver->connection = (void *) con;
		
		driver->currentdb = strcpy_safe(driver->currentdb, database);

		return 0;
	} else {
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));

		//driver->error_number = mysql_errno(con);

		mysql_close(con);

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
	MYSQL *con = (MYSQL*) driver->connection; /* Our Connection */

	if(con){
		mysql_close(con);
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

char **dbd_list_dbs( dbi_driver_t *driver )
{
	MYSQL *con = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *res = NULL;
	MYSQL_ROW *myrow = NULL;
	MYSQL_FIELD *field = NULL;

	res = mysql_list_dbs(con, "%");

	if(res == NULL){
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
		//driver->error_number = mysql_errno(con);
		return -1;
	}

	while(myrow = mysql_fetch_row(res)){
		
	}
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
	MYSQL *con = (MYSQL*) driver->connection; /* Our Connection */

	if(mysql_select_db(con, database)){ /* In Case Of Error */
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
	MYSQL *con = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *myres; /* MySQL's internal result type */
	dbi_result_t *dbires; /* DBI's internal result type*/

	/* Query, On Failure Return NULL */
	if(mysql_query(con, statement)){
		//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
		//driver->error_number = mysql_errno(con);
	
		return NULL;
	}

	myres = mysql_store_result(con); /* Grab Result*/

	dbires = (dbi_result_t*) malloc(sizeof(dbi_result_t));
	dbires->result_handle = (void*) myres;
	dbires->driver = driver;
	dbires->numrows_changed = mysql_affected_rows(con);
	dbires->numrows_matched = mysql_num_rows(myres);
	dbires->row = NULL;

	return dbires;
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
	MYSQL *con = (MYSQL*) driver->connection; /* Our Connection */
	MYSQL_RES *result = (MYSQL_RES*) result->result_handle; /* Our Result */

	dbi_row_t *dbirow = NULL; /* Will Become result->row */
	MYSQL_ROW *myrow = NULL; /* Will Become row->row_handle */

	/* Temporary Storage For Errors */
	int errno=0;

	/* Grab Row */
	myrow = (MYSQL_ROW*) malloc(sizeof(MYSQL_ROW));
	*myrow = mysql_fetch_row(result);

	/* Either No More Rows, Or Error*/
	if(*myrow == NULL){

		if( errno = mysql_errno(con) ){ /* In Case Of Error */
			//driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));
			//driver->error_number = errno;

			return -1;
		}

		return 0;
	}

	/* Create Row */
	dbirow = (dbi_row_t*) malloc(sizeof(dbi_row_t));
	dbirow->row_handle = (void*) myrow;
	dbirow->next = NULL;
	dbirow->numfields = mysql_num_fields(result);

	/*********************************************************************\
	* TODO: Interpret MYSQL_FIELDs to find dbirow->field_names,           *
	*         dbirow->field_types(very tedious), dbirow->field_values     *
	*         (just as tedious, if not more so :)                         *
	\*********************************************************************/
	
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
	dbi_row_t *dbirow = result->row;
	
	/* Free All Result Rows  */
	while(dbirow){
		result->row = result->row->next;
		free_row(dbirow);
		dbirow = result->row;
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
	MYSQL *con = (MYSQL*)driver->connection;
	
	driver->error_string = strcpy_safe(driver->error_string, mysql_error(con));

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
	MYSQL *con = (MYSQL*)driver->connection;
	
	driver->error_number = mysql_errno(con);

	return driver->error_number;
}
