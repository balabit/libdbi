/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, Brentwood Linux Users and Evangelists (BLUE), David Parker, Mark Tobenkin.
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
 * dbd_template.c: Template plugin for libdbi.
 * Copyright (C) 2001, Mark M. Tobenkin <mark@brentwoodradio.com>
 * http://libdbi.sourceforge.net/plugins/lookup.php?name=mysql
 * 
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
/* any database library includes */

#define DBD_CUSTOM_FUNCTIONS NULL

dbi_info_t dbd_template_info = {
	/* short name, used for loading drivers by name */
	"mysql",
	/* short desc, no more than two sentences, no newlines */
	"Wrapper for libmysql-client for use with MySQL servers",
	/* plugin author/maintainer, email address */
	"Mark M. Tobenkin <mark@brentwoodradio.com>",
	/* URL of plugin, if maintained by a third party */
	"http://libdbi.sourceforge.net/plugins/lookup.php?name=mysql",
	/* plugin version */
	"mysql v0.01",
	/* compilation date */
	__DATE__
};

int dbd_initialize(dbi_plugin_t *myself) {
	/* fill info structure and custom function list. name and filename will
	 * already be set by the main dbi initialization. functions and custom_functions
	 * will be automatically set by dbi initialization after this function executes.
	 * also perform any database-specific init. return -1 on error, 0 on success */
	
	myself->info = &dbd_template_info;
	myself->custom_functions_list = DBD_CUSTOM_FUNCTIONS;
	/* other init... */
	
	return 0;
}

int dbd_connect(dbi_driver_t *myself) {
	char *host = dbi_get_option(myself, "host");
	char *username = dbi_get_option(myself, "username");
	char *password = dbi_get_option(myself, "password");
	char *database = dbi_get_option(myself, "database");
	int port = dbi_get_option_numeric(myself, "port");
	/* grab any other options the database needs and connect! */
	/* myself->generic_connection and myself->connection should
	 * also be set if applicable */
	MYSQL *con;

	con = mysql_init(NULL);

	if(con == NULL){
		strcpy(driver->error_message, "Not Enough Memory To Allocate Handle");
		return -1;
	}

	if(mysql_real_connect(con, host, username, password, db, port, NULL, NULL)){
		driver->connection = (void*) con;
		strcpy(driver->currentdb, db);
	} else {
		strcpy(driver->error_message, mysql_error(con));
		driver->error_number = mysql_errno(con);
		mysql_close(con);
		return -1;
	}

	return 0;
}

int dbd_disconnect(dbi_driver_t *myself)
{
	if(myself && myself->connection){
		mysql_close( (MYSQL*) myself->connection);
		myself->connection = NULL;
		return 0;
	} else
		return -1;
}



int dbd_fetch_field(dbi_result_t *result, const char *key, void *dest) {
	/* grab the value in the field, convert it to the
	 * appropriate C datatype, and stuff it into dest */
	dbi_row_t *row;
	int i, f = -1;	

	row = result->rows;

	if(!row){
		dest = NULL;
		strcpy(result->driver->error_string, "No row fetched yet"); /* later add sprintf here*/
		return -1;
	}

	for(i = 0; i < row->numfields; i++){
		if(!strcmp(key, row->field_names[i]))
			break;
	}

	if(i == row->numfields){
		dest = NULL;
		strcpy(result->driver->error_string, "Field does not exist"); /* later add sprintf here*/
		return -1;
	}

	dest = row->field_values[i];

	return 0;
}

int dbd_fetch_row(dbi_result_t *result) {
	dbi_row_t *trav;
	MYSQL_ROW row; 
	
	if(!result) return -1;


	/* Grab a Row*/
	row = mysql_fetch_row((MYSQL_RES*)result->result_handle);

	/* If no row, check for errors, or just say it's empty*/
	if(row == NULL){
		if(mysql_errno((MYSQL*)result->driver->connection)){
			strcpy(result->driver->error_message, mysql_error((MYSQL*)result->driver->connection));
			result->driver->error_number = mysql_errno((MYSQL*)result->driver->connection);
			return -1;
		}
		return 0;
	}

	while(trav){ /* Free all preexisting rows */
		trav = result->row;
		result->row = result->row->next;
		free(trav);
	}

	result->row =  (dbi_row_t *) malloc(sizeof(dbi_row_t));
	result->row->row_handle = (void *) row;
	result->row->next = NULL;

	
	/*if(!trav){ /* If there are now rows already loaded, allocate a new one*/
	/*	result->row = trav = (dbi_row_t *) malloc(sizeof(dbi_row_t));
		trav->row_handle = (void *) row;
		trav->next = NULL;
	} else { /* otherwise, find the last row, and allocate it's next*/
	/*	while(trav->next){
			trav = trav->next;
		}
		trav->next = (dbi_row_t *) malloc(sizeof(dbi_row_t));
		trav = trav->next;
		trav->row_handle = (void *) row;
		trav->next = NULL;
	}*/

	/* Here we will turn all the fields into a fully qualified row */

	/*************************TODO***********************************/
	/*                                                              *
	 * As we don't know our mask/enum scheme yet for columns types, *
	 * The section turning fields into fully-qualified rows is      *
	 * incomplete.                                                  *
	 ****************************************************************/
		
	return 1; /* return -1 on error, 0 on no more rows, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	/* do whatever's necessary... */
	mysql_free_result(result);
	return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned int row) {
	/* do whatever's necessary... */
	return 0;
}

const char **dbd_list_dbs(dbi_driver_t *myself) {
	/* do whatever's necessary... */
	return 0;
}

const char **dbd_list_tables(dbi_driver_t *myself, const char *db) {
	/* do whatever's necessary... */
	return 0;
}

unsigned int dbd_num_rows(dbi_result_t *result) {
	/* return the numrows_matched or numrows_changed from the
	 * result struct, depending on what the database supports */
	if(result)
		return result->numrows_matched;
	else
		return 0;
}

unsigned int dbd_num_rows_affected(dbi_result_t *result) {
	/* return the numrows_matched or numrows_changed from the
	 * result struct, depending on what the database supports */
	if(result)
		return result->numrows_changed;
	else
		return 0;
}

dbi_result_t *dbd_query(dbi_driver_t *myself, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	MYSQL_RES *res;
	dbi_result_t *ret;

	if(!mysql_query( (MYSQL*)myself->connection, statement)){
		res = mysql_store_result((MYSQL*) myself->connection);
	} else {
		strcpy(myself->error_message, mysql_error((MYSQL*)myself->connection));
		myself->error_number = mysql_errno((MYSQL*)myself->connection);
		return NULL;
	}

	ret = malloc(sizeof(dbi_result_t));
	ret->result_handle = res;
	ret->driver = myself;
	ret->numrows_changed = mysql_affected_rows((MYSQL*)myself->connection);
	ret->numrows_matched = mysql_num_rows(res);

	return ret;
}

dbi_result_t *dbd_efficient_query(dbi_driver_t *myself, const char *statement) {
	/* do whatever's necessary... */
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	return 0;
}

int dbd_select_db(dbi_driver_t *myself, const char *db) {
	/* do whatever's necessary... */
	/* keep track of current db in myself->current_db */
	return 0;
}

const char *dbd_errstr(dbi_driver_t *myself) {
	if(myself){
		strcpy(myself->error_string, mysql_error((MYSQL*)myself->connection));
		return myself->error_string;
	}else
		return 0;
}

int dbd_errno(dbi_driver_t *myself) {
	if(myself){
		myself->error_number = mysql_error((MYSQL*)myself->connection);
		return myself->error_number;
	}else
		return 0;
}

/**************************************
 * CUSTOM DATABASE-SPECIFIC FUNCTIONS *
 **************************************/
