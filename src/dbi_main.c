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
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE /* since we need the asprintf() prototype */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <math.h>
#include <limits.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

#ifndef DBI_DRIVER_DIR
#define DBI_DRIVER_DIR "/usr/local/lib/dbd" /* use this as the default */
#endif

/* declarations for internal functions -- anything declared as static won't be accessible by name from client programs */
static dbi_driver_t *_get_driver(const char *filename);
static void _free_custom_functions(dbi_driver_t *driver);
static dbi_option_t *_find_or_create_option_node(dbi_conn Conn, const char *key);
static int _update_internal_conn_list(dbi_conn_t *conn, int operation);

dbi_result dbi_conn_query(dbi_conn Conn, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));

static const char *ERROR = "ERROR";
static dbi_driver_t *rootdriver;
static dbi_conn_t *rootconn;

/* XXX DBI CORE FUNCTIONS XXX */

int dbi_initialize(const char *driverdir) {
	DIR *dir;
	struct dirent *driver_dirent = NULL;
	struct stat statbuf;
	char fullpath[FILENAME_MAX];
	char *effective_driverdir;
	
	int num_loaded = 0;
	dbi_driver_t *driver = NULL;
	dbi_driver_t *prevdriver = NULL;
	
	rootdriver = NULL;
	effective_driverdir = (driverdir ? (char *)driverdir : DBI_DRIVER_DIR);
	dir = opendir(effective_driverdir);

	if (dir == NULL) {
		return -1;
	}
	else {
		while ((driver_dirent = readdir(dir)) != NULL) {
			driver = NULL;
			snprintf(fullpath, FILENAME_MAX, "%s/%s", effective_driverdir, driver_dirent->d_name);
			if ((stat(fullpath, &statbuf) == 0) && S_ISREG(statbuf.st_mode) && (!strcmp(strrchr(driver_dirent->d_name, '.'), DRIVER_EXT))) {
				/* file is a stat'able regular file that ends in .so (or appropriate dynamic library extension) */
				driver = _get_driver(fullpath);
				if (driver && (driver->functions->initialize(driver) != -1)) {
					if (!rootdriver) {
						rootdriver = driver;
					}
					if (prevdriver) {
						prevdriver->next = driver;
					}
					prevdriver = driver;
					num_loaded++;
				}
				else {
					if (driver && driver->dlhandle) dlclose(driver->dlhandle);
					if (driver && driver->functions) free(driver->functions);
					if (driver) free(driver);
					driver = NULL; /* don't include in linked list */
				}
			}
		}
		closedir(dir);
	}
	
	return num_loaded;
}

void dbi_shutdown() {
	dbi_conn_t *curconn = rootconn;
	dbi_conn_t *nextconn;
	
	dbi_driver_t *curdriver = rootdriver;
	dbi_driver_t *nextdriver;
	
	while (curconn) {
		nextconn = curconn->next;
		dbi_conn_close((dbi_conn)curconn);
		curconn = nextconn;
	}
	
	while (curdriver) {
		nextdriver = curdriver->next;
		dlclose(curdriver->dlhandle);
		free(curdriver->functions);
		_free_custom_functions(curdriver);
		free(curdriver->filename);
		free(curdriver);
		curdriver = nextdriver;
	}

	rootdriver = NULL;
}

const char *dbi_version() {
	return "libdbi v" VERSION;
}

/* XXX DRIVER FUNCTIONS XXX */

dbi_driver dbi_driver_list(dbi_driver Current) {
	dbi_driver_t *current = Current;

	if (current == NULL) {
		return (dbi_driver)rootdriver;
	}

	return (dbi_driver)current->next;
}

dbi_driver dbi_driver_open(const char *name) {
	dbi_driver_t *driver = rootdriver;

	while (driver && strcasecmp(name, driver->info->name)) {
		driver = driver->next;
	}

	return driver;
}

int dbi_driver_is_reserved_word(dbi_driver Driver, const char *word) {
	unsigned int idx = 0;
	dbi_driver_t *driver = Driver;
	
	if (!driver) return 0;
	
	while (driver->reserved_words[idx]) {
		if (strcasecmp(word, driver->reserved_words[idx]) == 0) {
			return 1;
		}
	}
	return 0;
}

void *dbi_driver_specific_function(dbi_driver Driver, const char *name) {
	dbi_driver_t *driver = Driver;
	dbi_custom_function_t *custom;

	if (!driver) return NULL;

	custom = driver->custom_functions;
	
	while (custom && strcasecmp(name, custom->name)) {
		custom = custom->next;
	}

	return custom ? custom->function_pointer : NULL;
}

/* DRIVER: informational functions */

const char *dbi_driver_get_name(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;
	
	return driver->info->name;
}

const char *dbi_driver_get_filename(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;
	
	return driver->filename;
}

const char *dbi_driver_get_description(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;
	
	return driver->info->description;
}

const char *dbi_driver_get_maintainer(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;

	return driver->info->maintainer;
}

const char *dbi_driver_get_url(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;

	if (!driver) return ERROR;

	return driver->info->url;
}

const char *dbi_driver_get_version(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;
	
	return driver->info->version;
}

const char *dbi_driver_get_date_compiled(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return ERROR;
	
	return driver->info->date_compiled;
}

int dbi_driver_quote_string(dbi_driver Driver, char **orig) {
	dbi_driver_t *driver = Driver;
	char *temp;
	char *newstr;
	int newlen;
	
	if (!driver || !orig || !*orig) return -1;

	newstr = malloc((strlen(*orig)*2)+4+1); /* worst case, we have to escape every character and add 2*2 surrounding quotes */
	
	newlen = driver->functions->quote_string(driver, *orig, newstr);
	if (newlen < 0) {
		free(newstr);
		return -1;
	}
	
	temp = *orig;
	*orig = newstr;
	free(temp); /* original unescaped string */

	return newlen;
}

/* XXX DRIVER FUNCTIONS XXX */

dbi_conn dbi_conn_new(const char *name) {
	dbi_driver driver;
	dbi_conn conn;

	driver = dbi_driver_open(name);
	conn = dbi_conn_open(driver);

	return conn;
}

dbi_conn dbi_conn_open(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	dbi_conn_t *conn;
	
	if (!driver) {
		return NULL;
	}

	conn = (dbi_conn_t *) malloc(sizeof(dbi_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->driver = driver;
	conn->options = NULL;
	conn->connection = NULL;
	conn->current_db = NULL;
	conn->error_number = 0;
	conn->error_message = NULL;
	conn->error_handler = NULL;
	conn->error_handler_argument = NULL;
	_update_internal_conn_list(conn, 1);

	return (dbi_conn)conn;
}

void dbi_conn_close(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	
	if (!conn) return;
	
	_update_internal_conn_list(conn, -1);
	
	conn->driver->functions->disconnect(conn);
	conn->driver = NULL;
	dbi_conn_clear_options(Conn);
	conn->connection = NULL;
	
	if (conn->current_db) free(conn->current_db);
	if (conn->error_message) free(conn->error_message);
	conn->error_number = 0;
	
	conn->error_handler = NULL;
	conn->error_handler_argument = NULL;
	free(conn);
}

dbi_driver dbi_conn_get_driver(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	
	if (!conn) return NULL;
	
	return conn->driver;
}

int dbi_conn_error(dbi_conn Conn, const char **errmsg_dest) {
	dbi_conn_t *conn = Conn;
	char number_portion[20];
	char *errmsg;
	
	if (conn->error_number) {
		snprintf(number_portion, 20, "%d: ", conn->error_number);
	}
	else {
		number_portion[0] = '\0';
	}

	asprintf(&errmsg, "%s%s", number_portion, conn->error_message);
	*errmsg_dest = errmsg;

	return conn->error_number;
}

void dbi_conn_error_handler(dbi_conn Conn, void *function, void *user_argument) {
	dbi_conn_t *conn = Conn;
	conn->error_handler = function;
	if (function == NULL) {
		conn->error_handler_argument = NULL;
	}
	else {
		conn->error_handler_argument = user_argument;
	}
}

/* DRIVER: option manipulation */

int dbi_conn_set_option(dbi_conn Conn, const char *key, char *value) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (!conn) {
		return -1;
	}
	
	option = _find_or_create_option_node(conn, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = strdup(value);
	option->numeric_value = 0;
	
	return 0;
}

int dbi_conn_set_option_numeric(dbi_conn Conn, const char *key, int value) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (!conn) {
		return -1;
	}
	
	option = _find_or_create_option_node(conn, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = NULL;
	option->numeric_value = value;
	
	return 0;
}

const char *dbi_conn_get_option(dbi_conn Conn, const char *key) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;

	if (!conn) {
		return NULL;
	}
	
	option = conn->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	return option ? option->string_value : NULL;
}

int dbi_conn_get_option_numeric(dbi_conn Conn, const char *key) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;

	if (!conn) return -1;
	
	option = conn->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	return option ? option->numeric_value : -1;
}

const char *dbi_conn_get_option_list(dbi_conn Conn, const char *current) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (conn && conn->options) option = conn->options;
	else return NULL;
	
	if (!current) {
		return option->key;
	}
	else {
		while (option && strcasecmp(current, option->key)) {
			option = option->next;
		}
		return (option && option->next) ? option->next->key : NULL;
	}
}

void dbi_conn_clear_option(dbi_conn Conn, const char *key) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *prevoption;
	dbi_option_t *option;
	
	if (!conn) return;
	option = conn->options;
	
	while (option && strcasecmp(key, option->key)) {
		prevoption = option;
		option = option->next;
	}
	if (!option) return;
	if (option == conn->options) {
		conn->options = option->next;
	}
	else {
		prevoption->next = option->next;
	}
	free(option->key);
	free(option->string_value);
	free(option);
	return;
}

void dbi_conn_clear_options(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *cur;
	dbi_option_t *next;

	if (!conn) return;
	cur = conn->options;
	
	while (cur) {
		next = cur->next;
		free(cur->key);
		free(cur->string_value);
		free(cur);
		cur = next;
	}

	conn->options = NULL;
}

/* DRIVER: SQL layer functions */

int dbi_conn_connect(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	int retval;
	
	if (!conn) return -1;
	
	retval = conn->driver->functions->connect(conn);
	//if (retval == -1) {			XXX cant call error handler when connection is already failed and terminated
	//	_error_handler(conn);
	//}
	return retval;
}

int dbi_conn_get_socket(dbi_conn Conn){
	dbi_conn_t *conn = Conn;
	int retval;

	if(!conn) return -1;

	retval = conn->driver->functions->get_socket(conn);

	return retval;
}

dbi_result dbi_conn_get_db_list(dbi_conn Conn, const char *pattern) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;
	
	if (!conn) return NULL;
	
	result = conn->driver->functions->list_dbs(conn, pattern);
	
	if (result == NULL) {
		_error_handler(conn);
	}

	return (dbi_result)result;
}

dbi_result dbi_conn_get_table_list(dbi_conn Conn, const char *db) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;
	
	if (!conn) return NULL;
	
	result = conn->driver->functions->list_tables(conn, db);
	
	if (result == NULL) {
		_error_handler(conn);
	}
	
	return (dbi_result)result;
}

dbi_result dbi_conn_query(dbi_conn Conn, const char *formatstr, ...) {
	dbi_conn_t *conn = Conn;
	char *statement;
	dbi_result_t *result;
	va_list ap;

	if (!conn) return NULL;
	
	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	result = conn->driver->functions->query(conn, statement);

	if (result == NULL) {
		_error_handler(conn);
	}
	free(statement);
	
	return (dbi_result)result;
}

dbi_result dbi_conn_query_null(dbi_conn Conn, const unsigned char *statement, unsigned long st_length) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;

	if (!conn) return NULL;

	result = conn->driver->functions->query_null(conn, statement, st_length);

	if (result == NULL) {
		_error_handler(conn);
	}
	
	return (dbi_result)result;
}

int dbi_conn_select_db(dbi_conn Conn, const char *db) {
	dbi_conn_t *conn = Conn;
	char *retval;
	
	if (!conn) return -1;
	
	free(conn->current_db);
	conn->current_db = NULL;
	
	retval = conn->driver->functions->select_db(conn, db);
	
	if (retval == NULL) {
		_error_handler(conn);
	}
	
	if (retval[0] == '\0') {
		/* if "" was returned, conn doesn't support switching databases */
		return -1;
	}
	else {
		conn->current_db = strdup(retval);
	}
	
	return 0;
}

/* XXX INTERNAL PRIVATE IMPLEMENTATION FUNCTIONS XXX */

static dbi_driver_t *_get_driver(const char *filename) {
	dbi_driver_t *driver;
	void *dlhandle;
	const char **custom_functions_list;
	unsigned int idx = 0;
	dbi_custom_function_t *prevcustom = NULL;
	dbi_custom_function_t *custom = NULL;
	char function_name[256];

	dlhandle = dlopen(filename, DLOPEN_FLAG); /* DLOPEN_FLAG defined by autoconf */

	if (dlhandle == NULL) {
		return NULL;
	}
	else {
		driver = (dbi_driver_t *) malloc(sizeof(dbi_driver_t));
		if (!driver) return NULL;

		driver->dlhandle = dlhandle;
		driver->filename = strdup(filename);
		driver->next = NULL;
		driver->functions = (dbi_functions_t *) malloc(sizeof(dbi_functions_t));

		if ( /* nasty looking if block... is there a better way to do it? */
			((driver->functions->register_driver = dlsym(dlhandle, "dbd_register_driver")) == NULL) || dlerror() ||
			((driver->functions->initialize = dlsym(dlhandle, "dbd_initialize")) == NULL) || dlerror() ||
			((driver->functions->connect = dlsym(dlhandle, "dbd_connect")) == NULL) || dlerror() ||
			((driver->functions->disconnect = dlsym(dlhandle, "dbd_disconnect")) == NULL) || dlerror() ||
			((driver->functions->fetch_row = dlsym(dlhandle, "dbd_fetch_row")) == NULL) || dlerror() ||
			((driver->functions->free_query = dlsym(dlhandle, "dbd_free_query")) == NULL) || dlerror() ||
			((driver->functions->goto_row = dlsym(dlhandle, "dbd_goto_row")) == NULL) || dlerror() ||
			((driver->functions->get_socket = dlsym(dlhandle, "dbd_get_socket")) == NULL) || dlerror() ||
			((driver->functions->list_dbs = dlsym(dlhandle, "dbd_list_dbs")) == NULL) || dlerror() ||
			((driver->functions->list_tables = dlsym(dlhandle, "dbd_list_tables")) == NULL) || dlerror() ||
			((driver->functions->query = dlsym(dlhandle, "dbd_query")) == NULL) || dlerror() ||
			((driver->functions->query_null = dlsym(dlhandle, "dbd_query_null")) == NULL) || dlerror() ||
			((driver->functions->quote_string = dlsym(dlhandle, "dbd_quote_string")) == NULL) || dlerror() ||
			((driver->functions->select_db = dlsym(dlhandle, "dbd_select_db")) == NULL) || dlerror() ||
			((driver->functions->geterror = dlsym(dlhandle, "dbd_geterror")) == NULL) || dlerror()
			)
		{
			free(driver->functions);
			free(driver->filename);
			free(driver);
			return NULL;
		}
		driver->functions->register_driver(&driver->info, &custom_functions_list, &driver->reserved_words);
		driver->custom_functions = NULL; /* in case no custom functions are available */
		while (custom_functions_list && custom_functions_list[idx] != NULL) {
			custom = (dbi_custom_function_t *) malloc(sizeof(dbi_custom_function_t));
			if (!custom) {
				_free_custom_functions(driver);
				free(driver->functions);
				free(driver->filename);
				free(driver);
				return NULL;
			}
			custom->next = NULL;
			custom->name = custom_functions_list[idx];
			snprintf(function_name, 256, "dbd_%s", custom->name);
			custom->function_pointer = dlsym(dlhandle, function_name);
			if (!custom->function_pointer || dlerror()) {
				_free_custom_functions(driver);
				free(custom); /* not linked into the list yet */
				free(driver->functions);
				free(driver->filename);
				free(driver);
				return NULL;
			}
			if (driver->custom_functions == NULL) {
				driver->custom_functions = custom;
			}
			else {
				prevcustom->next = custom;
			}
			prevcustom = custom;
			idx++;
		}
	}
	return driver;
}

static void _free_custom_functions(dbi_driver_t *driver) {
	dbi_custom_function_t *cur;
	dbi_custom_function_t *next;

	if (!driver) return;
	cur = driver->custom_functions;

	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}

	driver->custom_functions = NULL;
}

static int _update_internal_conn_list(dbi_conn_t *conn, const int operation) {
	/* maintain internal linked list of conns so that we can unload them all
	 * when dbi is shutdown
	 * 
	 * operation = -1: remove conn
	 *           =  0: just look for conn (return 1 if found, -1 if not)
	 *           =  1: add conn */
	dbi_conn_t *curconn = rootconn;
	dbi_conn_t *prevconn = NULL;

	if ((operation == -1) || (operation == 0)) {
		while (curconn && (curconn != conn)) {
			prevconn = curconn;
			curconn = curconn->next;
		}
		if (!curconn) return -1;
		if (operation == 0) return 1;
		else if (operation == -1) {
			if (prevconn) prevconn->next = curconn->next;
			else rootconn = NULL;
			return 0;
		}
	}
	else if (operation == 1) {
		while (curconn && curconn->next) {
			curconn = curconn->next;
		}
		if (curconn) {
			curconn->next = conn;
		}
		else {
			rootconn = conn;
		}
		conn->next = NULL;
		return 0;
	}
	return -1;
}

static dbi_option_t *_find_or_create_option_node(dbi_conn Conn, const char *key) {
	dbi_option_t *prevoption = NULL;
	dbi_conn_t *conn = Conn;
	dbi_option_t *option = conn->options;

	while (option && strcasecmp(key, option->key)) {
		prevoption = option;
		option = option->next;
	}

	if (option == NULL) {
		/* allocate a new option node */
		option = (dbi_option_t *) malloc(sizeof(dbi_option_t));
		if (!option) return NULL;
		option->next = NULL;
		if (conn->options == NULL) {
		    conn->options = option;
		}
		else {
		    prevoption->next = option;
		}
	}

	return option;
}

void _error_handler(dbi_conn_t *conn) {
	int errno = 0;
	char *errmsg = NULL;
	int errstatus;
	
	errstatus = conn->driver->functions->geterror(conn, &errno, &errmsg);

	if (errstatus == -1) {
		/* not _really_ an error */
		return;
	}

	if (conn->error_message) free(conn->error_message);

	conn->error_number = errno;
	conn->error_message = errmsg;
	
	if (conn->error_handler != NULL) {
		/* trigger the external callback function */
		conn->error_handler(conn, conn->error_handler_argument);
	}
}

unsigned long _isolate_attrib(unsigned long attribs, unsigned long rangemin, unsigned rangemax) {
	/* hahaha! who woulda ever thunk strawberry's code would come in handy? */
	unsigned short startbit = log(rangemin)/log(2);
	unsigned short endbit = log(rangemax)/log(2);
	unsigned long attrib_mask = 0;
	int x;
	
	for (x = startbit; x <= endbit; x++)
		attrib_mask |= (1 << x);

	return (attribs & attrib_mask);
}

