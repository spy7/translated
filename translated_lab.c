#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "postgres.h"
#include "libpq-fe.h"
#include "libpq/pqformat.h"
#include "fmgr.h"
#include "utils/geo_decls.h"
#include "lib/stringinfo.h"
#include "executor/spi.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


typedef struct {
    int32 key;
} Translated;


////////////////////////
//// INPUT FUNCTION ////
////////////////////////

PG_FUNCTION_INFO_V1(translated_in);

Datum
translated_in(PG_FUNCTION_ARGS)
{
    char       *str = PG_GETARG_CSTRING(0);
    int32      key;
    int        ret;
    int        proc;
    Translated *result;

    SPI_connect();

    PG_TRY();
    {
        ret = SPI_exec("SELECT current_setting('my.lang')", 0);
    }
    PG_CATCH();
    {
        SPI_finish();
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_PARAMETER), errmsg("Undefined language. Use: SET my.lang")));
    }
    PG_END_TRY();

    proc = SPI_processed;

    if (ret > 0 && SPI_tuptable != NULL)
    {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        char buf[8192];
        int i, j;
        for (j = 0; j < proc; j++)
        {
            HeapTuple tuple = tuptable->vals[j];
            for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++)
                snprintf(buf + strlen(buf),
                    sizeof(buf) - strlen(buf),
                    " %s(%s::%s)%s",
                    SPI_fname(tupdesc, i),
                    SPI_getvalue(tuple, tupdesc, i),
                    SPI_gettype(tupdesc, i);
            ereport(INFO, (errmsg("ROW: %s", buf)));
        }
    }

    if (sscanf(str, "%d", &key) != 1) {
        SPI_finish();
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("Invalid input: \"%s\"", str)));
    }

    // if (sscanf(str, " ( %lf , %lf )", &x, &y) != 2)
    //     ereport(ERROR,
    //             (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
    //              errmsg("invalid input syntax for type %s: \"%s\"",
    //                     "complex", str)));

    result = (Translated *) palloc(sizeof(Translated));
    result->key = key;

    SPI_finish();

    PG_RETURN_POINTER(result);
}


/////////////////////////
//// OUTPUT FUNCTION ////
/////////////////////////

PG_FUNCTION_INFO_V1(translated_out);

Datum
translated_out(PG_FUNCTION_ARGS)
{
    Translated *translated = (Translated *) PG_GETARG_POINTER(0);
    char *result;

    // result = psprintf("%d,%g)", complex->x, complex->y);
    // key = PG_GETARG_INT32(0);

    result = psprintf("%d b", translated->key);

    PG_RETURN_CSTRING(result);
}
