#include <stdio.h>
#include <dbi/dbi.h>

int main ()
{
	dbi_driver_t *driver=NULL;
	dbi_result_t *result=NULL;
	dbi_row_t *row=NULL;

	/* These are our data types, use for copying */
	short sshort;		short *p_sshort=&sshort;
	unsigned short ushort;	unsigned short *p_ushort=&ushort;
	long slong;		long *p_slong=&slong;
	double precision;	double *p_precision=&precision;
	char *string;		char **p_string=&string;


	int numplugins = dbi_initialize(NULL);

	if(numplugins < 1){
		if(numplugins == -1) fprintf(stderr, "Couldn't open plugin directory.\nTest failed.\n");
		else fprintf(stderr, "No Plugins Found.\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}


	driver = dbi_load_driver("mysql");

	if(driver == NULL){
		fprintf(stderr, "Can't load mysql driver.\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}

	dbi_set_option(driver, "host", "localhost");
	dbi_set_option(driver, "username", "test");
	dbi_set_option(driver, "password", "");
	dbi_set_option(driver, "database", "test");

	printf("Options Set! Connecting...");

	if( dbi_connect(driver) == -1 ){
		fprintf(stderr, "Connection failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	} /* else */ 

	printf("Connected!\nQuerying Server...");

	result = dbi_query(driver, "SELECT * FROM dbi");

	if(result == NULL){
		fprintf(stderr, "Query failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}

	printf("Got Result!\nProcessing %d Rows...\n",
		result->numrows);

	/* Each Row */
	while(dbi_fetch_row(result)) {
		if( dbi_fetch_field(result, "number", (void**) &p_slong) == -1 ){
			fprintf(stderr, "Field fetching failed.\n");
			break;
		}

		printf("Field 'number' Value [%d]\n",slong);
	}

	printf("Finished Processing Rows.\n");

	printf("Freeing result...");

	dbi_free_query(result);

	printf("done.\nClosing driver...");

	dbi_close_driver(driver);
	
	printf("done.\nShutting down DBI...");

	dbi_shutdown();

	printf("done.\n\nMySQL Test Has Been Successful!\n\nThank you for playing =P");

	return 0;
}

