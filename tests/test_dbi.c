#include <stdio.h>
#include <dbi/dbi.h>

int main() {
	dbi_plugin plugin;
	dbi_driver driver;
	dbi_result result;

	double foo = 0.0;
	
	long id;
	const char *name;
	double weight;
	
	int numplugins = dbi_initialize(NULL);
	printf("\nLoaded %d plugins... Library version: %s\n", numplugins, dbi_version());
	
	if (numplugins < 1) {
		if (numplugins == -1) printf("Couldn't open plugin directory.\n");
		printf("Unloading dbi, later...\n");
		dbi_shutdown();
		return 1;
	}
	
	plugin = dbi_plugin_open("template");
	driver = dbi_driver_open(plugin);
	if (driver == NULL) {
		printf("Can't load driver 'template'...\n");
		dbi_shutdown();
		return 1;
	}
	
	printf("\n\tName:       %s\n\tFilename:   %s\n\tDesc:       %s\n\tMaintainer: %s\n\tURL:        %s\n\tVersion:    %s\n\tCompiled:   %s\n\n", dbi_plugin_get_name(plugin), dbi_plugin_get_filename(plugin), dbi_plugin_get_description(plugin), dbi_plugin_get_maintainer(plugin), dbi_plugin_get_url(plugin), dbi_plugin_get_version(plugin), dbi_plugin_get_date_compiled(plugin));

	dbi_driver_set_option(driver, "host", "localhost");
	dbi_driver_set_option(driver, "username", "chug");
	dbi_driver_set_option(driver, "password", "dIP!");
	dbi_driver_set_option(driver, "database", "my_database");
	dbi_driver_set_option_numeric(driver, "efficient-queries", 0);

	printf("Options set, about to connect...\n");

	dbi_driver_connect(driver);
	printf("Connected, about to query...\n");
	result = NULL;
	result = dbi_driver_query(driver, "SELECT id, name, weight FROM employees WHERE weight != %0.2f", foo);
	printf("Query done, result successful? %s\n", result?"Yes":"No");
	
	//dbi_result_bind_long(result, "id", &id);
	//dbi_result_bind_string(result, "name", &name);
	//dbi_result_bind_double(result, "weight", &weight);
	dbi_result_bind_fields(result, "weight.%d id.%l name.%s", &weight, &id, &name);
	
	while (result && dbi_result_next_row(result)) {
		//id = dbi_result_get_long(result, "id");
		//weight = dbi_result_get_double(result, "weight");
		//name = dbi_result_get_string("name");
		//dbi_result_get_fields(result, "weight.%d id.%l name.%s", &weight, &id, &name);
		printf("%i. %s\t\t\t%0.4f\n", id, name, weight);
	}
	printf("\nDone fetching available rows...\n");
	if (result) dbi_result_free(result);
	printf("Freed query, closing driver.\n");
	dbi_driver_close(driver);
	printf("Driver closed and disconnected, shutting down DBI...\n");
	dbi_shutdown();
	printf("ja mata!\n");
	return 0;
}
