#include <stdio.h>
#include <string.h>
#include "postgres.h"
#include "libpq-fe.h"
#include "fmgr.h"
#include "utils/geo_decls.h"

#include <stdlib.h>

// void do_exit(PGconn *conn) {
    
//     PQfinish(conn);
//     exit(1);
// }

// int main() {
    
//     PGconn *conn = PQconnectdb("user=janbodnar dbname=testdb");

//     if (PQstatus(conn) == CONNECTION_BAD) {
        
//         fprintf(stderr, "Connection to database failed: %s\n",
//             PQerrorMessage(conn));
//         do_exit(conn);
//     }

//     int ver = PQserverVersion(conn);

//     printf("Server version: %d\n", ver);
    
//     PQfinish(conn);

//     return 0;
// }


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


typedef struct Complex {
    double      x;
    double      y;
} Complex;


// PG_FUNCTION_INFO_V1(add_one);

// Datum
// add_one(PG_FUNCTION_ARGS)
// {
//     int32   arg = PG_GETARG_INT32(0);

//     PG_RETURN_INT32(arg + 1);
// }

PG_FUNCTION_INFO_V1(complex_in);

Datum
complex_in(PG_FUNCTION_ARGS)
{
    // char       *str = PG_GETARG_CSTRING(0);
    double      x,
                y;
    Complex    *result;

    // if (sscanf(str, " ( %lf , %lf )", &x, &y) != 2)
    //     ereport(ERROR,
    //             (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
    //              errmsg("invalid input syntax for type %s: \"%s\"",
    //                     "complex", str)));

    x = 5;
    y = 8;

    result = (Complex *) palloc(sizeof(Complex));
    result->x = x;
    result->y = y;
    
    PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(complex_out);

Datum
complex_out(PG_FUNCTION_ARGS)
{
    Complex    *complex = (Complex *) PG_GETARG_POINTER(0);
    char       *result;

    result = psprintf("(%g,%g)", complex->x, complex->y);
    PG_RETURN_CSTRING(result);
}


// int main() {
    
//     int lib_ver = 2; // PQlibVersion();

//     printf("Version of libpq: %d\n", lib_ver);
    
//     return 0;
// }
