#include <stdio.h>
#include <dbi/dbi.h>

int main(int argc, char **argv) {
	dbi_plugin plugin = NULL;
	dbi_driver driver;
	dbi_result result;

	char *plugindir="/home/mmt/code/CVS/libdbi/local/lib/dbd";

	int sqlTAGID;
	char *sqlFILENAME;
	short sqlMEDIATYPEID;

	char *errmsg;
	int curplugidx = 0;
	int numplugins;

	if(argc > 1){
		argv++;
		printf("Looking in: %s", *argv);
		dbi_initialize(*argv);
	} else {
		printf("Looking in: %s", plugindir);
		dbi_initialize(plugindir);
	}
	
	printf("\nLibrary version: %s\n", dbi_version());
	
	if (numplugins < 1) {
		if (numplugins == -1) printf("Couldn't open plugin directory.\n");
		printf("Unloading dbi, later...\n");
		dbi_shutdown();
		return 1;
	}
	
		driver = dbi_driver_new("mysql");
		plugin = dbi_driver_get_plugin(driver);
		if (driver == NULL) {
			printf("Can't load driver 'mysql'...\n");
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
		
		//dbi_driver_set_option(driver, "host", "localhost");
		//dbi_driver_set_option_numeric(driver, "port", 12345);
		dbi_driver_set_option(driver, "username", "");
		dbi_driver_set_option(driver, "password", "");
		dbi_driver_set_option(driver, "dbname", "test");
		//dbi_driver_set_option_numeric(driver, "efficient-queries", 0);

		printf("Options set, about to connect...\n");

		if (dbi_driver_connect(driver) == -1) {
			dbi_driver_error(driver, &errmsg);
			printf("FAILED! Error message: %s\n", errmsg);
			free(errmsg);
			dbi_shutdown();
			return 1;
		}
		printf("Connected, about to query... ");

		result = dbi_driver_query(driver, "SELECT * FROM sample");
		if (result) {
			printf("OK\n");
			dbi_result_bind_fields(result, "key1.%l enum1.%s", &sqlTAGID, &sqlFILENAME);
			printf("\nkey1\tstring1\tFilename\n");
			while (dbi_result_next_row(result)) {
				printf("%d %s\n", sqlTAGID, sqlFILENAME);
			}
			printf("Done fetching rows.\n");
			dbi_result_free(result);
		}
		else {
			dbi_driver_error(driver, &errmsg);
			printf("FAILED! Error message: %s", errmsg);
			free(errmsg);
		}
		
		dbi_driver_close(driver);
		curplugidx++;
		printf("\n");
		
	printf("Shutting down DBI...\n");
	dbi_shutdown();
	printf("ja mata!\n");
	return 0;
}
