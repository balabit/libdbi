#include <stdio.h>
#include <dbi/dbi.h>

int main(int argc, char **argv) {
	dbi_driver driver = NULL;
	dbi_conn conn;
	dbi_result result;

	char *driverdir="/home/mmt/code/CVS/libdbi/local/lib/dbd";

	int sqlTAGID;
	char *sqlFILENAME;
	short sqlMEDIATYPEID;

	char *errmsg;
	int curplugidx = 0;
	int numdrivers;

	if(argc > 1){
		argv++;
		printf("Looking in: %s", *argv);
		dbi_initialize(*argv);
	} else {
		printf("Looking in: %s", driverdir);
		dbi_initialize(driverdir);
	}
	
	printf("\nLibrary version: %s\n", dbi_version());
	
	if (numdrivers < 1) {
		if (numdrivers == -1) printf("Couldn't open driver directory.\n");
		printf("Unloading dbi, later...\n");
		dbi_shutdown();
		return 1;
	}
	
		conn = dbi_conn_new("mysql");
		driver = dbi_conn_get_driver(conn);
		if (conn == NULL) {
			printf("Can't load conn 'mysql'...\n");
			dbi_shutdown();
			return 1;
		}
		
		printf("\n\tPLUGIN %d of %d\n\t---------------\n", curplugidx+1, numdrivers);
		
		printf("\tName:       %s\n"
			   "\tFilename:   %s\n"
			   "\tDesc:       %s\n"
			   "\tMaintainer: %s\n"
			   "\tURL:        %s\n"
			   "\tVersion:    %s\n"
			   "\tCompiled:   %s\n", dbi_driver_get_name(driver), dbi_driver_get_filename(driver), dbi_driver_get_description(driver), dbi_driver_get_maintainer(driver), dbi_driver_get_url(driver), dbi_driver_get_version(driver), dbi_driver_get_date_compiled(driver));

		printf("\tCreating conn instance of driver... ");
		conn = dbi_conn_open(driver);
		
		if (conn == NULL) {
			printf("Failed.\n");
			dbi_shutdown();
			return 1;
		}
		printf("OK.\n");
		
		//dbi_conn_set_option(conn, "host", "localhost");
		//dbi_conn_set_option_numeric(conn, "port", 12345);
		dbi_conn_set_option(conn, "username", "");
		dbi_conn_set_option(conn, "password", "");
		dbi_conn_set_option(conn, "dbname", "test");
		//dbi_conn_set_option_numeric(conn, "efficient-queries", 0);

		printf("Options set, about to connect...\n");

		if (dbi_conn_connect(conn) == -1) {
			dbi_conn_error(conn, &errmsg);
			printf("FAILED! Error message: %s\n", errmsg);
			free(errmsg);
			dbi_shutdown();
			return 1;
		}
		fprintf(stderr,"Connected to socket: %d, about to query... ",
				dbi_conn_get_socket(conn));
		while(1){}

		result = dbi_conn_query(conn, "SELECT * FROM sample");
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
			dbi_conn_error(conn, &errmsg);
			printf("FAILED! Error message: %s", errmsg);
			free(errmsg);
		}
		
		dbi_conn_close(conn);
		curplugidx++;
		printf("\n");
		
	printf("Shutting down DBI...\n");
	dbi_shutdown();
	printf("ja mata!\n");
	return 0;
}
