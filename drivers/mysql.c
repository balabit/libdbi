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
#include "dbi.h"
#include "mysqlclient.h"

//typedef struct mysql_connection_s {
//	MYSQL *connection;
//	int handler;
//	mysql_connection_t *next;
//} mysql_connection_t;

//mysql_connection_t *cons = NULL;


int dbd_connect(dbi_driver_t *driver) /* host and login info already stored in driver's info table */
{
	MYSQL *con = mysql_init(NULL);

	
}

int dbd_disconnect(dbi_driver_t *driver)
{

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

