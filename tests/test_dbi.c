#include <stdio.h>
#include <dbi/dbi.h>

int main() {
	dbi_driver_t *driver;
	dbi_result_t *result;
	dbi_row_t *row;
	unsigned long FOO = 0;

	double threshold = 2.34;
	unsigned int id_number;
	unsigned int *p_int;
	char *fullname;

	int numplugins = dbi_initialize(NULL);
	printf("\nLoaded %d plugins... Library version: %s\n", numplugins, dbi_version());
	if (numplugins < 1) {
		if (numplugins == -1) printf("Couldn't open plugin directory.\n");
		printf("Unloading dbi, later...\n");
		dbi_shutdown();
		return 1;
	}
	driver = dbi_load_driver("template");
	if (driver == NULL) {
		printf("Can't load driver 'template'...\n");
		dbi_shutdown();
		return 1;
	}
	printf("\nLoaded plugin into 0x%X, instantiated into 0x%X:\n\tName:       %s\n\tFilename:   %s\n\tDesc:       %s\n\tMaintainer: %s\n\tURL:        %s\n\tVersion:    %s\n\tCompiled:   %s\n\n", driver->plugin, driver, dbi_plugin_name(driver->plugin), dbi_plugin_filename(driver->plugin), dbi_plugin_description(driver->plugin), dbi_plugin_maintainer(driver->plugin), dbi_plugin_url(driver->plugin), dbi_plugin_version(driver->plugin), dbi_plugin_date_compiled(driver->plugin));

	dbi_set_option(driver, "host", "localhost");
	dbi_set_option(driver, "username", "chug");
	dbi_set_option(driver, "password", "dIP!");
	dbi_set_option(driver, "database", "my_database");
	dbi_set_option_numeric(driver, "use_compression", 1);

	printf("Options set, about to connect...\n");

	dbi_connect(driver);
	printf("Connected, about to query...\n");
	result = NULL;
	result = dbi_query(driver, "SELECT id, name FROM fundip_chuggers WHERE tolerance > %0.2f", threshold);
	printf("Query done, result successful? %s\n", result?"Yes":"No");
	while (result && dbi_fetch_row(result)) {
		id_number = 123;
		p_int = &id_number;
		dbi_fetch_field(result, "id", (void **) &p_int);
		dbi_fetch_field(result, "name", (void **) &fullname);
		printf("\tResult found: Name=%s (ID=%d) is an official dipper!\n", fullname, id_number);
	}
	printf("\nDone fetching available rows...\n");
	if (result) dbi_free_query(result);
	printf("Freed query, closing driver.\n");
	dbi_close_driver(driver);
	printf("Driver closed and disconnected, shutting down DBI...\n");
	dbi_shutdown();
	printf("ja mata!\n");
	return 0;
}
