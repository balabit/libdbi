dbi_driver_open(dbi_plugin Plugin) {
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

dbi_driver dbi_driver_new(const char *name) {
	dbi_plugin plugin;
	dbi_driver driver;

	plugin = dbi_plugin_open(name);
	driver = dbi_driver_open(plugin);

	return driver;
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
	driver->error_number = 0;
	if (driver->error_message) free(driver->error_message);
	driver->error_handler = NULL;
	driver->error_handler_argument = NULL;
	free(driver);
}

int dbi_driver_error(dbi_driver Driver, char *errmsg_dest) {
	dbi_driver_t *driver = Driver;
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
	if (retval == -1) {
		_error_handler(driver);
	}
	return retval;
}

dbi_result dbi_driver_get_db_list(dbi_driver Driver) {
	dbi_driver_t *driver = Driver;
	dbi_result_t *result;
	if (!driver) return NULL;
	result = driver->plugin->functions->list_dbs(driver);
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

int vasprintf(char **, const char *, va_list); /* to shut up gcc */

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
	
	/* at this point we have a dbi_result_t newly allocated by the plugin */
	result->driver = driver;
	result->field_bindings = NULL;
	result->numfields = 0;
	result->field_names = NULL;
	result->field_types = NULL;
	result->field_attribs = NULL;
	result->rows = NULL;
	result->currowidx = 0;
	
	return (dbi_result)result;
}

int dbi_driver_select_db(dbi_driver Driver, const char *db) {
	dbi_driver_t *driver = Driver;
	int retval;
	if (!driver) return -1;
	retval = driver->plugin->functions->select_db(driver, db);
	if (retval == -1) {
		_error_handler(driver);
	}
	return retval;
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
			((plugin->functions->initialize = dlsym(dlhandle, "dbd_initialize")) == NULL) || dlerror() ||
			((plugin->functions->connect = dlsym(dlhandle, "dbd_connect")) == NULL) || dlerror() ||
			((plugin->functions->disconnect = dlsym(dlhandle, "dbd_disconnect")) == NULL) || dlerror() ||
			((plugin->functions->fetch_row = dlsym(dlhandle, "dbd_fetch_row")) == NULL) || dlerror() ||
			((plugin->functions->free_query = dlsym(dlhandle, "dbd_free_query")) == NULL) || dlerror() ||
			((plugin->functions->get_custom_functions_list = dlsym(dlhandle, "dbd_get_custom_functions_list")) == NULL) || dlerror() ||
			((plugin->functions->get_info = dlsym(dlhandle, "dbd_get_info")) == NULL) || dlerror() ||
			((plugin->functions->get_reserved_words_list = dlsym(dlhandle, "dbd_get_reserved_words_list")) == NULL) || dlerror() ||
			((plugin->functions->goto_row = dlsym(dlhandle, "dbd_goto_row")) == NULL) || dlerror() ||
			((plugin->functions->list_dbs = dlsym(dlhandle, "dbd_list_dbs")) == NULL) || dlerror() ||
			((plugin->functions->list_tables = dlsym(dlhandle, "dbd_list_tables")) == NULL) || dlerror() ||
			((plugin->functions->query = dlsym(dlhandle, "dbd_query")) == NULL) || dlerror() ||
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
			prevdriver->next = curdriver->next;
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

