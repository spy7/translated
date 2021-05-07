#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "postgres.h"
#include "libpq-fe.h"
#include "libpq/pqformat.h"
#include "fmgr.h"
#include "utils/geo_decls.h"
#include "lib/stringinfo.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


typedef struct {
    int32 length;
    char data[FLEXIBLE_ARRAY_MEMBER];
} Translated;


////////////////////////
//// INPUT FUNCTION ////
////////////////////////

PG_FUNCTION_INFO_V1(translated_in);

Datum
translated_in(PG_FUNCTION_ARGS)
{
    char       *str = PG_GETARG_CSTRING(0);
    Translated *result;
    int32      length = strlen(str) + 1;
    
    result = (Translated *) palloc(VARHDRSZ + length);
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(result->data, str, length);
    ereport(INFO, (errmsg("Length: %d", length)));

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
    char       *result;

    // ereport(INFO, (errmsg("Length: %d", translated->length)));
    result = psprintf("%s", translated->data);

    PG_RETURN_CSTRING(result);
}
