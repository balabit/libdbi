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
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <dbi/dbi.h>

#define LIBDBI_VERSION "0.0.8"

#ifndef DBI_PLUGIN_DIR
#define DBI_PLUGIN_DIR "/usr/local/lib/dbd" /* use this default unless one is specified by the program */
#endif

/* declarations for internal functions */
dbi_plugin_t *_get_plugin(const char *filename);
void _free_custom_functions(dbi_plugin_t *plugin);
dbi_option_t *_find_or_create_option_node(dbi_driver_t *driver, const char *key);
void _error_handler(dbi_driver_t *driver);

/* declarations for variadic printf-like functions */
dbi_result_t *dbi_query(dbi_driver_t *driver, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));
dbi_result_t *dbi_efficient_query(dbi_driver_t *driver, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));

dbi_plugin_t *rootplugin;

/***********************************
 * PLUGIN INFRASTRUCTURE FUNCTIONS *
 ***********************************/

int dbi_initialize(const char *plugindir) {
	DIR *dir;
	struct dirent *plugin_dirent = NULL;
	struct stat statbuf;
	char fullpath[FILENAME_MAX];
	char *effective_plugindir;
	
	int num_loaded = 0;
	dbi_plugin_t *plugin = NULL;
	dbi_plugin_t *prevplugin = NULL;
	
	rootplugin = NULL;
	effective_plugindir = (plugindir ? (char *)plugindir : DBI_PLUGIN_DIR);
	dir = opendir(effective_plugindir);

	if (dir == NULL) {
		return -1;
	}
	else {
		while ((plugin_dirent = readdir(dir)) != NULL) {
			plugin = NULL;
			snprintf(fullpath, FILENAME_MAX, "%s/%s", effective_plugindir, plugin_dirent->d_name);
			if ((stat(fullpath, &statbuf)==0/*XXX*/) && S_ISREG(statbuf.st_mode) && (!strcmp(strrchr(plugin_dirent->d_name, '.'), ".so"))) {
				/* file is a stat'able regular file that ends in .so */
				plugin = _get_plugin(fullpath);
				if (plugin && (plugin->functions->initialize(plugin) != -1)) {
					if (!rootplugin) {
						rootplugin = plugin;
					}
					if (prevplugin) {
						prevplugin->next = plugin;
					}
					prevplugin = plugin;
					num_loaded++;
				}
				else {
					if (plugin && plugin->dlhandle) dlclose(plugin->dlhandle);
					if (plugin && plugin->functions) free(plugin->functions);
					if (plugin) free(plugin);
					plugin = NULL; /* don't include in linked list */
				}
			}
		}
		closedir(dir);
	}
	
	return num_loaded;
}

dbi_plugin_t *_get_plugin(const char *filename) {
	dbi_plugin_t *plugin;
	void *dlhandle;
	const char **custom_functions_list;
	unsigned int idx = 0;
	dbi_custom_function_t *prevcustom = NULL;
	dbi_custom_function_t *custom = NULL;
	char function_name[256];

	dlhandle = dlopen(filename, RTLD_NOW);

	if (dlhandle == NULL) {
		return NULL;
	}
	else {
		plugin = (dbi_plugin_t *) malloc(sizeof(dbi_plugin_t));
		if (!plugin) return NULL;

		plugin->dlhandle = dlhandle;
		plugin->filename = filename;
		plugin->next = NULL;
		plugin->functions = (dbi_functions_t *) malloc(sizeof(dbi_functions_t));

		if ( /* nasty looking if block... is there a better way to do it? */
			((plugin->functions->initialize = dlsym(dlhandle, "dbd_initialize")) == NULL) || dlerror() ||
			((plugin->functions->connect = dlsym(dlhandle, "dbd_connect")) == NULL) || dlerror() ||
			((plugin->functions->disconnect = dlsym(dlhandle, "dbd_disconnect")) == NULL) || dlerror() ||
			((plugin->functions->fetch_field = dlsym(dlhandle, "dbd_fetch_field")) == NULL) || dlerror() ||
			((plugin->functions->fetch_field_raw = dlsym(dlhandle, "dbd_fetch_field_raw")) == NULL) || dlerror() ||
			((plugin->functions->fetch_row = dlsym(dlhandle, "dbd_fetch_row")) == NULL) || dlerror() ||
			((plugin->functions->free_query = dlsym(dlhandle, "dbd_free_query")) == NULL) || dlerror() ||
			((plugin->functions->get_custom_functions_list = dlsym(dlhandle, "dbd_get_custom_functions_list")) == NULL) || dlerror() ||
			((plugin->functions->get_info = dlsym(dlhandle, "dbd_get_info")) == NULL) || dlerror() ||
			((plugin->functions->get_reserved_words_list = dlsym(dlhandle, "dbd_get_reserved_words_list")) == NULL) || dlerror() ||
			((plugin->functions->goto_row = dlsym(dlhandle, "dbd_goto_row")) == NULL) || dlerror() ||
			((plugin->functions->list_dbs = dlsym(dlhandle, "dbd_list_dbs")) == NULL) || dlerror() ||
			((plugin->functions->list_tables = dlsym(dlhandle, "dbd_list_tables")) == NULL) || dlerror() ||
			((plugin->functions->num_rows = dlsym(dlhandle, "dbd_num_rows")) == NULL) || dlerror() ||
			((plugin->functions->num_rows_affected = dlsym(dlhandle, "dbd_num_rows_affected")) == NULL) || dlerror() ||
			((plugin->functions->query = dlsym(dlhandle, "dbd_query")) == NULL) || dlerror() ||
			((plugin->functions->efficient_query = dlsym(dlhandle, "dbd_efficient_query")) == NULL) || dlerror() ||
			((plugin->functions->select_db = dlsym(dlhandle, "dbd_select_db")) == NULL) || dlerror() ||
			((plugin->functions->errstr = dlsym(dlhandle, "dbd_errstr")) == NULL) || dlerror() ||
			((plugin->functions->errno = dlsym(dlhandle, "dbd_errno")) == NULL) || dlerror()
			)
		{
			free(plugin->functions);
			free(plugin);
			return NULL;
		}
		plugin->info = plugin->functions->get_info();
		plugin->reserved_words = plugin->functions->get_reserved_words_list();
		custom_functions_list = plugin->functions->get_custom_functions_list();
		plugin->custom_functions = NULL; /* in case no custom functions are available */
		while (custom_functions_list[idx] != NULL) {
			custom = (dbi_custom_function_t *) malloc(sizeof(dbi_custom_function_t));
			if (!custom) {
				_free_custom_functions(plugin);
				free(plugin->functions);
				free(plugin);
				return NULL;
			}
			custom->next = NULL;
			custom->name = custom_functions_list[idx];
			snprintf(function_name, 256, "dbd_%s", custom->name);
			custom->function_pointer = dlsym(dlhandle, function_name);
			if (!custom->function_pointer || dlerror()) {
				_free_custom_functions(plugin);
				free(custom); /* not linked into the list yet */
				free(plugin->functions);
				free(plugin);
				return NULL;
			}
			if (plugin->custom_functions == NULL) {
				plugin->custom_functions = custom;
			}
			else {
				prevcustom->next = custom;
			}
			prevcustom = custom;
			idx++;
		}
	}		
	return plugin;
}

void _free_custom_functions(dbi_plugin_t *plugin) {
	dbi_custom_function_t *cur = plugin->custom_functions;
	dbi_custom_function_t *next;

	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}

	plugin->custom_functions = NULL;
}

dbi_plugin_t *dbi_list_plugins() {
	return rootplugin;
}

dbi_plugin_t *dbi_open_plugin(const char *name) {
	dbi_plugin_t *plugin = rootplugin;
	
	while (plugin) {
		if (strcasecmp(name, plugin->info->name) == 0) {
			return plugin;
		}
		else {
			plugin = plugin->next;
		}
	}

	return NULL;
}

dbi_driver_t *dbi_load_driver(const char *name) {
	dbi_plugin_t *plugin;
	dbi_driver_t *driver;

	plugin = dbi_open_plugin(name);
	driver = dbi_start_driver(plugin);

	return driver;
}

dbi_driver_t *dbi_start_driver(dbi_plugin_t *plugin) {
	dbi_driver_t *driver;

	driver = (dbi_driver_t *) malloc(sizeof(dbi_driver_t));
	if (!driver) {
		return NULL;
	}
	driver->plugin = plugin;
	driver->options = NULL;
	driver->connection = NULL;
	driver->current_db = NULL;
	driver->status = 0;
	driver->error_number = 0;
	driver->error_message = NULL;
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;

	return driver;
}

dbi_option_t *_find_or_create_option_node(dbi_driver_t *driver, const char *key) {
	dbi_option_t *prevoption = NULL;
	dbi_option_t *option = driver->options;
	int found = 0;

	while (option && !found) {
		if (strcasecmp(key, option->key) == 0) {
			found = 1;
		}
		else {
			prevoption = option;
			option = option->next;
		}
	}
	if (!found) {
		/* allocate a new option node */
		option = (dbi_option_t *) malloc(sizeof(dbi_option_t));
		if (!option) {
			return NULL;
		}
		option->next = NULL;
		if (driver->options == NULL) {
		    driver->options = option;
		}
		else {
		    prevoption->next = option;
		}
	}

	return option;
}

int dbi_set_option(dbi_driver_t *driver, const char *key, char *value) {
	dbi_option_t *option;
	
	option = _find_or_create_option_node(driver, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = strdup(value);
	option->numeric_value = 0;
	
	return 0;
}

int dbi_set_option_numeric(dbi_driver_t *driver, const char *key, int value) {
	dbi_option_t *option;
	
	option = _find_or_create_option_node(driver, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = NULL;
	option->numeric_value = value;
	
	return 0;
}

const char *dbi_get_option(dbi_driver_t *driver, const char *key) {
	dbi_option_t *option;

	option = driver->options;
	while (option) {
		if (strcasecmp(key, option->key) == 0) {
			return option->string_value;
		}
		option = option->next;
	}

	return NULL;
}

int dbi_get_option_numeric(dbi_driver_t *driver, const char *key) {
	dbi_option_t *option;

	option = driver->options;
	while (option) {
		if (strcasecmp(key, option->key) == 0) {
			return option->numeric_value;
		}
		option = option->next;
	}

	return 0;
}

void dbi_clear_options(dbi_driver_t *driver) {
	dbi_option_t *cur = driver->options;
	dbi_option_t *next;

	while (cur) {
		next = cur->next;
		free(cur->key);
		free(cur->string_value);
		free(cur);
		cur = next;
	}

	driver->options = NULL;
}

void *dbi_custom_function(dbi_plugin_t *plugin, const char *name) {
	dbi_custom_function_t *prevcustom = NULL;
	dbi_custom_function_t *custom = plugin->custom_functions;

	while (custom) {
		if (strcasecmp(name, custom->name) == 0) {
			return custom->function_pointer;
		}
		prevcustom = custom;
		custom = custom->next;
	}
	
	return NULL;
}

int dbi_is_reserved_word(dbi_plugin_t *plugin, const char *word) {
	unsigned int idx = 0;
	while (plugin->reserved_words[idx]) {
		if (strcasecmp(word, plugin->reserved_words[idx]) == 0) {
			return 1;
		}
	}
	return 0;
}

void dbi_close_driver(dbi_driver_t *driver) {
	driver->plugin->functions->disconnect(driver);
	driver->plugin = NULL;
	dbi_clear_options(driver);
	driver->connection = NULL;
	if (driver->current_db) free(driver->current_db);
	driver->status = 0;
	driver->error_number = 0;
	if (driver->error_message) free(driver->error_message); /* XXX: bleh? */
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;
	free(driver);
}

void dbi_shutdown() {
	dbi_plugin_t *cur = rootplugin;
	dbi_plugin_t *next;

	while (cur) {
		next = cur->next;
		dlclose(cur->dlhandle);
		free(cur->functions);
		_free_custom_functions(cur);
		//free(cur->filename); /* XXX: ??? */
		free(cur);
		cur = next;
	}

	rootplugin = NULL;
}

const char *dbi_plugin_name(dbi_plugin_t *plugin) {
	return plugin->info->name;
}

const char *dbi_plugin_filename(dbi_plugin_t *plugin) {
	return plugin->filename;
}

const char *dbi_plugin_description(dbi_plugin_t *plugin) {
	return plugin->info->description;
}

const char *dbi_plugin_maintainer(dbi_plugin_t *plugin) {
	return plugin->info->maintainer;
}

const char *dbi_plugin_url(dbi_plugin_t *plugin) {
	return plugin->info->url;
}

const char *dbi_plugin_version(dbi_plugin_t *plugin) {
	return plugin->info->version;
}

const char *dbi_plugin_date_compiled(dbi_plugin_t *plugin) {
	return plugin->info->date_compiled;
}

const char *dbi_version() {
	return "libdbi v" LIBDBI_VERSION;
}


/***********************
 * SQL LAYER FUNCTIONS *
 ***********************/

int dbi_connect(dbi_driver_t *driver) {
	int retval = driver->plugin->functions->connect(driver);
	if (retval == -1) {
		_error_handler(driver);
	}
	return retval;
}

int dbi_fetch_field(dbi_result_t *result, const char *key, void **dest) {
	int retval = result->driver->plugin->functions->fetch_field(result, key, dest);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

int dbi_fetch_field_raw(dbi_result_t *result, const char *key, unsigned char **dest) {
	int retval = result->driver->plugin->functions->fetch_field_raw(result, key, dest);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

int dbi_fetch_row(dbi_result_t *result) {
	int retval = result->driver->plugin->functions->fetch_row(result);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

int dbi_free_query(dbi_result_t *result) {
	int retval = result->driver->plugin->functions->free_query(result);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

int dbi_goto_row(dbi_result_t *result, unsigned int row) {
	int retval = result->driver->plugin->functions->goto_row(result, row);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

const char **dbi_list_dbs(dbi_driver_t *driver) {
	const char **retval = driver->plugin->functions->list_dbs(driver);
	if (retval == NULL) {
		_error_handler(driver);
	}
	return retval;
}

const char **dbi_list_tables(dbi_driver_t *driver, const char *db) {
	const char **retval = driver->plugin->functions->list_tables(driver, db);
	if (retval == NULL) {
		_error_handler(driver);
	}
	return retval;
}

unsigned int dbi_num_rows(dbi_result_t *result) {
	return result->numrows_matched;
}

unsigned int dbi_num_rows_affected(dbi_result_t *result) {
	return result->numrows_affected;
}

int vasprintf(char **, const char *, va_list); /* to shut up gcc */

dbi_result_t *dbi_query(dbi_driver_t *driver, const char *formatstr, ...) {
	char *statement;
	dbi_result_t *retval;
	va_list ap;
	
	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	retval = driver->plugin->functions->query(driver, statement);
	if (retval == NULL) {
		_error_handler(driver);
	}
	free(statement);
	return retval;
}

dbi_result_t *dbi_efficient_query(dbi_driver_t *driver, const char *formatstr, ...) {
	char *statement;
	dbi_result_t *retval;
	va_list ap;
	
	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	retval = driver->plugin->functions->efficient_query(driver, statement);
	if (retval == NULL) {
		_error_handler(driver);
	}
	free(statement);
	return retval;
}

int dbi_select_db(dbi_driver_t *driver, const char *db) {
	int retval = driver->plugin->functions->select_db(driver, db);
	if (retval == -1) {
		_error_handler(driver);
	}
	return retval;
}

int dbi_error(dbi_driver_t *driver, char *errmsg_dest) {
	char number_portion[20];
	char errmsg[512];
	if (driver->error_number) {
		snprintf(number_portion, 20, "%d: ", driver->error_number);
	}
	else {
		number_portion[0] = '\0';
	}
	snprintf(errmsg, 512, "%s%s", number_portion, driver->error_message);
	errmsg_dest = errmsg;
	return driver->error_number; /* XXX: handle lack of error number or string */
}

void dbi_error_handler(dbi_driver_t *driver, void *function, void *user_argument) {
	driver->error_handler = function;
	if (function == NULL) {
		driver->error_handler_argument = NULL;
	}
	else {
		driver->error_handler_argument = user_argument;
	}
}

void _error_handler(dbi_driver_t *driver) {
	int errno = driver->plugin->functions->errno(driver);
	const char *errmsg = driver->plugin->functions->errstr(driver);
	void (*errfunc)(dbi_driver_t *, void *);

	if (errno) {
		driver->error_number = errno;
	}
	if (errmsg) {
		driver->error_message = (char *) errmsg;
	}
	if (driver->error_handler != NULL) {
		/* trigger the external callback function */
		errfunc = driver->error_handler;
		errfunc(driver, driver->error_handler_argument);
	}
}

