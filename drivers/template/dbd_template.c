/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, Brentwood Linux Users and Evangelists (BLUE).
 * Copyright (C) David Parker and Mark Tobenkin.
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

int dbd_initialize(dbi_plugin_t *myself) {
	/* perform any database-specific server initialization.
	 * dlhandle, filename, next, functions, reserved_words, and custom_functions
	 * already filled in by the dbi core. return -1 on error, 0 on success.
	 * if -1 is returned, the plugin will not be added to the list of available plugins. */
	
	return 0;
}

int dbd_connect(dbi_driver_t *myself) {
	const char *host = dbi_get_option(myself, "host");
	const char *username = dbi_get_option(myself, "username");
	const char *password = dbi_get_option(myself, "password");
	int port = dbi_get_option_numeric(myself, "port");
	/* grab any other options the database needs and connect! */
	/* myself->generic_connection and myself->connection should
	 * also be set if applicable */
	return 0;
}

int dbd_disconnect(dbi_driver_t *myself) {
	/* do whatever's necessary... */
	return 0;
}

int dbd_fetch_field(dbi_result_t *result, const char *key, void **dest) {
	/* grab the value in the field, convert it to the
	 * appropriate C datatype, and stuff it into dest */
	static const char *woot = "Bob";
	int *p_int;
	if (strcasecmp(key, "id") == 0) {
		/* we're dealing with a number, don't allocate anything */
		p_int = *dest;
		*p_int = 123;
	}
	else if (strcasecmp(key, "name") == 0) {
		/* string... the returned pointer is or was allocated by us (either here or in fetchrow) */
		*dest = (void *) woot;
	}
	else {
		*dest = NULL;
	}

	return 0;
}

int dbd_fetch_field_raw(dbi_result_t *result, const char *key, void **dest) {
	/* grab the value in the field, convert it to the
	 * appropriate C datatype, and stuff it into dest */
	static const char *woot = "Bob";
	int *p_int;
	if (strcasecmp(key, "id")) {
		/* we're dealing with a number, don't allocate anything */
		p_int = *dest;
		*p_int = 123;
	}
	else if (strcasecmp(key, "name")) {
		/* string... the returned pointer is or was allocated by us (either here or in fetchrow) */
		*dest = (void *) woot;
	}
	else {
		*dest = NULL;
	}
	
	return 0;
}

int dbd_fetch_row(dbi_result_t *result) {
	/* do whatever's necessary... */
	static int beenhere = 0;
	beenhere++;
	if (beenhere > 5) {
		return 0; /* don't simulate rows forever... */
	}
	return 1; /* return -1 on error, 0 on no more rows, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
	/* do whatever's necessary... */
	result->driver = NULL;
	free(result);
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
	return result->numrows_matched;
}

unsigned int dbd_num_rows_affected(dbi_result_t *result) {
	/* return the numrows_matched or numrows_changed from the
	 * result struct, depending on what the database supports */
	return result->numrows_affected;
}

dbi_result_t *dbd_query(dbi_driver_t *myself, const char *statement) {
	/* do whatever's necessary... */
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	dbi_result_t *result;
	result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) {
		return NULL;
	}
	
	result->driver = myself;
	result->result_handle = NULL;
	result->numrows_matched = 5;
	result->numrows_affected = 5;
	result->row = NULL;
	
	return result;
}

dbi_result_t *dbd_efficient_query(dbi_driver_t *myself, const char *statement) {
	/* do whatever's necessary... */
	/* allocate a new dbi_result_t and fill its applicable members:
	 * driver, result_handle, numrows_matched, numrows_changed, row */
	dbi_result_t *result;
	result = (dbi_result_t *) malloc(sizeof(dbi_result_t));
	if (!result) {
		return NULL;
	}
	
	result->driver = myself;
	result->result_handle = NULL;
	result->numrows_matched = 5;
	result->numrows_affected = 5;
	result->row = NULL;
	
	return result;
}

int dbd_select_db(dbi_driver_t *myself, const char *db) {
	/* do whatever's necessary... */
	/* keep track of current db in myself->current_db */
	myself->current_db = strdup(db);
	return 0;
}

const char *dbd_errstr(dbi_driver_t *myself) {
	/* do whatever's neccesary */
	return NULL;
}

int dbd_errno(dbi_driver_t *myself) {
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

