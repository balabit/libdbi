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

#define LIBDBI_VERSION "0.0.8"

#ifndef DBI_PLUGIN_DIR
#define DBI_PLUGIN_DIR "/usr/local/lib/dbd" /* use this default unless one is specified by the program */
#endif

/* declarations for internal functions */
dbi_plugin_t *_get_plugin(const char *filename);
void _free_custom_functions(dbi_plugin_t *plugin);
dbi_option_t *_find_or_create_option_node(dbi_driver_t *driver, const char *key);
void _error_handler(dbi_driver_t *driver);
int _update_internal_driver_list(dbi_driver_t *driver, int operation);
dbi_field_t *_find_field_node(dbi_field_t *first, const char *name);
struct _field_binding_s *_field_binding_t_pointer;
_field_binding_t_pointer _find_or_create_binding_node(dbi_result_t *result, const char *field);
void _remove_binding_node(dbi_result_t *result, _field_binding_t_pointer deadbinding);
void _bind_helper_char(_field_binding_t *binding);
void _bind_helper_uchar(_field_binding_t *binding);
void _bind_helper_short(_field_binding_t *binding);
void _bind_helper_ushort(_field_binding_t *binding);
void _bind_helper_long(_field_binding_t *binding);
void _bind_helper_ulong(_field_binding_t *binding);
void _bind_helper_longlong(_field_binding_t *binding);
void _bind_helper_ulonglong(_field_binding_t *binding);
void _bind_helper_float(_field_binding_t *binding);
void _bind_helper_double(_field_binding_t *binding);
void _bind_helper_string(_field_binding_t *binding);
void _bind_helper_binary(_field_binding_t *binding);
void _bind_helper_string_copy(_field_binding_t *binding);
void _bind_helper_binary_copy(_field_binding_t *binding);
void _bind_helper_set(_field_binding_t *binding);
void _bind_helper_enum(_field_binding_t *binding);

static const char *ERROR = "ERROR";

/* declarations for variadic printf-like functions */
dbi_result_t *dbi_query(dbi_driver_t *driver, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));

dbi_plugin_t *rootplugin;
dbi_driver_t *rootdriver;

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
			/* ((plugin->functions->fetch_field = dlsym(dlhandle, "dbd_fetch_field")) == NULL) || dlerror() || */
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
	if (!plugin) return;
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

int _update_internal_driver_list(dbi_driver_t *driver, const int operation) {
	/* maintain internal linked list of drivers so that we can unload them all
	 * when dbi is shutdown
	 * 
	 * operation = -1: remove driver
	 *           =  0: just look for driver (return 1 if found, -1 if not)
	 *           =  1: add driver */
	dbi_driver_t *curdriver;
	int notfound = 1;
	dbi_driver_t *prevdriver;

	if ((operation == -1) || (operation == 0)) {
		curdriver = rootdriver;
		while (notfound && curdriver && curdriver->next) {
			if (curdriver == driver) {
				notfound = 0;
			}
			else {
				prevdriver = curdriver;
				curdriver = curdriver->next;
			}
		}
		if (notfound) {
			return -1;
		}
		if (operation == 0) {
			return 1;
		}
		if (operation == -1) {
			prevdriver->next = curdriver->next;
			return 0;
		}
	}
	else if (operation == 1) {
		curdriver = rootdriver;
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

dbi_driver_t *dbi_start_driver(dbi_plugin_t *plugin) {
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
	driver->status = 0;
	driver->error_number = 0;
	driver->error_message = NULL;
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;
	_update_internal_driver_list(driver, 1);

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

int dbi_set_option_numeric(dbi_driver_t *driver, const char *key, int value) {
	dbi_option_t *option;
	
	if (!option) {
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

const char *dbi_get_option(dbi_driver_t *driver, const char *key) {
	dbi_option_t *option;

	if (!driver) {
		return NULL;
	}
	
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

	if (!driver) {
		return ~0;
	}
	
	option = driver->options;
	while (option) {
		if (strcasecmp(key, option->key) == 0) {
			return option->numeric_value;
		}
		option = option->next;
	}

	return ~0;
}

void dbi_clear_options(dbi_driver_t *driver) {
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

dbi_option_t *dbi_list_options(dbi_driver_t *driver) {
	if (!driver) return NULL;
	return driver->options;
}

void *dbi_custom_function(dbi_plugin_t *plugin, const char *name) {
	dbi_custom_function_t *prevcustom = NULL;
	dbi_custom_function_t *custom;

	if (!plugin) return NULL;
	custom = plugin->custom_functions;
	
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
	if (!plugin) return 0;
	while (plugin->reserved_words[idx]) {
		if (strcasecmp(word, plugin->reserved_words[idx]) == 0) {
			return 1;
		}
	}
	return 0;
}

void dbi_close_driver(dbi_driver_t *driver) {
	if (!driver) return;
	_update_internal_driver_list(driver, -1);
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
	dbi_driver_t *curdriver = rootdriver;
	dbi_driver_t *nextdriver;
	
	dbi_plugin_t *curplugin = rootplugin;
	dbi_plugin_t *nextplugin;
	
	while (curdriver) {
		nextdriver = curdriver->next;
		dbi_close_driver(curdriver);
		curdriver = nextdriver;
	}
	
	while (curplugin) {
		nextplugin = curplugin->next;
		dlclose(curplugin->dlhandle);
		free(curplugin->functions);
		_free_custom_functions(curplugin);
		//free(cur->filename); /* XXX: ??? */
		free(curplugin);
		curplugin = nextplugin;
	}

	rootplugin = NULL;
}

const char *dbi_plugin_name(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->name;
}

const char *dbi_plugin_filename(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->filename;
}

const char *dbi_plugin_description(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->description;
}

const char *dbi_plugin_maintainer(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->maintainer;
}

const char *dbi_plugin_url(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->url;
}

const char *dbi_plugin_version(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->version;
}

const char *dbi_plugin_date_compiled(dbi_plugin_t *plugin) {
	if (!plugin) return ERROR;
	return plugin->info->date_compiled;
}

const char *dbi_version() {
	return "libdbi v" LIBDBI_VERSION;
}


/***********************
 * SQL LAYER FUNCTIONS *
 ***********************/

int dbi_connect(dbi_driver_t *driver) {
	int retval;
	if (!driver) return -1;
	retval = driver->plugin->functions->connect(driver);
	if (retval == -1) {
		_error_handler(driver);
	}
	return retval;
}

int dbi_fetch_row(dbi_result_t *result) {
	int retval;
	if (!result) return -1;
	retval = result->driver->plugin->functions->fetch_row(result);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

int dbi_free_query(dbi_result_t *result) {
	int retval;
	if (!result) return -1;
	retval = result->driver->plugin->functions->free_query(result);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

unsigned int dbi_get_size(dbi_result_t *result, const char *field) {
	int idx = 0;
	int notfound = 1;
	char *curfieldname;
/*	for the old school API...
	if (result && result->row->field_names && result->row->field_sizes) {
		curfieldname = result->row->field_names;
		while (curfieldname && notfound) {
			if (strcasecmp(curfieldname, field) == 0) {
				notfound = 0;
			}
			curfieldname = result->row->field_names[idx+1];
			idx++;
		}
		if (notfound) {
			return ~0;
		}
		return result->row->field_sizes[idx-1];
	} */
	dbi_field_t *curfield;
	if (result && result->row && result->row->fields) {
		curfield = result->row->fields;
		while (curfield && curfield->next) {
			if (strcasecmp(curfield->name, field) == 0) {
				return curfield->size;
			}
			curfield = curfield->next;
		}
	}
	return ~0;
}

unsigned int dbi_get_length(dbi_result_t *result, const char *field) {
	unsigned int size = dbi_get_size(result, field);
	if (size == 0) {
		return 0;
	}
	if (size != ~0) {
		return size-1;
	}
	return ~0;
}

int dbi_goto_row(dbi_result_t *result, unsigned int row) {
	int retval;
	if (!result) return -1;
	retval = result->driver->plugin->functions->goto_row(result, row);
	if (retval == -1) {
		_error_handler(result->driver);
	}
	return retval;
}

dbi_result_t *dbi_list_dbs(dbi_driver_t *driver) {
	dbi_result_t *result;
	if (!driver) return NULL;
	result = driver->plugin->functions->list_dbs(driver);
	if (result == NULL) {
		_error_handler(driver);
	}
	return result;
}

dbi_result_t *dbi_list_tables(dbi_driver_t *driver, const char *db) {
	dbi_result_t *result;
	if (!driver) return NULL;
	result = driver->plugin->functions->list_tables(driver, db);
	if (result == NULL) {
		_error_handler(driver);
	}
	return result;
}

unsigned int dbi_num_rows(dbi_result_t *result) {
	if (!result) return ~0;
	return result->numrows_matched;
}

unsigned int dbi_num_rows_affected(dbi_result_t *result) {
	if (!result) return ~0;
	return result->numrows_affected;
}

int vasprintf(char **, const char *, va_list); /* to shut up gcc */

dbi_result_t *dbi_query(dbi_driver_t *driver, const char *formatstr, ...) {
	char *statement;
	dbi_result_t *retval;
	va_list ap;

	if (!driver) return NULL;
	
	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	if ((dbi_get_option_numeric(driver, "efficient-queries") == 1) && (driver->plugin->functions->efficient_query)) {
		retval = driver->plugin->functions->efficient_query(driver, statement);
	}
	else {
		retval = driver->plugin->functions->query(driver, statement);
	}

	if (retval == NULL) {
		_error_handler(driver);
	}
	free(statement);
	return retval;
}

int dbi_select_db(dbi_driver_t *driver, const char *db) {
	int retval;
	if (!driver) return -1;
	retval = driver->plugin->functions->select_db(driver, db);
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

dbi_field_t *_find_field_node(dbi_field_t *first, const char *name) {
	dbi_field_t *curfield = first;
	while (curfield) {
		if (strcasecmp(curfield->name, name) == 0) {
			return curfield;
		}
		curfield = curfield->next;
	}
	return NULL;
}

signed char dbi_get_char(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	signed char ERROR = CHAR_MAX;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_INTEGER) {
		switch (field->attributes) {
			case DBI_INTEGER_SIZE1:
				return field->data.d_char;
				break;
			case DBI_INTEGER_SIZE2:
			case DBI_INTEGER_SIZE3:
			case DBI_INTEGER_SIZE4:
			case DBI_INTEGER_SIZE8:
				return ERROR;
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

short dbi_get_short(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	short ERROR = SHRT_MAX;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_INTEGER) {
		switch (field->attributes) {
			case DBI_INTEGER_SIZE1:
			case DBI_INTEGER_SIZE2:
				return field->data.d_short;
				break;
			case DBI_INTEGER_SIZE3:
			case DBI_INTEGER_SIZE4:
			case DBI_INTEGER_SIZE8:
				return ERROR;
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

long dbi_get_long(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	long ERROR = LONG_MAX;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_INTEGER) {
		switch (field->attributes) {
			case DBI_INTEGER_SIZE1:
			case DBI_INTEGER_SIZE2:
			case DBI_INTEGER_SIZE3:
			case DBI_INTEGER_SIZE4:
				return field->data.d_long;
				break;
			case DBI_INTEGER_SIZE8:
				return ERROR;
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

long long dbi_get_longlong(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	long long ERROR = (long long)(~0); /* no appropriate foo_MAX constant */
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_INTEGER) {
		switch (field->attributes) {
			case DBI_INTEGER_SIZE1:
			case DBI_INTEGER_SIZE2:
			case DBI_INTEGER_SIZE3:
			case DBI_INTEGER_SIZE4:
			case DBI_INTEGER_SIZE8:
				return field->data.d_longlong;
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

unsigned char dbi_get_uchar(dbi_result_t *result, const char *fieldname) {
	return (unsigned char)dbi_get_char(result, fieldname);
}

unsigned short dbi_get_ushort(dbi_result_t *result, const char *fieldname) {
	return (unsigned short)dbi_get_short(result, fieldname);
}

unsigned long dbi_get_ulong(dbi_result_t *result, const char *fieldname) {
	return (unsigned long)dbi_get_long(result, fieldname);
}

unsigned long long dbi_get_ulonglong(dbi_result_t *result, const char *fieldname) {
	return (unsigned long long)dbi_get_longlong(result, fieldname);
}

float dbi_get_float(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	float ERROR = FLT_MAX;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_DECIMAL) {
		switch (field->attributes) {
			case DBI_DECIMAL_SIZE4:
				return field->data.d_float;
			case DBI_DECIMAL_SIZE8:
				return ERROR;
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

double dbi_get_double(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	double ERROR = DBL_MAX;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_DECIMAL) {
		switch (field->attributes) {
			case DBI_DECIMAL_SIZE4:
				return (double)(field->data.d_float); /* byte ordering doesn't overlap for float/double like it does for ints */
				break;
			case DBI_DECIMAL_SIZE8:
				return (field->data.d_double);
				break;
			default:
				return ERROR;
		}
	}
	return ERROR;
}

const char *dbi_get_string(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	const char *ERROR = "ERROR";
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_STRING) {
		return (const char)(field->data.d_string);
	}
	return ERROR;
}

const unsigned char *dbi_get_binary(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	const unsigned char *ERROR = "ERROR";
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_BINARY) {
		return (const unsigned char)(field->data.d_string);
	}
	return ERROR;
}

char *dbi_get_string_copy(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	const char *ERROR = "ERROR";
	char *newstring = NULL;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_STRING) {
		newstring = strdup(field->data.d_string);
		if (newstring == NULL) {
			newstring = strdup(ERROR);
		}
		return newstring;
	}
	return strdup(ERROR);
}

unsigned char *dbi_get_binary_copy(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	unsigned char *ERROR = "ERROR";
	unsigned char *newblob = NULL;
	unsigned int length;
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_BINARY) {
		length = field->size;
		newblob = malloc(length);
		if (newblob == NULL) {
			newblob = strdup(ERROR);
		}
		else {
			memcpy(newblob, field->data.d_string, length);
		}
		return newblob;
	}
	return strdup(ERROR);
}

const char *dbi_get_enum(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	const char *ERROR = "ERROR";
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_ENUM) {
		return (const char)(field->data.d_string); /* XXX */
	}
	return ERROR;
}

const char *dbi_get_set(dbi_result_t *result, const char *fieldname) {
	dbi_field_t *field;
	const char *ERROR = "ERROR";
	
	field = _find_field_node(result->row->fields, fieldname);
	if (!field) return -1;
	
	if (field->type == DBI_TYPE_SET) {
		return (const char)(field->data.d_string); /* XXX */
	}
	return ERROR;
}

/* dbi_bind functions */

struct _field_binding_s {
	void *(helper_function)(_field_binding_t *);
	dbi_result_t *result;
	const char *field;
	void *bindto;
	_field_binding_s *next;
} _field_binding_t;

_field_binding_t *_find_or_create_binding_node(dbi_result_t *result, const char *field) {
	_field_binding_t *prevbinding = NULL;
	_field_binding_t *binding = (_field_binding_t *)(result->field_bindings);
	int found = 0;

	while (binding && !found) {
		if (strcasecmp(field, binding->field) == 0) {
			found = 1;
		}
		else {
			prevbinding = binding;
			binding = binding->next;
		}
	}
	if (!found) {
		/* allocate a new option node */
		binding = (_field_binding_t *) malloc(sizeof(_field_binding_t));
		if (!binding) {
			return NULL;
		}
		binding->result = result;
		binding->field = strdup(field);
		binding->next = NULL;
		if (result->field_bindings == NULL) {
		    result->field_bindings = binding;
		}
		else {
		    prevbinding->next = binding;
		}
	}

	return binding;
}

void _remove_binding_node(dbi_result_t *result, _field_binding_t *deadbinding) {
	_field_binding_t *prevbinding = NULL;
	_field_binding_t *binding = (_field_binding_t *)(result->field_bindings);
	int found = 0;

	while (binding && !found) {
		if (binding == deadbinding) {
			found = 1;
		}
		else {
			prevbinding = binding;
			binding = binding->next;
		}
	}
	if (!found) {
		/* this should never ever happen. silently pretend it never did. */
		return;
	}
	free(binding->field);
	if (result->field_bindings == deadbinding) {
		result->field_bindings = NULL;
	}
	else {
		prevbinding->next = deadbinding->next;
	}
	free(deadbinding);
}

/* bind helpers */

void _bind_helper_char(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_char(binding->result, binding->field);
}

void _bind_helper_uchar(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_uchar(binding->result, binding->field);
}

void _bind_helper_short(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_short(binding->result, binding->field);
}

void _bind_helper_ushort(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_ushort(binding->result, binding->field);
}

void _bind_helper_long(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_long(binding->result, binding->field);
}

void _bind_helper_ulong(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_ulong(binding->result, binding->field);
}

void _bind_helper_longlong(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_longlong(binding->result, binding->field);
}

void _bind_helper_ulonglong(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_ulonglong(binding->result, binding->field);
}

void _bind_helper_float(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_float(binding->result, binding->field);
}

void _bind_helper_double(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_double(binding->result, binding->field);
}

void _bind_helper_string(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_string(binding->result, binding->field);
}

void _bind_helper_binary(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_binary(binding->result, binding->field);
}

void _bind_helper_string_copy(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_string_copy(binding->result, binding->field);
}

void _bind_helper_binary_copy(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_binary_copy(binding->result, binding->field);
}

void _bind_helper_set(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_set(binding->result, binding->field);
}

void _bind_helper_enum(_field_binding_t *binding) {
	*(binding->bindto) = dbi_get_enum(binding->result, binding->field);
}

int dbi_bind_char(dbi_result_t *result, const char *field, char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_char;
	}

	return 0;
}

int dbi_bind_uchar(dbi_result_t *result, const char *field, unsigned char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_uchar;
	}

	return 0;
}

int dbi_bind_short(dbi_result_t *result, const char *field, short *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_short;
	}

	return 0;
}

int dbi_bind_ushort(dbi_result_t *result, const char *field, unsigned short *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_ushort;
	}

	return 0;
}

int dbi_bind_long(dbi_result_t *result, const char *field, long *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_long;
	}

	return 0;
}

int dbi_bind_ulong(dbi_result_t *result, const char *field, unsigned long *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_ulong;
	}

	return 0;
}

int dbi_bind_longlong(dbi_result_t *result, const char *field, long long *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_longlong;
	}

	return 0;
}

int dbi_bind_ulonglong(dbi_result_t *result, const char *field, unsigned long long *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_longlong;
	}

	return 0;
}


int dbi_bind_float(dbi_result_t *result, const char *field, float *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_float;
	}

	return 0;
}

int dbi_bind_double(dbi_result_t *result, const char *field, double *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_double;
	}

	return 0;
}

int dbi_bind_string(dbi_result_t *result, const char *field, const char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_string;
	}

	return 0;
}

int dbi_bind_binary(dbi_result_t *result, const char *field, const unsigned char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_binary;
	}

	return 0;
}


int dbi_bind_string_copy(dbi_result_t *result, const char *field, char **bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_string_copy;
	}

	return 0;
}

int dbi_bind_binary_copy(dbi_result_t *result, const char *field, unsigned char **bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_binary_copy;
	}

	return 0;
}

int dbi_bind_enum(dbi_result_t *result, const char *field, const char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_enum;
	}

	return 0;
}

int dbi_bind_set(dbi_result_t *result, const char *field, const char *bindto) {
	_field_binding_t *binding;

	if ((!result) || (!field)) return -1;

	binding = _find_or_create_binding_node(result, field);
	if (!binding) return -1;

	if (bindto == NULL) {
		/* kill the binding */
		_remove_binding_node(result, binding);
	}
	else {
		binding->bindto = bindto;
		binding->helper_function = _bind_helper_set;
	}

	return 0;
}

