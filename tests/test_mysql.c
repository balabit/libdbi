#include <stdio.h>
#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>

int main (int argc, char **argv)
{
	dbi_driver_t *driver=NULL;
	dbi_result_t *result=NULL;
	dbi_row_t *row=NULL;
	char *plugdir = "/home/mmt/libdbi/lib/lib/dbd";
	char *query;
	long example;

	int numplugins;

	
	numplugins = dbi_initialize(plugdir);

	if(numplugins < 1){
		if(numplugins == -1) fprintf(stderr, "Couldn't open plugin directory.\nTest failed.\n");
		else fprintf(stderr, "No Plugins Found.\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}


	driver = dbi_driver_open("mysql");

	if(driver == NULL){
		fprintf(stderr, "Can't load mysql driver.\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}

	dbi_driver_set_option(driver, "host", "localhost");
	dbi_driver_set_option(driver, "username", "");
	dbi_driver_set_option(driver, "password", "");
	dbi_driver_set_option(driver, "database", "test");

	fprintf(stderr, "Options Set! Connecting...");

	if( dbi_driver_connect(driver) == -1 ){
		fprintf(stderr, "Connection failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	} /* else */ 

	fprintf(stderr, "Connected!\nQuery > ");

	fgets(query, 1023, stdin);

	fprintf(stderr, "Sending Query <%s>...", query);

	result = dbi_driver_query(driver, query);

	if(result == NULL){
		fprintf(stderr, "Query failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}

	fprintf(stderr, "Got Result!\nProcessing %d Rows...\n",
		result->numrows_matched);

	/* Each Row */
	while(dbi_result_next_row(result)) {
		example = dbi_result_get_long(result, "id");
	}

	fprintf(stderr, "Finished Processing Rows.\n");

	fprintf(stderr, "Freeing result...");

	dbi_result_free(result);

	fprintf(stderr, "done.\nClosing driver...");

	dbi_driver_close(driver);
	
	fprintf(stderr, "done.\nShutting down DBI...");

	dbi_shutdown();

	fprintf(stderr, "done.\n\nMySQL Test Has Been Successful!\n\nThank you for playing =P\n\n");

	return 0;
}

