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

#ifndef DBI_PLUGIN_DIR
#define DBI_PLUGIN_DIR "/usr/local/lib/dbd" /* use this as the default */
#endif

/* declarations for internal functions -- anything declared as static won't be accessible by name from client programs */
static dbi_plugin_t *_get_plugin(const char *filename);
static void _free_custom_functions(dbi_plugin_t *plugin);
static dbi_option_t *_find_or_create_option_node(dbi_driver Driver, const char *key);
static int _update_internal_driver_list(dbi_driver_t *driver, int operation);

dbi_result dbi_driver_query(dbi_driver Driver, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));

static const char *ERROR = "ERROR";
static dbi_plugin_t *rootplugin;
static dbi_driver_t *rootdriver;

/* XXX DBI CORE FUNCTIONS XXX */

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
			if ((stat(fullpath, &statbuf) == 0) && S_ISREG(statbuf.st_mode) && (!strcmp(strrchr(plugin_dirent->d_name, '.'), PLUGIN_EXT))) {
				/* file is a stat'able regular file that ends in .so (or appropriate dynamic library extension) */
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

void dbi_shutdown() {
	dbi_driver_t *curdriver = rootdriver;
	dbi_driver_t *nextdriver;
	
	dbi_plugin_t *curplugin = rootplugin;
	dbi_plugin_t *nextplugin;
	
	while (curdriver) {
		nextdriver = curdriver->next;
		dbi_driver_close((dbi_driver)curdriver);
		curdriver = nextdriver;
	}
	
	while (curplugin) {
		nextplugin = curplugin->next;
		dlclose(curplugin->dlhandle);
		free(curplugin->functions);
		_free_custom_functions(curplugin);
		free(curplugin->filename);
		free(curplugin);
		curplugin = nextplugin;
	}

	rootplugin = NULL;
}

const char *dbi_version() {
	return "libdbi v" VERSION;
}

/* XXX PLUGIN FUNCTIONS XXX */

dbi_plugin dbi_plugin_list(dbi_plugin Current) {
	dbi_plugin_t *current = Current;

	if (current == NULL) {
		return (dbi_plugin)rootplugin;
	}

	return (dbi_plugin)current->next;
}

dbi_plugin dbi_plugin_open(const char *name) {
	dbi_plugin_t *plugin = rootplugin;

	while (plugin && strcasecmp(name, plugin->info->name)) {
		plugin = plugin->next;
	}

	return plugin;
}

int dbi_plugin_is_reserved_word(dbi_plugin Plugin, const char *word) {
	unsigned int idx = 0;
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return 0;
	
	while (plugin->reserved_words[idx]) {
		if (strcasecmp(word, plugin->reserved_words[idx]) == 0) {
			return 1;
		}
	}
	return 0;
}

void *dbi_plugin_specific_function(dbi_plugin Plugin, const char *name) {
	dbi_plugin_t *plugin = Plugin;
	dbi_custom_function_t *custom;

	if (!plugin) return NULL;

	custom = plugin->custom_functions;
	
	while (custom && strcasecmp(name, custom->name)) {
		custom = custom->next;
	}

	return custom ? custom->function_pointer : NULL;
}

/* PLUGIN: informational functions */

const char *dbi_plugin_get_name(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;
	
	return plugin->info->name;
}

const char *dbi_plugin_get_filename(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;
	
	return plugin->filename;
}

const char *dbi_plugin_get_description(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;
	
	return plugin->info->description;
}

const char *dbi_plugin_get_maintainer(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;

	return plugin->info->maintainer;
}

const char *dbi_plugin_get_url(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;

	if (!plugin) return ERROR;

	return plugin->info->url;
}

const char *dbi_plugin_get_version(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;
	
	return plugin->info->version;
}

const char *dbi_plugin_get_date_compiled(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	
	if (!plugin) return ERROR;
	
	return plugin->info->date_compiled;
}

int dbi_plugin_quote_string(dbi_plugin Plugin, char **orig) {
	dbi_plugin_t *plugin = Plugin;
	char *temp;
	char *newstr;
	int newlen;
	
	if (!plugin || !orig || !*orig) return -1;

	newstr = malloc((strlen(*orig)*2)+4+1); /* worst case, we have to escape every character and add 2*2 surrounding quotes */
	
	newlen = plugin->functions->quote_string(plugin, *orig, newstr);
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

dbi_driver dbi_driver_new(const char *name) {
	dbi_plugin plugin;
	dbi_driver driver;

	plugin = dbi_plugin_open(name);
	driver = dbi_driver_open(plugin);

	return driver;
}

dbi_driver dbi_driver_open(dbi_plugin Plugin) {
	dbi_plugin_t *plugin = Plugin;
	dbi_driver_t *driver;
	
	if (!plugin) {
		return NULL;
	}

	driver = (dbi_driver_t *) malloc(sizeof(dbi_driver_t));
	if (!driver) {
		return NULL;
	}
	driver->plugin = plugin;
	driver->options = NULL;
	driver->connection = NULL;
	driver->current_db = NULL;
	driver->error_number = 0;
	driver->error_message = NULL;
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;
	_update_internal_driver_list(driver, 1);

	return (dbi_driver)driver;
}

void dbi_driver_close(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return;
	
	_update_internal_driver_list(driver, -1);
	
	driver->plugin->functions->disconnect(driver);
	driver->plugin = NULL;
	dbi_driver_clear_options(Driver);
	driver->connection = NULL;
	
	if (driver->current_db) free(driver->current_db);
	if (driver->error_message) free(driver->error_message);
	driver->error_number = 0;
	
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;
	free(driver);
}

dbi_plugin dbi_driver_get_plugin(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return NULL;
	
	return driver->plugin;
}

int dbi_driver_error(dbi_driver Driver, char **errmsg_dest) {
	dbi_driver_t *driver = Driver;
	char number_portion[20];
	char *errmsg;
	
	if (driver->error_number) {
		snprintf(number_portion, 20, "%d: ", driver->error_number);
	}
	else {
		number_portion[0] = '\0';
	}

	asprintf(&errmsg, "%s%s", number_portion, driver->error_message);
	*errmsg_dest = errmsg;

	return driver->error_number;
}

void dbi_driver_error_handler(dbi_driver Driver, void *function, void *user_argument) {
	dbi_driver_t *driver = Driver;
	driver->error_handler = function;
	if (function == NULL) {
		driver->error_handler_argument = NULL;
	}
	else {
		driver->error_handler_argument = user_argument;
	}
}

/* DRIVER: option manipulation */

int dbi_driver_set_option(dbi_driver Driver, const char *key, char *value) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *option;
	
	if (!driver) {
		return -1;
	}
	
	option = _find_or_create_option_node(driver, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = strdup(value);
	option->numeric_value = 0;
	
	return 0;
}

int dbi_driver_set_option_numeric(dbi_driver Driver, const char *key, int value) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *option;
	
	if (!driver) {
		return -1;
	}
	
	option = _find_or_create_option_node(driver, key);
	if (!option) {
		return -1;
	}
	
	option->key = strdup(key);
	option->string_value = NULL;
	option->numeric_value = value;
	
	return 0;
}

const char *dbi_driver_get_option(dbi_driver Driver, const char *key) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *option;

	if (!driver) {
		return NULL;
	}
	
	option = driver->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	return option ? option->string_value : NULL;
}

int dbi_driver_get_option_numeric(dbi_driver Driver, const char *key) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *option;

	if (!driver) return -1;
	
	option = driver->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	return option ? option->numeric_value : -1;
}

const char *dbi_driver_get_option_list(dbi_driver Driver, const char *current) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *option;
	
	if (driver && driver->options) option = driver->options;
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

void dbi_driver_clear_option(dbi_driver Driver, const char *key) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *prevoption;
	dbi_option_t *option;
	
	if (!driver) return;
	option = driver->options;
	
	while (option && strcasecmp(key, option->key)) {
		prevoption = option;
		option = option->next;
	}
	if (!option) return;
	if (option == driver->options) {
		driver->options = option->next;
	}
	else {
		prevoption->next = option->next;
	}
	free(option->key);
	free(option->string_value);
	free(option);
	return;
}

void dbi_driver_clear_options(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	dbi_option_t *cur;
	dbi_option_t *next;

	if (!driver) return;
	cur = driver->options;
	
	while (cur) {
		next = cur->next;
		free(cur->key);
		free(cur->string_value);
		free(cur);
		cur = next;
	}

	driver->options = NULL;
}

/* DRIVER: SQL layer functions */

int dbi_driver_connect(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	int retval;
	
	if (!driver) return -1;
	
	retval = driver->plugin->functions->connect(driver);
	//if (retval == -1) {			XXX cant call error handler when connection is already failed and terminated
	//	_error_handler(driver);
	//}
	return retval;
}

dbi_result dbi_driver_get_db_list(dbi_driver Driver, const char *pattern) {
	dbi_driver_t *driver = Driver;
	dbi_result_t *result;
	
	if (!driver) return NULL;
	
	result = driver->plugin->functions->list_dbs(driver, pattern);
	
	if (result == NULL) {
		_error_handler(driver);
	}

	return (dbi_result)result;
}

dbi_result dbi_driver_get_table_list(dbi_driver Driver, const char *db) {
	dbi_driver_t *driver = Driver;
	dbi_result_t *result;
	
	if (!driver) return NULL;
	
	result = driver->plugin->functions->list_tables(driver, db);
	
	if (result == NULL) {
		_error_handler(driver);
	}
	
	return (dbi_result)result;
}

dbi_result dbi_driver_query(dbi_driver Driver, const char *formatstr, ...) {
	dbi_driver_t *driver = Driver;
	char *statement;
	dbi_result_t *result;
	va_list ap;

	if (!driver) return NULL;
	
	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	result = driver->plugin->functions->query(driver, statement);

	if (result == NULL) {
		_error_handler(driver);
	}
	free(statement);
	
	return (dbi_result)result;
}

int dbi_driver_select_db(dbi_driver Driver, const char *db) {
	dbi_driver_t *driver = Driver;
	char *retval;
	
	if (!driver) return -1;
	
	free(driver->current_db);
	driver->current_db = NULL;
	
	retval = driver->plugin->functions->select_db(driver, db);
	
	if (retval == NULL) {
		_error_handler(driver);
	}
	
	if (retval[0] == '\0') {
		/* if "" was returned, driver doesn't support switching databases */
		return -1;
	}
	else {
		driver->current_db = strdup(retval);
	}
	
	return 0;
}

/* XXX INTERNAL PRIVATE IMPLEMENTATION FUNCTIONS XXX */

static dbi_plugin_t *_get_plugin(const char *filename) {
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
		plugin->filename = strdup(filename);
		plugin->next = NULL;
		plugin->functions = (dbi_functions_t *) malloc(sizeof(dbi_functions_t));

		if ( /* nasty looking if block... is there a better way to do it? */
			((plugin->functions->register_plugin = dlsym(dlhandle, "dbd_register_plugin")) == NULL) || dlerror() ||
			((plugin->functions->initialize = dlsym(dlhandle, "dbd_initialize")) == NULL) || dlerror() ||
			((plugin->functions->connect = dlsym(dlhandle, "dbd_connect")) == NULL) || dlerror() ||
			((plugin->functions->disconnect = dlsym(dlhandle, "dbd_disconnect")) == NULL) || dlerror() ||
			((plugin->functions->fetch_row = dlsym(dlhandle, "dbd_fetch_row")) == NULL) || dlerror() ||
			((plugin->functions->free_query = dlsym(dlhandle, "dbd_free_query")) == NULL) || dlerror() ||
			((plugin->functions->goto_row = dlsym(dlhandle, "dbd_goto_row")) == NULL) || dlerror() ||
			((plugin->functions->list_dbs = dlsym(dlhandle, "dbd_list_dbs")) == NULL) || dlerror() ||
			((plugin->functions->list_tables = dlsym(dlhandle, "dbd_list_tables")) == NULL) || dlerror() ||
			((plugin->functions->query = dlsym(dlhandle, "dbd_query")) == NULL) || dlerror() ||
			((plugin->functions->quote_string = dlsym(dlhandle, "dbd_quote_string")) == NULL) || dlerror() ||
			((plugin->functions->select_db = dlsym(dlhandle, "dbd_select_db")) == NULL) || dlerror() ||
			((plugin->functions->geterror = dlsym(dlhandle, "dbd_geterror")) == NULL) || dlerror()
			)
		{
			free(plugin->functions);
			free(plugin->filename);
			free(plugin);
			return NULL;
		}
		plugin->functions->register_plugin(&plugin->info, &custom_functions_list, &plugin->reserved_words);
		plugin->custom_functions = NULL; /* in case no custom functions are available */
		while (custom_functions_list && custom_functions_list[idx] != NULL) {
			custom = (dbi_custom_function_t *) malloc(sizeof(dbi_custom_function_t));
			if (!custom) {
				_free_custom_functions(plugin);
				free(plugin->functions);
				free(plugin->filename);
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
				free(plugin->filename);
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

static void _free_custom_functions(dbi_plugin_t *plugin) {
	dbi_custom_function_t *cur;
	dbi_custom_function_t *next;

	if (!plugin) return;
	cur = plugin->custom_functions;

	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}

	plugin->custom_functions = NULL;
}

static int _update_internal_driver_list(dbi_driver_t *driver, const int operation) {
	/* maintain internal linked list of drivers so that we can unload them all
	 * when dbi is shutdown
	 * 
	 * operation = -1: remove driver
	 *           =  0: just look for driver (return 1 if found, -1 if not)
	 *           =  1: add driver */
	dbi_driver_t *curdriver = rootdriver;
	dbi_driver_t *prevdriver = NULL;

	if ((operation == -1) || (operation == 0)) {
		while (curdriver && (curdriver != driver)) {
			prevdriver = curdriver;
			curdriver = curdriver->next;
		}
		if (!curdriver) return -1;
		if (operation == 0) return 1;
		else if (operation == -1) {
			if (prevdriver) prevdriver->next = curdriver->next;
			else rootdriver = NULL;
			return 0;
		}
	}
	else if (operation == 1) {
		while (curdriver && curdriver->next) {
			curdriver = curdriver->next;
		}
		if (curdriver) {
			curdriver->next = driver;
		}
		else {
			rootdriver = driver;
		}
		driver->next = NULL;
		return 0;
	}
	return -1;
}

static dbi_option_t *_find_or_create_option_node(dbi_driver Driver, const char *key) {
	dbi_option_t *prevoption = NULL;
	dbi_driver_t *driver = Driver;
	dbi_option_t *option = driver->options;

	while (option && strcasecmp(key, option->key)) {
		prevoption = option;
		option = option->next;
	}

	if (option == NULL) {
		/* allocate a new option node */
		option = (dbi_option_t *) malloc(sizeof(dbi_option_t));
		if (!option) return NULL;
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

void _error_handler(dbi_driver_t *driver) {
	int errno = 0;
	char *errmsg = NULL;
	void (*errfunc)(dbi_driver_t *, void *);
	int errstatus;
	
	errstatus = driver->plugin->functions->geterror(driver, &errno, &errmsg);

	if (errno) {
		driver->error_number = errno;
	}
	if (errmsg) {
		driver->error_message = errmsg;
	}
	if (driver->error_handler != NULL) {
		/* trigger the external callback function */
		errfunc = driver->error_handler;
		errfunc(driver, driver->error_handler_argument);
	}
}

unsigned long _isolate_attrib(unsigned long attribs, unsigned long rangemin, unsigned rangemax) {
	/* hahaha! who woulda ever thunk strawberry's code would come in handy? --David */
	unsigned short startbit = log(rangemin)/log(2);
	unsigned short endbit = log(rangemax)/log(2);
	unsigned long attrib_mask = 0;
	int x;
	
	for (x = startbit; x <= endbit; x++)
		attrib_mask |= (unsigned long) pow(2, x);

	return (attribs & attrib_mask);
}

#ifndef HAVE_ATOLL
long long int atoll(const char *nptr) {
	long long int tmp = 0;
	int curpos;
	int base10 = 0;
	char fakestr[2];
	
	if (!str || strlen(nptr) < 1) return 0;
	fakestr[1] = '\0';
	for (curpos = strlen(nptr)-1; curpos >= 0; curpos--,base10++) {
		fakestr[0] = nptr[curpos];
		tmp += atol(fakestr) * (long)pow(10, base10);
	}
	return tmp;
}
#endif

