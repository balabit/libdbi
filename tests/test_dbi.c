#include <stdio.h>
#include <dbi/dbi.h>

int main(int argc, char **argv) {
	dbi_driver driver;
	dbi_conn conn;
	dbi_result result;

	char driverdir[256];
	char drivername[64];
	char hostname[64];
	char username[64];
	char password[64];
	char dbname[64];

	char *errmsg;
	int numdrivers;

	printf("\nlibdbi test program: $Id$\nLibrary version: %s\n\n", dbi_version());
	
	printf("libdbi driver directory? [%s] ", DBI_DRIVER_DIR);
	fgets(driverdir, 256, stdin);
	if (driverdir[0] == '\n') strncpy(driverdir, DBI_DRIVER_DIR, 255), driverdir[255] = '\0';
	else driverdir[strlen(driverdir)-1] = '\0';
	
	numdrivers = dbi_initialize(driverdir);
	
	if (numdrivers < 0) {
		printf("Unable to initialize libdbi! Make sure you specified a valid driver directory.\n");
		dbi_shutdown();
		return 1;
	}
	else if (numdrivers == 0) {
		printf("Initialized libdbi, but no drivers were found!\n");
		dbi_shutdown();
		return 1;
	}
	
	driver = NULL;
	printf("Available drivers (%d): ", numdrivers);
	while ((driver = dbi_driver_list(driver)) != NULL) {
		printf("%s ", dbi_driver_get_name(driver));
	}
	driver = NULL;
	drivername[0] = '\n';

	while (drivername[0] == '\n') {
		printf("\ntest which driver? ");
		fgets(drivername, 64, stdin);
	}
	drivername[strlen(drivername)-1] = '\0';

	printf("database hostname? [localhost] ");
	fgets(hostname, 64, stdin);
	if (hostname[0] == '\n') strncpy(hostname, "localhost", 63), hostname[63] = '\0';
	else hostname[strlen(hostname)-1] = '\0';
	
	printf("database username? [none] ");
	fgets(username, 64, stdin);
	if (username[0] == '\n') username[0] = '\0';
	else username[strlen(username)-1] = '\0';
	
	printf("database password? [none] ");
	fgets(password, 64, stdin);
	if (password[0] == '\n') password[0] = '\0';
	else password[strlen(password)-1] = '\0';

	printf("database name? [libdbitest] ");
	fgets(dbname, 64, stdin);
	if (dbname[0] == '\n') strncpy(dbname, "libdbitest", 63), dbname[63] = '\0';
	else dbname[strlen(dbname)-1] = '\0';
	
	if ((conn = dbi_conn_new(drivername)) == NULL) {
		printf("Can't instantiate '%s' driver into a dbi_conn!\n", drivername);
		dbi_shutdown();
		return 1;
	}

	driver = dbi_conn_get_driver(conn);

	printf("\nPlugin information:\n-------------------\n");
	printf("\tName:       %s\n"
		   "\tFilename:   %s\n"
		   "\tDesc:       %s\n"
		   "\tMaintainer: %s\n"
		   "\tURL:        %s\n"
		   "\tVersion:    %s\n"
		   "\tCompiled:   %s\n", dbi_driver_get_name(driver), dbi_driver_get_filename(driver), dbi_driver_get_description(driver), dbi_driver_get_maintainer(driver), dbi_driver_get_url(driver), dbi_driver_get_version(driver), dbi_driver_get_date_compiled(driver));

	dbi_conn_set_option(conn, "host", hostname);
	dbi_conn_set_option(conn, "username", username);
	dbi_conn_set_option(conn, "password", password);
	dbi_conn_set_option(conn, "dbname", dbname);

	if (dbi_conn_connect(conn) < 0) {
		dbi_conn_error(conn, &errmsg);
		printf("Unable to connect! Error message: %s\n", errmsg);
		free(errmsg);
		dbi_shutdown();
		return 1;
	}

	printf("\nSuccessfully connected! Available tables: \n\t");
	
	if ((result = dbi_conn_get_table_list(conn, dbname)) == NULL) {
		dbi_conn_error(conn, &errmsg);
		printf("AAH! Can't get table list! Error message: %s\n", errmsg);
		free(errmsg);
		dbi_conn_close(conn);
		dbi_shutdown();
		return 1;
	}

	while (dbi_result_next_row(result)) {
		const char *tablename = NULL;
		tablename = dbi_result_get_string_idx(result, 1);
		printf("%s ", tablename);
	}

	dbi_result_free(result);
	printf("\n\n");
	printf("SUCCESS! All done, disconnecting and shutting down libdbi. Have a nice day.\n\n");

	dbi_conn_close(conn);
	dbi_shutdown();

	return 0;
}
