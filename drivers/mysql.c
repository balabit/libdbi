/*
 * boilerplate LGPL licence, authors, and mini-description go here. what are we assigning the copyright as?
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbi.h"
#include "mysqlclient.h"

/*
typedef struct mysql_connection_s { 
 	MYSQL *connection; 
 	int handler; 
 	struct mysql_connection_s *next; 
} mysql_connection_t; 

 mysql_connection_t *cons = NULL; */


int dbd_connect(dbi_driver_t *driver) /* host and login info already stored in driver's info table */
{
	MYSQL *con=NULL; /* The mysql connection handler */

	char *host=NULL, *user=NULL, *password=NULL, *db=NULL; 
	int port=0, flags = 0; /* This and the above variables will be used for the connection */

	dbi_option_t *trav = driver->options; /* The option list */

	*con = mysql_init(NULL);

	if(con == NULL){
		strcpy(driver->error_message, "Not Enough Memory To Allocate Handle");
		driver->status = -1;
		return -1;
	}

	/* This loops traverses the options and tries to set as many as possible for the connection */
	while(trav){ 	
		if(! strcmp(trav->key, "host") ){
			strcpy(host, trav->string_value);
			
		} else if(! strcmp(trav->key, "username") ){
			strcpy(user, trav->string_value);
			
		} else if(! strcmp(trav->key, "password") ){
			strcpy(password, trav->string_value);

		} else if(! strcmp(trav->key, "database") ){
			strcpy(db, trav->string_value);
		
		} else if(! strcmp(trav->key, "port") ){
			port = trav->numeric_value;
			
		} else if(! strcmp(trav->key, "use_compression") ){
			if(trav->numeric_value) flags |= CLIENT_COMPRESS;
			else{ flags |= CLIENT_COMPRESS; flags ^= CLIENT_COMPRESS; }
		}

		trav = trav->next;
	}

	if(mysql_real_connect(con, host, user, password, db, port, NULL, flags)){
		driver->connection = (void*) con;
		strcpy(driver->currentdb, db);
		driver->status = 0;
	} else {
		strcpy(driver->error_message, mysql_error(con));
		driver->error_number = mysql_errno(con);
		driver->status = -1;
	}

	return driver->status;
}

int dbd_disconnect(dbi_driver_t *driver)
{
	if(driver->connecntion){
		mysql_close((MYSQL*)driver->connection);
	}
}

int dbd_fetch_field(dbi_result_t *result, const char *key, void *to)
{

}

int dbd_fetch_field_raw(dbi_result_t *result, const char *key, void *to)
{

}

int dbd_fetch_row(dbi_result_t *result)
{

}

int dbd_free_query(dbi_result_t *result)
{

}

int dbd_goto_row(dbi_result_t *result, unsigned int row);
const char **dbd_list_dbs(dbi_driver_t *driver);
const char **dbd_list_tables(dbi_driver_t *driver, const char *db);
unsigned int dbd_num_rows(dbi_result_t *result); /* number of rows in result set */
unsigned int dbd_num_rows_affected(dbi_result_t *result); /* only the rows in the result set that were actually modified */
dbi_result_t *dbd_query(dbi_driver_t *driver, const char *formatstr, ...); /* dynamic num of arguments, a la printf */
dbi_result_t *dbd_efficient_query(dbi_driver_t *driver, const char *formatstr, ...); /* better name instead of efficient_query?
this will only request one row at a time, but has the downside that other queries can't be made until this one is closed. at least that's how it works in mysql, so it has to be the common denominator */
int dbd_select_db(dbi_driver_t *driver, const char *db);

int *dbd_error(dbi_driver_t *driver, char *errmsg_dest); /* returns formatted message with the error number and string */
void dbd_error_handler(dbi_driver_t *driver, void *function, void *user_argument); /* registers a callback that's activated when the database encounters an error */

#ifdef __cplusplus
}
#endif /* __cplusplus */

