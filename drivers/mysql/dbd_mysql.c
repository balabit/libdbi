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
 * Copyright (C) 2001, Herbert Z. Bartholemew <hbz@bombdiggity.net>.
 * http://www.bombdiggity.net/~hzb/dbd_template/
 * 
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
/* any database library includes */

#define DBD_CUSTOM_FUNCTIONS { "template_frob", "template_bork", NULL } /* actual functions must be called dbd_template_frob and dbd_template_bork, but this is how they are referred to when using dbi_custom_function() */

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
		return -1;
	}

	return 0;
}



int dbd_fetch_field(dbi_result_t *result, const char *key, void *dest) {
	/* grab the value in the field, convert it to the
	 * appropriate C datatype, and stuff it into dest */
	return 0;
}

int dbd_fetch_row(dbi_result_t *result) {
	/* do whatever's necessary... */
	return 1; /* return -1 on error, 0 on no more rows, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	/* do whatever's necessary... */
	return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned int row) {
	/* do whatever's necessary... */
	return 0;
}

const char **dbd_list_dbs(dbi_driver_t *myself) {
	/* do whatever's necessary... */
	return NULL;
}

const char **dbd_list_tables(dbi_driver_t *myself, const char *db) {
	/* do whatever's necessary... */
	return NULL;
}

unsigned int dbd_num_rows(dbi_result_t *result) {
	/* return the numrows_matched or numrows_changed from the
	 * result struct, depending on what the database supports */
	return 0;
}

unsigned int dbd_num_rows_affected(dbi_result_t *result) {
	/* return the numrows_matched or numrows_changed from the
	 * result struct, depending on what the database supports */
	return 0;
}

dbi_result_t *dbd_query(dbi_driver_t *myself, const char *statement) {
	/* do whatever's necessary... */
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	return NULL;
}

dbi_result_t *dbd_efficient_query(dbi_driver_t *myself, const char *statement) {
	/* do whatever's necessary... */
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	return NULL;
}

int dbd_select_db(dbi_driver_t *myself, const char *db) {
	/* do whatever's necessary... */
	/* keep track of current db in myself->current_db */
	return 0;
}

const char *dbd_errstr(dbi_driver_t *myself) {
	if(myself && myself->connection)
		return mysql_error((MYSQL*)myself->connection);
	else
		return NULL;
}

int dbd_errno(dbi_driver_t *myself) {
	if(myself && myself->connection)
		return mysql_errno((MYSQL*)myself->connection);
	else
		return NULL;
}

/**************************************
 * CUSTOM DATABASE-SPECIFIC FUNCTIONS *
 **************************************/
