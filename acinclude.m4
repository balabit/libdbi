##
## originally by Christian Hammers <ch@westend.com>,
## who borrowed ideas from Stephan Kulow.
## then this file was tweaked by David Parker for libdbi.
##

AC_DEFUN(AC_FIND_FILE,
[
$3=NO
for i in $2; do
	for j in $1; do
		if test -r "$i/$j"; then
			$3=$i
			break 2
		fi
	done
done
])

## MYSQL

AC_DEFUN(AC_CHECK_MYSQL,
[
ac_mysql="NO"
ac_mysql_incdir="NO"
ac_mysql_libdir="NO"

# exported variables
MYSQL_LIBS=""
MYSQL_LFLAGS=""
MYSQL_INCLUDE=""

AC_MSG_CHECKING(for MySQL support)

AC_ARG_WITH(mysql,
	[  --with-mysql		  Include MySQL support.],
	[  ac_mysql="YES" ])
AC_ARG_WITH(mysql-dir,
	[  --with-mysql-dir	  Specifies the MySQL root directory.],
	[  ac_mysql_incdir="$withval"/include
	   ac_mysql_libdir="$withval"/lib ])
AC_ARG_WITH(mysql-incdir,
	[  --with-mysql-incdir	  Specifies where the MySQL include files are.],
	[  ac_mysql_incdir="$withval" ])
AC_ARG_WITH(mysql-libdir,
	[  --with-mysql-libdir	  Specifies where the MySQL libraries are.],
	[  ac_mysql_libdir="$withval" ])

if test "$ac_mysql" = "YES"; then
	if test "$ac_mysql_incdir" = "NO" || test "$ac_mysql_libs" = "NO"; then
		mysql_incdirs="/usr/include /usr/local/include /usr/include/mysql /usr/local/include/mysql /usr/local/mysql/include /usr/local/mysql/include/mysql /opt/mysql/include/mysql"
		AC_FIND_FILE(mysql.h, $mysql_incdirs, ac_mysql_incdir)
		mysql_libdirs="/usr/lib /usr/local/lib /usr/lib/mysql /usr/local/lib/mysql /usr/local/mysql/lib /usr/local/mysql/lib/mysql /opt/mysql/lib/mysql"
		mysql_libs="libmysqlclient.so libmysqlclient.a"
		AC_FIND_FILE($mysql_libs, $mysql_libdirs, ac_mysql_libdir)
		if test "$ac_mysql_incdir" = "NO"; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([Invalid MySQL directory - include files not found.])
		fi
		if test "$ac_mysql_libdir" = "NO"; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([Invalid MySQL directory - libraries not found.])
		fi
	fi
	AC_MSG_RESULT([yes: libs in $ac_mysql_libdir, headers in $ac_mysql_incdir])
	AM_CONDITIONAL(HAVE_MYSQL, true)
	
	MYSQL_LIBS=-lmysqlclient
	MYSQL_INCLUDE=-I$ac_mysql_incdir
	MYSQL_LFLAGS=-L$ac_mysql_libdir
	
	AC_SUBST(MYSQL_LIBS)
	AC_SUBST(MYSQL_INCLUDE)
	AC_SUBST(MYSQL_LFLAGS)
else
	AC_MSG_RESULT(no)
fi
])

## PGSQL

AC_DEFUN(AC_CHECK_PGSQL,
[
ac_pgsql="NO"
ac_pgsql_incdir="NO"
ac_pgsql_libdir="NO"

# exported variables
PGSQL_LIBS=""
PGSQL_LFLAGS=""
PGSQL_INCLUDE=""

AC_MSG_CHECKING(for PostgreSQL support)

AC_ARG_WITH(pgsql,
	[  --with-pgsql		  Include PostgreSQL support.],
	[  ac_pgsql="YES" ])
AC_ARG_WITH(pgsql-dir,
	[  --with-pgsql-dir	  Specifies the PostgreSQL root directory.],
	[  ac_pgsql_incdir="$withval"/include
	   ac_pgsql_libdir="$withval"/lib ])
AC_ARG_WITH(pgsql-incdir,
	[  --with-pgsql-incdir	  Specifies where the PostgreSQL include files are.],
	[  ac_pgsql_incdir="$withval" ])
AC_ARG_WITH(pgsql-libdir,
	[  --with-pgsql-libdir	  Specifies where the PostgreSQL libraries are.],
	[  ac_pgsql_libdir="$withval" ])

if test "$ac_pgsql" = "YES"; then
	if test "$ac_pgsql_incdir" = "NO" || test "$ac_pgsql_libs" = "NO"; then
		pgsql_incdirs="/usr/include /usr/local/include /usr/include/pgsql /usr/local/include/pgsql /usr/local/pgsql/include /usr/include/postgresql /usr/local/postgresql/include /opt/pgsql/include"
		AC_FIND_FILE(libpq-fe.h, $pgsql_incdirs, ac_pgsql_incdir)
		pgsql_libdirs="/usr/lib /usr/local/lib /usr/lib/pgsql /usr/local/lib/pgsql /usr/local/pgsql/lib /opt/pgsql/lib"
		pgsql_libs="libpq.so libpq.a"
		AC_FIND_FILE($pgsql_libs, $pgsql_libdirs, ac_pgsql_libdir)
		if test "$ac_pgsql_incdir" = "NO"; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([Invalid PostgreSQL directory - include files not found.])
		fi
		if test "$ac_pgsql_libdir" = "NO"; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([Invalid PostgreSQL directory - libraries not found.])
		fi
	fi
	AC_MSG_RESULT([yes: libs in $ac_pgsql_libdir, headers in $ac_pgsql_incdir])
	AM_CONDITIONAL(HAVE_PGSQL, true)
	
	PGSQL_LIBS=-lpq
	PGSQL_INCLUDE=-I$ac_pgsql_incdir
	PGSQL_LFLAGS=-L$ac_pgsql_libdir
	
	AC_SUBST(PGSQL_LIBS)
	AC_SUBST(PGSQL_INCLUDE)
	AC_SUBST(PGSQL_LFLAGS)
else
	AC_MSG_RESULT(no)
fi
])
