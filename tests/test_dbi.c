#include <stdio.h>
#include <dbi/dbi.h>

int main() {
	dbi_plugin plugin = NULL;
	dbi_driver driver;
	dbi_result result;

	int curplugidx = 0;
	int numplugins = dbi_initialize(NULL);
	
	printf("\nLibrary version: %s\n", dbi_version());
	
	if (numplugins < 1) {
		if (numplugins == -1) printf("Couldn't open plugin directory.\n");
		printf("Unloading dbi, later...\n");
		dbi_shutdown();
		return 1;
	}
	
	while ((plugin = dbi_plugin_list(plugin))) {
		driver = dbi_driver_open(plugin);
		if (driver == NULL) {
			printf("Can't load driver 'template'...\n");
			dbi_shutdown();
			return 1;
		}
		
		printf("\n\tPLUGIN %d of %d\n\t---------------\n", curplugidx+1, numplugins);
		
		printf("\tName:       %s\n"
			   "\tFilename:   %s\n"
			   "\tDesc:       %s\n"
			   "\tMaintainer: %s\n"
			   "\tURL:        %s\n"
			   "\tVersion:    %s\n"
			   "\tCompiled:   %s\n", dbi_plugin_get_name(plugin), dbi_plugin_get_filename(plugin), dbi_plugin_get_description(plugin), dbi_plugin_get_maintainer(plugin), dbi_plugin_get_url(plugin), dbi_plugin_get_version(plugin), dbi_plugin_get_date_compiled(plugin));

		printf("\tCreating driver instance of plugin... ");
		driver = dbi_driver_open(plugin);
		
		if (driver == NULL) {
			printf("Failed.\n");
			dbi_shutdown();
			return 1;
		}
		printf("OK.\n");
		
		/*dbi_driver_set_option(driver, "host", "localhost");
		dbi_driver_set_option(driver, "username", "chug");
		dbi_driver_set_option(driver, "password", "dIP!");
		dbi_driver_set_option(driver, "database", "my_database");
		dbi_driver_set_option_numeric(driver, "efficient-queries", 0);

		printf("Options set, about to connect...\n");

		dbi_driver_connect(driver);
		printf("Connected, about to query...\n"); */
		
		dbi_driver_close(driver);
		curplugidx++;
		printf("\n");
	}
		
	printf("Shutting down DBI...\n");
	dbi_shutdown();
	printf("ja mata!\n");
	return 0;
}
