#include <stdio.h>
#include <dbi/dbi.h>

int main ()
{
	dbi_driver_t *driver=NULL;
	dbi_result_t *result=NULL;
	dbi_row_t *row=NULL;

	/* These are our data types, use for copying */
	short sshort=-1;		short *p_sshort=&sshort;
	unsigned short ushort=-1;	unsigned short *p_ushort=&ushort;
	long slong=-1;			long *p_slong=&slong;
	double precision=-1;		double *p_precision=&precision;
	char *string;			char **p_string=&string;
	char query[1024] = "";


	int numplugins = dbi_initialize("/home/mmt/Projects/libdbi/locallib/lib/dbd");

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
	dbi_set_option(driver, "username", "");
	dbi_set_option(driver, "password", "");
	dbi_set_option(driver, "database", "test");

	fprintf(stderr, "Options Set! Connecting...");

	if( dbi_connect(driver) == -1 ){
		fprintf(stderr, "Connection failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	} /* else */ 

	fprintf(stderr, "Connected!\nQuery > ");

	fgets(query, 1023, stdin);

	fprintf(stderr, "Sending Query <%s>...", query);

	result = dbi_query(driver, query);

	if(result == NULL){
		fprintf(stderr, "Query failed!\nTest failed.\n");
		dbi_shutdown();
		return 1;
	}

	fprintf(stderr, "Got Result!\nProcessing %d Rows...\n",
		result->numrows_matched);

	/* Each Row */
	while(dbi_fetch_row(result)) {
		if( dbi_fetch_field(result, "_year", (void**) &p_sshort) == -1 ){
			fprintf(stderr, "Field fetching failed.\n");
			break;
		}

		fprintf(stderr, "Field 'number' Value [%d]\n", sshort);

		precision = -1;
	}

	fprintf(stderr, "Finished Processing Rows.\n");

	fprintf(stderr, "Freeing result...");

	dbi_free_query(result);

	fprintf(stderr, "done.\nClosing driver...");

	dbi_close_driver(driver);
	
	fprintf(stderr, "done.\nShutting down DBI...");

	dbi_shutdown();

	fprintf(stderr, "done.\n\nMySQL Test Has Been Successful!\n\nThank you for playing =P\n\n");

	return 0;
}

