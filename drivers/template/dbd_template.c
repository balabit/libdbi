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
 * dbd_template.c: Example template plugin for libdbi.
 * Copyright (C) 2001, Herbert Z. Bartholemew <hbz@bombdiggity.net>.
 * http://www.bombdiggity.net/~hzb/dbd_template/
 * 
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbi/dbi.h>
#include "../../src/dbi_main.h"
/* any database library includes */


/*******************************
 * plugin-specific information *
 *******************************/

dbi_info_t plugin_info = {
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
 * this is how they are referred as when using dbi_custom_function() */

static const char *reserved_words_list[] = { "SELECT", "INSERT", "UPDATE", "UNSIGNED", "NULL", "NOT", "IS", "TINYINT", "VARCHAR", "etc...", NULL };



/********************************
 * database-dependent functions *
 ********************************/

static int fetched_rows = 0; /* this is a hack to simulate fetched data, erase this line in your own plugin code */

int dbd_initialize(dbi_plugin_t *plugin) {
	/* perform any database-specific server initialization.
	 * dlhandle, filename, next, functions, reserved_words, and custom_functions
	 * already filled in by the dbi core. return -1 on error, 0 on success.
	 * if -1 is returned, the plugin will not be added to the list of available plugins. */
	
	return 0;
}

int dbd_connect(dbi_driver_t *driver) {
	const char *host = dbi_driver_get_option(driver, "host");
	const char *username = dbi_driver_get_option(driver, "username");
	const char *password = dbi_driver_get_option(driver, "password");
	int port = dbi_driver_get_option_numeric(driver, "port");

	/* driver->connection = connect() */

	return 0;
}

int dbd_disconnect(dbi_driver_t *driver) {
	/* do whatever's necessary... */
	return 0;
}


int dbd_fetch_row(dbi_result_t *result) {
	/* this is just dummy code to simulate data */

	static unsigned int numfields = 3;
	static char *field_names[3] = { "id", "name", "weight" };
	static unsigned short field_types[3] = { DBI_TYPE_INTEGER, DBI_TYPE_STRING, DBI_TYPE_DECIMAL };
	static unsigned int field_attribs[3] = { DBI_INTEGER_SIZE4, 0, DBI_DECIMAL_SIZE8 };
	static dbi_data_t field_data[4][3];
	static unsigned int field_sizes[4][3] = {
		{ 0, 9, 0 },
		{ 0, 15, 0 },
		{ 0, 11, 0 },
		{ 0, 10, 0 },
	};
	static dbi_row_t rows[4];
	
	field_data[0][0].d_long = 1;
	field_data[0][1].d_string = "Rob Malda";
	field_data[0][2].d_double = 123.456789;
	
	field_data[1][0].d_long = 2;
	field_data[1][1].d_string = "Tux Penguin";
	field_data[1][2].d_double = 1.33333333;

	field_data[2][0].d_long = 3;
	field_data[2][1].d_string = "Santa Claus";
	field_data[2][2].d_double = 999999.1;

	field_data[3][0].d_long = 4;
	field_data[3][1].d_string = "Bill Gates";
	field_data[3][2].d_double = -666.666;

	rows[0].field_values = field_data[0];
	rows[0].field_sizes = field_sizes[0];
	rows[1].field_values = field_data[1];
	rows[1].field_sizes = field_sizes[1];
	rows[2].field_values = field_data[2];
	rows[2].field_sizes = field_sizes[2];
	rows[3].field_values = field_data[3];
	rows[3].field_sizes = field_sizes[3];
	
	if (fetched_rows >= 4) {
		return 0; /* don't simulate rows forever... */
	}

	if (fetched_rows == 0) {
		result->numfields = numfields;
		/* usually the next 3 members will have to be allocated dynamically */
		result->field_names = field_names;
		result->field_types = field_types;
		result->field_attribs = field_attribs;
	}
	
	result->currowidx++;
	result->rows[result->currowidx] = &rows[result->currowidx-1];
	fetched_rows++;
	
	return 1; /* return -1 on error, 0 on no more rows, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	/* do whatever's necessary... */
	fetched_rows = 0;
	return 0;
}

const char **dbd_get_custom_functions_list() {
	return custom_functions_list;
}

const dbi_info_t *dbd_get_info() {
	return &plugin_info;
}

const char **dbd_get_reserved_words_list() {
	return reserved_words_list;
}

int dbd_goto_row(dbi_driver_t *driver, unsigned int row) {
	return 0;
}

dbi_result_t *dbd_list_dbs(dbi_driver_t *driver) {
	/* return dbd_query(driver, "SELECT DB_Name AS dbname FROM system.dblist"); */
	return NULL;
}

dbi_result_t *dbd_list_tables(dbi_driver_t *driver, const char *db) {
	/* return dbi_driver_query((dbi_driver)driver, "SELECT Table_Name AS tablename FROM system.tablelist WHERE DB = %s", db); */
	return NULL;
}

dbi_result_t *dbd_query(dbi_driver_t *driver, const char *statement) {
	/* allocate a new dbi_result_t and fill its applicable members:
	 * 
	 * result_handle, numrows_matched, and numrows_changed.
	 * everything else will be filled in by DBI */
	
	int efficient = dbi_driver_get_option_numeric(driver, "efficient-queries");
	dbi_result_t *result;
	result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) {
		return NULL;
	}
	
	result->result_handle = NULL;
	result->numrows_matched = 4;
	result->numrows_affected = 0;
	
	return result;
}

int dbd_select_db(dbi_driver_t *driver, const char *db) {
	/* keep track of current db in myself->current_db */
	if (driver->current_db) free(driver->current_db);
	driver->current_db = strdup(db);
	return 0;
}

const char *dbd_errstr(dbi_driver_t *driver) {
	/* do whatever's neccesary */
	return NULL;
}

int dbd_errno(dbi_driver_t *driver) {
	/* do whatever's necessary */
	return 0;
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

