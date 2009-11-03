/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2003, David Parker and Mark Tobenkin.
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

/* Dlopen stuff */
#if HAVE_LTDL_H
#include <ltdl.h>
#define my_dlopen(foo,bar) lt_dlopen(foo)
#define my_dlsym lt_dlsym
#define my_dlclose lt_dlclose
#define my_dlerror lt_dlerror

#elif HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>
static void * dyld_dlopen(const char * file);
static void * dyld_dlsym(void * hand, const char * name);
static int dyld_dlclose(void * hand);
static const char * dyld_dlerror();
#define my_dlopen(foo,bar) dyld_dlopen(foo)
#define my_dlsym dyld_dlsym
#define my_dlerror dyld_dlerror
#define my_dlclose dyld_dlclose

#elif __MINGW32__
#include <windows.h>
void *win_dlopen(const char*, int);
void *win_dlsym(void *, const char*);
int win_dlclose(void *);
char *win_dlerror();
/* just for compiling support,if anyone has used these masks in code. The MODE argument to `dlopen' contains one of the following: */
#define RTLD_LAZY       0x001   /* Lazy function call binding.  */
#define RTLD_NOW        0x002   /* Immediate function call binding.  */
#define RTLD_BINDING_MASK 0x003   /* Mask of binding time value.  */
#define my_dlopen(foo,bar) win_dlopen(foo,bar)
#define my_dlsym win_dlsym
#define my_dlclose win_dlclose
#define my_dlerror win_dlerror

#elif HAVE_DLFCN_H
#include <dlfcn.h>
#define my_dlopen dlopen
#define my_dlsym dlsym
#define my_dlclose dlclose
#define my_dlerror dlerror

#else
#error no dynamic loading support
#endif
/* end dlopen stuff */

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <math.h>
#include <limits.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

#ifdef __MINGW32__
#  define DBI_PATH_SEPARATOR "\\"
#  ifndef DBI_DRIVER_DIR
#	define DBI_DRIVER_DIR "c:\\libdbi\\lib\\dbd" /* use this as the default */
#  endif
#  else
#    define DBI_PATH_SEPARATOR "/"
#    ifndef DBI_DRIVER_DIR
#    	define DBI_DRIVER_DIR "/usr/local/lib/dbd" /* use this as the default */
#    endif
#endif

#ifndef DLSYM_PREFIX
#define DLSYM_PREFIX ""
#endif

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1) /* taken from FreeBSD */
#endif

/* declarations of optional external functions */
#ifndef HAVE_VASPRINTF
int vasprintf(char **result, const char *format, va_list args);
#endif
#ifndef HAVE_ASPRINTF
int asprintf(char **result, const char *format, ...);
#endif

/* declarations for internal functions -- anything declared as static won't be accessible by name from client programs */
static dbi_driver_t *_get_driver(const char *filename, dbi_inst_t *inst);
static void _free_custom_functions(dbi_driver_t *driver);
static dbi_option_t *_find_or_create_option_node(dbi_conn Conn, const char *key);
static int _update_internal_conn_list(dbi_conn_t *conn, int operation);
static void _free_caps(_capability_t *caproot);
static const char *_get_option(dbi_conn Conn, const char *key, int aggressive);
static int _get_option_numeric(dbi_conn Conn, const char *key, int aggressive);
static unsigned int _parse_versioninfo(const char *version);
static int _safe_dlclose(dbi_driver_t *driver);

void _error_handler(dbi_conn_t *conn, dbi_error_flag errflag);
extern int _disjoin_from_conn(dbi_result_t *result);

dbi_result dbi_conn_queryf(dbi_conn Conn, const char *formatstr, ...) __attribute__ ((format (printf, 2, 3)));
int dbi_conn_set_error(dbi_conn Conn, int errnum, const char *formatstr, ...) __attribute__ ((format (printf, 3, 4)));

/* must not be called "ERROR" due to a name clash on Windoze */
static const char *my_ERROR = "ERROR";
static dbi_inst dbi_inst_legacy;

/* XXX DBI CORE FUNCTIONS XXX */

int dbi_initialize_r(const char *driverdir, dbi_inst *pInst) {
	dbi_inst_t *inst;
	DIR *dir;
	struct dirent *driver_dirent = NULL;
	struct stat statbuf;
	char fullpath[256];
	char *effective_driverdir;
	
	int num_loaded = 0;
	dbi_driver_t *driver = NULL;
	dbi_driver_t *prevdriver = NULL;
#if HAVE_LTDL_H
        (void)lt_dlinit();
#endif	

	*pInst = NULL; /* use a defined value if the function fails */
	/* init the instance */
	inst = malloc(sizeof(dbi_inst_t));
	if (!inst) {
		return -1;
	}
	*pInst = (void*) inst;
	inst->rootdriver = NULL;
	inst->rootconn = NULL;
	inst->dbi_verbosity = 1; /* TODO: is this really the right default? */
	/* end instance init */
	effective_driverdir = (driverdir ? (char *)driverdir : DBI_DRIVER_DIR);
	dir = opendir(effective_driverdir);

	if (dir == NULL) {
		return -1;
	}
	else {
		while ((driver_dirent = readdir(dir)) != NULL) {
			driver = NULL;
			snprintf(fullpath, sizeof(fullpath), "%s%s%s", effective_driverdir, DBI_PATH_SEPARATOR, driver_dirent->d_name);
			if ((stat(fullpath, &statbuf) == 0) && S_ISREG(statbuf.st_mode) && strrchr(driver_dirent->d_name, '.') && (!strcmp(strrchr(driver_dirent->d_name, '.'), DRIVER_EXT))) {
				/* file is a stat'able regular file that ends in .so (or appropriate dynamic library extension) */
				driver = _get_driver(fullpath, inst);
				if (driver && (driver->functions->initialize(driver) != -1)) {
					if (!inst->rootdriver) {
						inst->rootdriver = driver;
					}
					if (prevdriver) {
						prevdriver->next = driver;
					}
					prevdriver = driver;
					num_loaded++;
				}
				else {
					if (driver && driver->dlhandle) _safe_dlclose(driver);
					if (driver && driver->functions) free(driver->functions);
					if (driver) free(driver);
					driver = NULL; /* don't include in linked list */
					if (inst->dbi_verbosity) fprintf(stderr, "libdbi: Failed to load driver: %s\n", fullpath);
				}
			}
		}
		closedir(dir);
	}
	
	return num_loaded;
}

int dbi_initialize(const char *driverdir) {
	dbi_initialize_r(driverdir, &dbi_inst_legacy);
}

void dbi_shutdown_r(dbi_inst Inst) {
	dbi_inst_t *inst = (dbi_inst_t*) Inst;
	dbi_conn_t *curconn = inst->rootconn;
	dbi_conn_t *nextconn;
	
	dbi_driver_t *curdriver = inst->rootdriver;
	dbi_driver_t *nextdriver;
	
	while (curconn) {
		nextconn = curconn->next;
		dbi_conn_close((dbi_conn)curconn);
		curconn = nextconn;
	}
	
	while (curdriver) {
		nextdriver = curdriver->next;
 		_safe_dlclose(curdriver);
		free(curdriver->functions);
		_free_custom_functions(curdriver);
		_free_caps(curdriver->caps);
		free(curdriver->filename);
		free(curdriver);
		curdriver = nextdriver;
	}
#if HAVE_LTDL_H
        (void)lt_dlexit();
#endif	
	free(inst);
}

void dbi_shutdown() {
	dbi_shutdown_r(dbi_inst_legacy);
}

const char *dbi_version() {
	return "libdbi v" VERSION;
}

int dbi_set_verbosity_r(int verbosity, dbi_inst Inst) {
	dbi_inst_t *inst = (dbi_inst_t*) Inst;
	/* whether or not to spew stderr messages when something bad happens (and
	 * isn't handled through the normal connection-oriented DBI error
	 * mechanisms) */

	int prev = inst->dbi_verbosity;
	inst->dbi_verbosity = verbosity;
	return prev;
}
int dbi_set_verbosity(int verbosity) {
	return dbi_set_verbosity_r(verbosity, dbi_inst_legacy);
}

/* XXX DRIVER FUNCTIONS XXX */

dbi_driver dbi_driver_list_r(dbi_driver Current, dbi_inst Inst) {
	dbi_inst_t *inst = (dbi_inst_t*) Inst;
	dbi_driver_t *current = Current;

	if (current == NULL) {
		return (dbi_driver)inst->rootdriver;
	}

	return (dbi_driver)current->next;
}
dbi_driver dbi_driver_list(dbi_driver Current) {
	return dbi_driver_list_r(Current, dbi_inst_legacy);
}

dbi_driver dbi_driver_open_r(const char *name, dbi_inst Inst) {
	dbi_inst_t *inst = (dbi_inst_t*) Inst;
	dbi_driver_t *driver = inst->rootdriver;

	while (driver && strcasecmp(name, driver->info->name)) {
		driver = driver->next;
	}

	return driver;
}
dbi_driver dbi_driver_open(const char *name) {
	return dbi_driver_open_r(name, dbi_inst_legacy);
}

dbi_inst dbi_driver_get_instance(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return NULL;
	
	return driver->dbi_inst;
}

int dbi_driver_is_reserved_word(dbi_driver Driver, const char *word) {
	unsigned int idx = 0;
	dbi_driver_t *driver = Driver;
	
	if (!driver) return 0;
	
	while (driver->reserved_words[idx]) {
		if (strcasecmp(word, driver->reserved_words[idx]) == 0) {
			return 1;
		}
		idx++;
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

int dbi_driver_cap_get(dbi_driver Driver, const char *capname) {
	dbi_driver_t *driver = Driver;
	_capability_t *cap;

	if (!driver) {
	  return 0;
	}

	cap = driver->caps;

 	while (cap && strcmp(capname, cap->name)) {
	    cap = cap->next;
	}

	return cap ? cap->value : 0;
}

int dbi_conn_cap_get(dbi_conn Conn, const char *capname) {
	dbi_conn_t *conn = Conn;
	_capability_t *cap;

	if (!conn) return 0;

	cap = conn->caps;
	
	while (cap && strcmp(capname, cap->name)) {
		cap = cap->next;
	}

	return cap ? cap->value : dbi_driver_cap_get((dbi_driver)conn->driver, capname);
}

/* DRIVER: informational functions */

const char *dbi_driver_get_name(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;
	
	return driver->info->name;
}

const char *dbi_driver_get_filename(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;
	
	return driver->filename;
}

const char *dbi_driver_get_description(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;
	
	return driver->info->description;
}

const char *dbi_driver_get_maintainer(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;

	return driver->info->maintainer;
}

const char *dbi_driver_get_url(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;

	if (!driver) return my_ERROR;

	return driver->info->url;
}

const char *dbi_driver_get_version(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;
	
	return driver->info->version;
}

const char *dbi_driver_get_date_compiled(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	
	if (!driver) return my_ERROR;
	
	return driver->info->date_compiled;
}

/* DEPRECATED. Use dbi_conn_quote_string_copy instead */
size_t dbi_driver_quote_string_copy(dbi_driver Driver, const char *orig, char **newquoted) {
	dbi_driver_t *driver = Driver;
	char *newstr;
	size_t newlen;
	
	if (!driver || !orig || !newquoted) return 0;

	newstr = malloc((strlen(orig)*2)+4+1); /* worst case, we have to escape every character and add 2*2 surrounding quotes */

	if (!newstr) {
		return 0;
	}
	
	newlen = driver->functions->quote_string(driver, orig, newstr);
	if (!newlen) {
		free(newstr);
		return 0;
	}

	*newquoted = newstr;

	return newlen;
}

/* DEPRECATED. Use dbi_conn_quote_string instead */
size_t dbi_driver_quote_string(dbi_driver Driver, char **orig) {
	char *temp = NULL;
	char *newstr = NULL;
	size_t newlen;

	if (!orig || !*orig) {
		return 0;
	}
	
	newlen = dbi_driver_quote_string_copy(Driver, *orig, &newstr);
	if (!newlen) {
	  /* in case of an error, leave the original string alone */
	  return 0;
	}

	temp = *orig;
	*orig = newstr;
	free(temp); /* original unescaped string */

	return newlen;
}

const char* dbi_driver_encoding_from_iana(dbi_driver Driver, const char* iana_encoding) {
  dbi_driver_t *driver = Driver;
	
  if (!driver) {
    return NULL;
  }

  return driver->functions->encoding_from_iana(iana_encoding);
}

const char* dbi_driver_encoding_to_iana(dbi_driver Driver, const char* db_encoding) {
  dbi_driver_t *driver = Driver;
	
  if (!driver) {
    return NULL;
  }

  return driver->functions->encoding_to_iana(db_encoding);
}


/* XXX DRIVER FUNCTIONS XXX */

dbi_conn dbi_conn_new_r(const char *name, dbi_inst Inst) {
	dbi_driver driver;
	dbi_conn conn;

	driver = dbi_driver_open_r(name, Inst);
	conn = dbi_conn_open(driver);

	return conn;
}
dbi_conn dbi_conn_new(const char *name) {
	return dbi_conn_new_r(name, dbi_inst_legacy);
}

dbi_conn dbi_conn_open(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	dbi_conn_t *conn;
	
	if (!driver) {
		return NULL;
	}

	conn = malloc(sizeof(dbi_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->driver = driver;
	conn->options = NULL;
	conn->caps = NULL;
	conn->connection = NULL;
	conn->current_db = NULL;
	conn->error_flag = DBI_ERROR_NONE; /* for legacy code only */
	conn->error_number = DBI_ERROR_NONE;
	conn->error_message = NULL;
	conn->error_handler = NULL;
	conn->error_handler_argument = NULL;
	_update_internal_conn_list(conn, 1);
	conn->results = NULL;
	conn->results_size = conn->results_used = 0;

	return (dbi_conn)conn;
}

int dbi_conn_disjoin_results(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	int errors = 0;
	int idx;

	if (!conn) return 0;

	for (idx = conn->results_used-1; idx >= 0; idx--) {
		if (dbi_result_disjoin((dbi_result)conn->results[idx]) < 0) {
			errors--;
		}
	}

	return errors;
}

void dbi_conn_close(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	
	if (!conn) return;
	
	_update_internal_conn_list(conn, -1);
	
	conn->driver->functions->disconnect(conn);
	conn->driver = NULL;
	dbi_conn_clear_options(Conn);
	_free_caps(conn->caps);
	conn->connection = NULL;
	
	if (conn->current_db) free(conn->current_db);
	if (conn->error_message) free(conn->error_message);
	conn->error_number = 0;
	
	conn->error_handler = NULL;
	conn->error_handler_argument = NULL;
	free(conn->results);

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
	static char *errmsg = NULL; // XXX quick hack, revisit this when API is redesigned

	if (errmsg_dest) {
		if (errmsg) free(errmsg);
		
		if (conn->error_number) {
			snprintf(number_portion, 20, "%d: ", conn->error_number);
		}
		else {
			number_portion[0] = '\0';
		}

		asprintf(&errmsg, "%s%s", number_portion, conn->error_message ? conn->error_message : "");
		*errmsg_dest = errmsg;
	}

	return conn->error_number;
}

void dbi_conn_error_handler(dbi_conn Conn, dbi_conn_error_handler_func function, void *user_argument) {
	dbi_conn_t *conn = Conn;
	conn->error_handler = function;
	if (function == NULL) {
		conn->error_handler_argument = NULL;
	}
	else {
		conn->error_handler_argument = user_argument;
	}
}

/* DEPRECATED */
dbi_error_flag dbi_conn_error_flag(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	return conn->error_flag;
}

/* this function allows applications to set their own error messages
   and display them through the libdbi API */
int dbi_conn_set_error(dbi_conn Conn, int errnum, const char *formatstr, ...) {
	dbi_conn_t *conn = Conn;
	char *msg;
	int len;
	va_list ap;
	int trigger_callback;

	if (!conn) return 0;

	trigger_callback = dbi_conn_get_option_numeric(Conn, "UserErrorTriggersCallback");

	va_start(ap, formatstr);
	len = vasprintf(&msg, formatstr, ap);
	va_end(ap);

	if (conn->error_message) free(conn->error_message);
	conn->error_message = msg;
	conn->error_number = errnum;
	conn->error_flag = DBI_ERROR_USER;

	if (trigger_callback && (conn->error_handler != NULL)) {
		/* trigger the external callback function */
		conn->error_handler((dbi_conn)conn, conn->error_handler_argument);
	}
	
	return len;
}

/* CONN: quoting functions. These functions escape SQL special
   characters and surround the resulting string with appropriate
   quotes to insert it into a SQL query string */

size_t dbi_conn_quote_string_copy(dbi_conn Conn, const char *orig, char **newquoted) {
	dbi_conn_t *conn = Conn;
	char *newstr;
	size_t newlen;
	
	if (!conn) {
	  return 0;
	}

	_reset_conn_error(conn);

	if (!orig || !newquoted) {
	  _error_handler(conn, DBI_ERROR_BADPTR);
	  return 0;
	}

	newstr = malloc((strlen(orig)*2)+4+1); /* worst case, we have to escape every character and add 2*2 surrounding quotes */

	if (!newstr) {
	  _error_handler(conn, DBI_ERROR_NOMEM);
	  return 0;
	}
	
	newlen = conn->driver->functions->conn_quote_string(conn, orig, newstr);
	if (!newlen) {
	  free(newstr);
	  _error_handler(conn, DBI_ERROR_NOMEM);
	  return 0;
	}

	*newquoted = newstr;

	return newlen;
}

size_t dbi_conn_quote_string(dbi_conn Conn, char **orig) {
	dbi_conn_t *conn = Conn;
	char *temp = NULL;
	char *newstr = NULL;
	size_t newlen;

	if (!conn) {
	  return 0;
	}

	_reset_conn_error(conn);

	if (!orig || !*orig) {
	  _error_handler(conn, DBI_ERROR_BADPTR);
	  return 0;
	}
	
	newlen = dbi_conn_quote_string_copy(Conn, *orig, &newstr);
	if (!newlen) {
	  /* leave original string alone in case of an error */
	  /* error number was set by called function */
	  return 0;
	}
	temp = *orig;
	*orig = newstr;
	free(temp); /* original unescaped string */

	return newlen;
}

size_t dbi_conn_quote_binary_copy(dbi_conn Conn, const unsigned char *orig, size_t from_length, unsigned char **ptr_dest) {
  unsigned char *temp = NULL;
  size_t newlen;
  dbi_conn_t *conn = Conn;

  if (!conn) {
    return 0;
  }

  _reset_conn_error(conn);

  if (!orig || !ptr_dest) {
    _error_handler(conn, DBI_ERROR_BADPTR);
    return 0;
  }

  newlen = conn->driver->functions->quote_binary(conn, orig, from_length, &temp);
  if (!newlen) {
    _error_handler(conn, DBI_ERROR_NOMEM);
    return 0;
  }

  *ptr_dest = temp;

  return newlen;
}

/* CONN: escaping functions. These functions escape SQL special
   characters but do not add the quotes like the quoting functions
   do */

size_t dbi_conn_escape_string_copy(dbi_conn Conn, const char *orig, char **newquoted) {
	char *newstr;
	size_t newlen;
	
	if (!Conn) {
	  return 0;
	}

	newlen = dbi_conn_quote_string_copy(Conn, orig, newquoted);

	if (newlen) {
	  (*newquoted)[newlen-1] = '\0';
	  memmove(*newquoted, (*newquoted)+1, newlen-1);
	}

	return newlen-2;
}

size_t dbi_conn_escape_string(dbi_conn Conn, char **orig) {
	size_t newlen;

	newlen = dbi_conn_quote_string(Conn, orig);

	if (newlen) {
	  (*orig)[newlen-1] = '\0';
	  memmove(*orig, (*orig)+1, newlen-1);
	}
	return newlen-2;
}

size_t dbi_conn_escape_binary_copy(dbi_conn Conn, const unsigned char *orig, size_t from_length, unsigned char **ptr_dest) {
	size_t newlen;

	newlen = dbi_conn_quote_binary_copy(Conn, orig, from_length, ptr_dest);

	if (newlen) {
	  (*ptr_dest)[newlen-1] = '\0';
	  memmove(*ptr_dest, (*ptr_dest)+1, newlen-1);
	}

	return newlen-2;
}

/* CONN: option manipulation */

int dbi_conn_set_option(dbi_conn Conn, const char *key, const char *value) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (!conn) {
		return -1;
	}
	
	_reset_conn_error(conn);

	option = _find_or_create_option_node(conn, key);
	if (!option) {
		_error_handler(conn, DBI_ERROR_NOMEM);
		return -1;
	}

	if (option->string_value) free(option->string_value);
	option->string_value = (value) ? strdup(value) : NULL;
	option->numeric_value = 0;
	
	return 0;
}

int dbi_conn_set_option_numeric(dbi_conn Conn, const char *key, int value) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (!conn) {
		return -1;
	}
	
	_reset_conn_error(conn);

	option = _find_or_create_option_node(conn, key);
	if (!option) {
		_error_handler(conn, DBI_ERROR_NOMEM);
		return -1;
	}
	
	if (option->string_value) free(option->string_value);
	option->string_value = NULL;
	option->numeric_value = value;
	
	return 0;
}

static const char *_get_option(dbi_conn Conn, const char *key, int aggressive) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;

	if (!conn) {
		return NULL;
	}
	
	_reset_conn_error(conn);

	option = conn->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	if (option) {
		return option->string_value;
	}
	else {
		if (aggressive) _error_handler(conn, DBI_ERROR_BADNAME);
		return NULL;
	}
}

const char *dbi_conn_get_option(dbi_conn Conn, const char *key) {
	return _get_option(Conn, key, 0);
}

const char *dbi_conn_require_option(dbi_conn Conn, const char *key) {
	return _get_option(Conn, key, 1);
}

static int _get_option_numeric(dbi_conn Conn, const char *key, int aggressive) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;

	if (!conn) return 0;
	
	_reset_conn_error(conn);

	option = conn->options;
	while (option && strcasecmp(key, option->key)) {
		option = option->next;
	}

	if (option) {
		return option->numeric_value;
	}
	else {
		if (aggressive) _error_handler(conn, DBI_ERROR_BADNAME);
		return 0;
	}
}

int dbi_conn_get_option_numeric(dbi_conn Conn, const char *key) {
	return _get_option_numeric(Conn, key, 0);
}

int dbi_conn_require_option_numeric(dbi_conn Conn, const char *key) {
	return _get_option_numeric(Conn, key, 1);
}

const char *dbi_conn_get_option_list(dbi_conn Conn, const char *current) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *option;
	
	if (!conn) {
	  return NULL;
	}

	_reset_conn_error(conn);

	if (!conn->options) {
	  _error_handler(conn, DBI_ERROR_BADPTR);
	  return NULL;
	}
	option = conn->options;
	
	if (!current) {
		return option->key;
	}
	else {
		while (option && strcasecmp(current, option->key)) {
			option = option->next;
		}
		/* return NULL if there are no more options but don't make
		   this an error */
		return (option && option->next) ? option->next->key : NULL;
	}
}

void dbi_conn_clear_option(dbi_conn Conn, const char *key) {
	dbi_conn_t *conn = Conn;
	dbi_option_t *prevoption = NULL; /* shut up compiler */
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
	
	_reset_conn_error(conn);

	retval = conn->driver->functions->connect(conn);
	if (retval == -1) {
		/* couldn't create a connection and no DBD-level error information is available */
		_error_handler(conn, DBI_ERROR_NOCONN);
	}
/* 	else if (retval == -2) { */
		/* a DBD-level error has already been set and the callback has already triggered */
/* 	} */

	return retval;
}

int dbi_conn_get_socket(dbi_conn Conn){
	dbi_conn_t *conn = Conn;
	int retval;

	if (!conn) {
	  return -1;
	}

	_reset_conn_error(conn);

	retval = conn->driver->functions->get_socket(conn);

	return retval;
}

const char *dbi_conn_get_encoding(dbi_conn Conn){
	dbi_conn_t *conn = Conn;
	const char *retval;

	if (!conn) return NULL;

	_reset_conn_error(conn);

	retval = conn->driver->functions->get_encoding(conn);

	return retval;
}

unsigned int dbi_conn_get_engine_version(dbi_conn Conn){
	dbi_conn_t *conn = Conn;
	char versionstring[VERSIONSTRING_LENGTH];

	if (!conn) return 0;

	_reset_conn_error(conn);

	conn->driver->functions->get_engine_version(conn, versionstring);

	return _parse_versioninfo(versionstring);
}

char* dbi_conn_get_engine_version_string(dbi_conn Conn, char *versionstring) {
	dbi_conn_t *conn = Conn;

	if (!conn) return 0;

	_reset_conn_error(conn);

	return conn->driver->functions->get_engine_version(conn, versionstring);
}

dbi_result dbi_conn_get_db_list(dbi_conn Conn, const char *pattern) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;
	
	if (!conn) return NULL;
	
	_reset_conn_error(conn);

	result = conn->driver->functions->list_dbs(conn, pattern);
	
	if (result == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
	}

	return (dbi_result)result;
}

dbi_result dbi_conn_get_table_list(dbi_conn Conn, const char *db, const char *pattern) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;
	
	if (!conn) return NULL;
	
	_reset_conn_error(conn);

	result = conn->driver->functions->list_tables(conn, db, pattern);
	
	if (result == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
	}
	
	return (dbi_result)result;
}

dbi_result dbi_conn_query(dbi_conn Conn, const char *statement) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;

	if (!conn) return NULL;
	
	_reset_conn_error(conn);

	_logquery(conn, "[query] %s\n", statement);
	result = conn->driver->functions->query(conn, statement);

	if (result == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
	}

	return (dbi_result)result;
}

dbi_result dbi_conn_queryf(dbi_conn Conn, const char *formatstr, ...) {
	dbi_conn_t *conn = Conn;
	char *statement;
	dbi_result_t *result;
	va_list ap;

	if (!conn) return NULL;
	
	_reset_conn_error(conn);

	va_start(ap, formatstr);
	vasprintf(&statement, formatstr, ap);
	va_end(ap);
	
	_logquery(conn, "[queryf] %s\n", statement);
	result = conn->driver->functions->query(conn, statement);

	if (result == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
	}
	free(statement);
	
	return (dbi_result)result;
}

dbi_result dbi_conn_query_null(dbi_conn Conn, const unsigned char *statement, size_t st_length) {
	dbi_conn_t *conn = Conn;
	dbi_result_t *result;

	if (!conn) return NULL;

	_reset_conn_error(conn);

	_logquery_null(conn, statement, st_length);
	result = conn->driver->functions->query_null(conn, statement, st_length);

	if (result == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
	}
	
	return (dbi_result)result;
}

int dbi_conn_select_db(dbi_conn Conn, const char *db) {
	dbi_conn_t *conn = Conn;
	const char *retval;
	
	if (!conn) return -1;
	
	_reset_conn_error(conn);

	if (conn->current_db) free(conn->current_db);
	conn->current_db = NULL;
	
	retval = conn->driver->functions->select_db(conn, db);
	
	if (retval == NULL) {
		_error_handler(conn, DBI_ERROR_DBD);
		return -1;
	}
	
	if (*retval == '\0') {
		/* if "" was returned, conn doesn't support switching databases */
		_error_handler(conn, DBI_ERROR_UNSUPPORTED);
		return -1;
	}
	else {
		conn->current_db = strdup(retval);
	}
	
	return 0;
}

unsigned long long dbi_conn_sequence_last(dbi_conn Conn, const char *name) {
	dbi_conn_t *conn = Conn;
	unsigned long long result;
	if (!conn) return 0;

	_reset_conn_error(conn);

	result = conn->driver->functions->get_seq_last(conn, name);
	return result;
}

unsigned long long dbi_conn_sequence_next(dbi_conn Conn, const char *name) {
	dbi_conn_t *conn = Conn;
	unsigned long long result;
	if (!conn) return 0;

	_reset_conn_error(conn);

	result = conn->driver->functions->get_seq_next(conn, name);
	return result;
}

int dbi_conn_ping(dbi_conn Conn) {
	dbi_conn_t *conn = Conn;
	int result;

	if (!conn) return 0;

	_reset_conn_error(conn);

	result = conn->driver->functions->ping(conn);
	return result;
}

/* XXX INTERNAL PRIVATE IMPLEMENTATION FUNCTIONS XXX */

static dbi_driver_t *_get_driver(const char *filename, dbi_inst_t *inst) {
	dbi_driver_t *driver;
	void *dlhandle;
	void *symhandle;
	const char **custom_functions_list;
	unsigned int idx = 0;
	dbi_custom_function_t *prevcustom = NULL;
	dbi_custom_function_t *custom = NULL;
	char function_name[256];

	dlhandle = my_dlopen(filename, DLOPEN_FLAG); /* DLOPEN_FLAG defined by autoconf */

	if (dlhandle == NULL) {
	  fprintf(stderr, "%s\n", my_dlerror());
		return NULL;
	}
	else {
		driver = malloc(sizeof(dbi_driver_t));
		if (!driver) return NULL;

		driver->dlhandle = dlhandle;
		driver->filename = strdup(filename);
		driver->dbi_inst = inst;
		driver->next = NULL;
		driver->caps = NULL;
		driver->functions = malloc(sizeof(dbi_functions_t));

		if ( /* nasty looking if block... is there a better way to do it? */
			((driver->functions->register_driver = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_register_driver")) == NULL) ||
			((driver->functions->initialize = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_initialize")) == NULL) ||
			((driver->functions->connect = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_connect")) == NULL) ||
			((driver->functions->disconnect = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_disconnect")) == NULL) ||
			((driver->functions->fetch_row = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_fetch_row")) == NULL) ||
			((driver->functions->free_query = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_free_query")) == NULL) ||
			((driver->functions->goto_row = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_goto_row")) == NULL) ||
			((driver->functions->get_socket = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_get_socket")) == NULL) ||
			((driver->functions->get_encoding = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_get_encoding")) == NULL) ||
			((driver->functions->encoding_from_iana = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_encoding_from_iana")) == NULL) ||
			((driver->functions->encoding_to_iana = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_encoding_to_iana")) == NULL) ||
			((driver->functions->get_engine_version = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_get_engine_version")) == NULL) ||
			((driver->functions->list_dbs = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_list_dbs")) == NULL) ||
			((driver->functions->list_tables = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_list_tables")) == NULL) ||
			((driver->functions->query = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_query")) == NULL) ||
			((driver->functions->query_null = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_query_null")) == NULL) ||
			((driver->functions->quote_string = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_quote_string")) == NULL) ||
			((driver->functions->quote_binary = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_quote_binary")) == NULL) ||
			((driver->functions->conn_quote_string = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_conn_quote_string")) == NULL) ||
			((driver->functions->select_db = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_select_db")) == NULL) ||
			((driver->functions->geterror = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_geterror")) == NULL) ||
			((driver->functions->get_seq_last = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_get_seq_last")) == NULL) ||
			((driver->functions->get_seq_next = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_get_seq_next")) == NULL) ||
			((driver->functions->ping = my_dlsym(dlhandle, DLSYM_PREFIX "dbd_ping")) == NULL)
			)
		{
			free(driver->functions);
			free(driver->filename);
			free(driver);
			return NULL;
		}
		driver->functions->register_driver(&driver->info, &custom_functions_list, &driver->reserved_words);
		driver->custom_functions = NULL; /* in case no custom functions are available */

		/* this is a weird hack for the sake of dlsym
		   portability. I can't imagine why using dlhandle
		   fails on FreeBSD except that dlsym may expect a
		   leading underscore in front of the function
		   names. But then, why does RTLD_NEXT work? */
		if (DLSYM_HANDLE) { /* most OSes */
		  symhandle = dlhandle;
		}
		else { /* the BSDs */
		  symhandle = RTLD_NEXT;
		}

		while (custom_functions_list && custom_functions_list[idx] != NULL) {
			custom = malloc(sizeof(dbi_custom_function_t));
			if (!custom) {
				_free_custom_functions(driver);
				free(driver->functions);
				free(driver->filename);
				free(driver);
				return NULL;
			}
			custom->next = NULL;
			custom->name = custom_functions_list[idx];
/* 			snprintf(function_name, 256, DLSYM_PREFIX "dbd_%s", custom->name); */
/* 			printf("loading %s<<\n", custom->name); */

			custom->function_pointer = my_dlsym(symhandle, custom->name);
			if (!custom->function_pointer) {
/* 			  printf(my_dlerror()); */

			  /* this usually fails because a function was
			     renamed, is no longer available, or not
			     yet available. Simply skip this
			     function */
			  free(custom); /* not linked into the list yet */
			  idx++;
			  continue;
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

static void _free_caps(_capability_t *caproot) {
	_capability_t *cap = caproot;
	while (cap) {
		_capability_t *nextcap = cap->next;
		if (cap->name) free(cap->name);
		free(cap);
		cap = nextcap;
	}
	return;
}

static int _update_internal_conn_list(dbi_conn_t *conn, const int operation) {
	/* maintain internal linked list of conns so that we can unload them all
	 * when dbi is shutdown
	 * 
	 * operation = -1: remove conn
	 *           =  0: just look for conn (return 1 if found, -1 if not)
	 *           =  1: add conn */
	dbi_conn_t *curconn = conn->driver->dbi_inst->rootconn;
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
			else conn->driver->dbi_inst->rootconn = NULL;
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
			conn->driver->dbi_inst->rootconn = conn;
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
		option = malloc(sizeof(dbi_option_t));
		if (!option) return NULL;
		option->next = NULL;
		option->key = strdup(key);
		option->string_value = NULL;
		if (conn->options == NULL) {
		    conn->options = option;
		}
		else {
		    prevoption->next = option;
		}
	}

	return option;
}

#define COUNTOF(array) (sizeof(array)/sizeof((array)[0]))

/* sets conn->error_number and conn->error_message values */
void _error_handler(dbi_conn_t *conn, dbi_error_flag errflag) {
	int my_errno = 0;
	char *errmsg = NULL;
	int errstatus;
	static const char *errflag_messages[] = {
		/* DBI_ERROR_USER */		NULL,
		/* DBI_ERROR_DBD */			NULL,
		/* DBI_ERROR_BADOBJECT */	"An invalid or NULL object was passed to libdbi",
		/* DBI_ERROR_BADTYPE */		"The requested variable type does not match what libdbi thinks it should be",
		/* DBI_ERROR_BADIDX */		"An invalid or out-of-range index was passed to libdbi",
		/* DBI_ERROR_BADNAME */		"An invalid name was passed to libdbi",
		/* DBI_ERROR_UNSUPPORTED */	"This particular libdbi driver or connection does not support this feature",
		/* DBI_ERROR_NOCONN */		"libdbi could not establish a connection",
		/* DBI_ERROR_NOMEM */		"libdbi ran out of memory",
		/* DBI_ERROR_BADPTR */		"An invalid pointer was passed to libdbi",
		/* DBI_ERROR_NONE */		NULL,
		/* DBI_ERROR_CLIENT */          NULL
};
	
	if (conn == NULL) {
		/* temp hack... if a result is disjoined and encounters an error, conn
		 * will be null when we get here. just ignore it, since we assume
		 * errors require a valid conn. this shouldn't even be a problem now,
		 * since (currently) the only reason a result would be disjoint is if a
		 * garbage collector was about to get rid of it. */
		// conn will also be NULL if the connection itself could not be created,
		// or if a NULL result handle was passed to a dbi routine, so we would
		// always want to return if NULL. Probably should say something though:
		fprintf(stderr, "libdbi: _error_handler: %s (NULL conn/result handle)\n",
				errflag >= DBI_ERROR_BADOBJECT-1 && errflag < COUNTOF(errflag_messages)-1
					? errflag_messages[errflag+1] : "");
		return;
	}

	if (errflag == DBI_ERROR_DBD) {
		errstatus = conn->driver->functions->geterror(conn, &my_errno, &errmsg);

		if (errstatus == -1) {
			/* not _really_ an error. XXX debug this, does it ever actually happen? */
			return;
		}
	}
	else {
	  my_errno = errflag;
	}

	if (conn->error_message) free(conn->error_message);

	if ((errflag-DBI_ERROR_USER) >= 0 && (errflag-DBI_ERROR_USER) < COUNTOF(errflag_messages) 
			&& errflag_messages[(errflag-DBI_ERROR_USER)] != NULL) {
		errmsg = strdup(errflag_messages[(errflag-DBI_ERROR_USER)]);
	}
	
	conn->error_flag = errflag; /* should always be DBI_ERROR_DBD,
				       DBI_ERROR_UNSUPPORTED,
				       DBI_ERROR_NOCONN,
				       DBI_ERROR_BADNAME, or
				       DBI_ERROR_NOMEM for conn
				       errors */
	conn->error_number = my_errno;
	conn->error_message = errmsg;
	
	if (conn->error_handler != NULL) {
		/* trigger the external callback function */
		conn->error_handler((dbi_conn)conn, conn->error_handler_argument);
	}
}

/* this function should be called by all functions that may alter the
   connection error status*/
void _reset_conn_error(dbi_conn_t *conn) {
  if (conn) {
    conn->error_flag = DBI_ERROR_NONE;
    conn->error_number = DBI_ERROR_NONE;
    if (conn->error_message) {
      free(conn->error_message);
      conn->error_message = NULL;
    }
  }
}

void _verbose_handler(dbi_conn_t *conn, const char* fmt, ...) {
	va_list ap;

	if(conn && dbi_conn_get_option_numeric(conn, "Verbosity"))
	{
	  fputs("libdbi: ",stderr);
	  va_start(ap, fmt);
	  vfprintf(stderr, fmt, ap);
	  va_end(ap);
	}
}

void _logquery(dbi_conn_t *conn, const char* fmt, ...) {
	va_list ap;

	if(conn && dbi_conn_get_option_numeric(conn, "LogQueries")){
	  fputs("libdbi: ", stderr);
	  va_start(ap, fmt);
	  vfprintf(stderr, fmt, ap);
	  va_end(ap);
	}
}

void _logquery_null(dbi_conn_t *conn, const char* statement, size_t st_length) {
	if(conn && dbi_conn_get_option_numeric(conn, "LogQueries")){
	  fputs("libdbi: [query_null] ", stderr);
	  fwrite(statement, 1, st_length, stderr);
	  fputc('\n', stderr);
	}
}

unsigned int _isolate_attrib(unsigned int attribs, unsigned int rangemin, unsigned int rangemax) {
	/* hahaha! who woulda ever thunk strawberry's code would come in handy? */
	// find first (highest) bit set; methods not using FP can be found at 
	// http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
	unsigned startbit = log(rangemin)/log(2);
	unsigned endbit = log(rangemax)/log(2);
	// construct mask from startbit to endbit inclusive
	unsigned attrib_mask = ((1<<(endbit+1))-1) ^ ((1<<startbit)-1);
	return attribs & attrib_mask;
}

/* computes an int from a [[[[A.]B.]C.]D.]E version number according to this
  formula: version = E + D*100 + C*10000 + B*1000000 + A*100000000 */
static unsigned int _parse_versioninfo(const char *version) {
  char my_version[VERSIONSTRING_LENGTH];
  char* start;
  char* dot;
  int i = 0;
  unsigned int n_version = 0;
  unsigned int n_multiplier = 1;

  if (!version || !*version) {
    return 0;
  }

  /* get a copy of version to mangle */
  strncpy(my_version, version, VERSIONSTRING_LENGTH-1);
  my_version[VERSIONSTRING_LENGTH-1] = '\0';

  /* remove trailing periods */
  if (my_version[strlen(my_version)-1] == '.') {
    my_version[strlen(my_version)-1] = '\0';
  }

  start = my_version;
  while ((dot = strrchr(start, (int)'.')) != NULL && i<5) {
    n_version += atoi(dot+1) * n_multiplier;
    *dot = '\0';
    n_multiplier *= 100;
    i++;
  }
  
  /* take care of the remainder */
  n_version += atoi(start) * n_multiplier;

  if (i==5) {
    /* version string can't be encoded in an unsigned int */
    return 0;
  }
  return n_version;
}

/* this is to work around nasty database client libraries which
   install exit handlers although they're not supposed to. If we
   unload drivers linked against one of these libraries, the
   application linked against libdbi will crash on exit if running on
   FreeBSD and other OSes sticking to the rule. Drivers which can be
   safely dlclose()d should set the driver capability "safe_dlclose"
   to nonzero */
/* returns 0 if the driver was closed, 1 if not */
static int _safe_dlclose(dbi_driver_t *driver) {
  int may_close = 0;

  may_close = dbi_driver_cap_get((dbi_driver)driver, "safe_dlclose");
  if (may_close) {
    my_dlclose(driver->dlhandle);
    return 0;
  }
  return 1;
}

#if HAVE_MACH_O_DYLD_H
static int dyld_error_set=0;
static void * dyld_dlopen(const char * file)
{
	NSObjectFileImage o=NULL;
	NSObjectFileImageReturnCode r;
	NSModule m=NULL;
	const unsigned int flags =  NSLINKMODULE_OPTION_RETURN_ON_ERROR | NSLINKMODULE_OPTION_PRIVATE;
	dyld_error_set=0;
	r = NSCreateObjectFileImageFromFile(file,&o);
	if (NSObjectFileImageSuccess == r)
	{
		m=NSLinkModule(o,file,flags);
		NSDestroyObjectFileImage(o);
		if (!m) dyld_error_set=1;
	}
        return (void*)m;
}
static void * dyld_dlsym(void * hand, const char * name)
{
        NSSymbol sym=NSLookupSymbolInModule((NSModule)hand, name);
	void * addr = NULL;
	dyld_error_set=0;
	if (sym) addr=(void*)NSAddressOfSymbol(sym);
	if (!addr) dyld_error_set=1;
        return addr;
}
static int dyld_dlclose(void * hand)
{
        int retVal=0;
	dyld_error_set=0;
        if (!NSUnLinkModule((NSModule)hand, 0))
	{
	        dyld_error_set=1;
		retVal=-1;
	}
        return retVal;
}

static const char * dyld_dlerror()
{
	NSLinkEditErrors ler;
	int lerno;
	const char *errstr;
	const char *file;
	NSLinkEditError(&ler, &lerno, &file, &errstr);
	if (!dyld_error_set) errstr=NULL;
	dyld_error_set=0;
	return errstr;
}
#endif /* HAVE_MACH_O_DYLD_H */

#ifdef __MINGW32__
static char win_errstr[512];
static int win_err_set = 0;
void *win_dlopen(const char *filename, int mode)
{
  HINSTANCE handle;
  handle = LoadLibrary(filename);
#ifdef _WIN32
  if (! handle) {
    sprintf(win_errstr, "Error code 1(%d) while loading library %s", GetLastError(), filename);
	win_err_set =1;
    return NULL;
  }
#else
  if ((UINT) handle < 32) {
    sprintf(win_errstr, "error code 1(%d) while loading library %s", (UINT) handle, filename);
	win_err_set =1;
    return NULL;
  }
#endif
  return (void *) handle;
}

void *win_dlsym(void *handle1, const char *symname)
{
  HMODULE handle = (HMODULE) handle1;
  void *symaddr;
  symaddr = (void *) GetProcAddress(handle, symname);
  if (symaddr == NULL)
  {
    sprintf(win_errstr, "can not find symbol %s", symname);
	win_err_set =2;
  }
  return symaddr;
}

int win_dlclose(void *handle1)
{
  HMODULE handle = (HMODULE) handle1;
#ifdef _WIN32
  if (FreeLibrary(handle))
    return 0;
  else {
    sprintf(win_errstr, "error code 3(%d) while closing library", GetLastError());
	win_err_set =3;
    return -1;
  }
#else
  FreeLibrary(handle);
  return 0;
#endif
}

char *win_dlerror()
{
  return win_errstr;
}

#endif
