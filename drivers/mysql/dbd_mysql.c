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
 * Precondtion: str2 != NULL, str1 passed by reference
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

	char *error; /* Temporary Storage For mysql_error()  */
	
	/* Initialize Connection */
	con = mysql_init(NULL);

	if(con == NULL){ /* Failure, Memory Problems */
		driver->error_string = strcpy_safe(driver->error_string, "Not Enough Memory");

		return -1;
	}

	/* Attempt To Make Connection, Give Error On Failure */
	if( mysql_real_connect(con, host, username, password, database, port, NULL, 0) ){
		driver->connection = (void *) con;
		
		driver->currentdb = strcpy_safe(driver->currentdb, database);

		return 0;
	} else {
		error = mysql_error(con);

		driver->error_string = strcpy_safe(driver->error_string, error);

		driver->error_number = mysql_errno(con);

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

		driver->error_string = strcpy_safe(driver->error_string, error);
		
		return -1;
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
	char *error; /* Temporary Storage For mysql_error() */

	if(mysql_select_db(con, database)){ /* In Case Of Error */
		error = mysql_error(con);

		driver->error_string = strcpy_safe(driver->error_string, error);

		driver->error_number = mysql_errno(con);

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

	char *error; /* Temporary Storage For mysql_error() */

	/* Query, On Failure Return NULL */
	if(mysql_query(con, statement)){

		error = mysql_error(con); 

		driver->error_string = strcpy_safe(driver->error_string, error);
	
		driver->error_number = mysql_errno(con);
	
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
	char *error=NULL;
	int errno=0;

	/* Grab Row */
	myrow = (MYSQL_ROW*) malloc(sizeof(MYSQL_ROW));
	*myrow = mysql_fetch_row(result);

	/* Either No More Rows, Or Error*/
	if(*myrow == NULL){

		if( errno = mysql_errno(con) ){ /* In Case Of Error */
			error = mysql_error(con);

			driver->error_string = strcpy_safe(driver->error_string, error);

			driver->error_number = errno;

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
	
	/*********************************************************************\
	* NOTE: Should we really have field_values? Why not use a combination *
	*         of row_handle and fetch_field()?                            *
	\*********************************************************************/
}


