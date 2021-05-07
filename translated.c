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
    int32           length;
    char            text[FLEXIBLE_ARRAY_MEMBER];
} TranslatedText;

typedef struct {
    int32           length;
    int32           number;
    char            texts[FLEXIBLE_ARRAY_MEMBER];
} Translated;


///////////////////
//// FUNCTIONS ////
///////////////////

int read_number(char*, int, int);
int next_index(char*, int, int);
TranslatedText* read_text(char*, int, int);
int next_text_index(char*, TranslatedText*, int, int);
char* show_text(TranslatedText*);
int len_number(int);
char* write_number(int);
char* write_text(Translated*);

int read_number(char* str, int index, int max)
{
    int number_index = 0;
    char number[20];
    while (index < max && number_index < 20 && str[index] >= '0' && str[index] <= '9') {
        number[number_index] = str[index];
        number_index++;
        index++;
    }
    if (number_index == 0 || number_index == 20 || index == max || (str[index] != ',' && str[index] != ')')) {
        return -1;
    }
    number[number_index] = 0;
    return atoi(number);
}

int next_index(char* str, int index, int max) {
    while (index < max && str[index] != ',' && str[index] != ')') {
        index++;
    }
    if (index == max) {
        return -1;
    }
    return index + 1;
}

TranslatedText* read_text(char* str, int index, int max) {
    int length = read_number(str, index, max);
    int next = next_index(str, index, max);
    TranslatedText *result;

    if (length == -1 || next == -1 || next + length >= max)
        return NULL;
    result = (TranslatedText *) palloc(sizeof(int32) + length);
    result->length = sizeof(int32) + length;
    memcpy(result->text, str + next, length);
    return result;
}

int next_text_index(char* str, TranslatedText* text, int index, int max) {
    int next = next_index(str, index, max);

    if (text == NULL || next == -1)
        return -1;

    next += text->length - sizeof(int32);
    if (next >= max || (str[next] != ',' && str[next] != ')'))
        return -1;
    return next + 1;
}

char* show_text(TranslatedText* text) {
    int length = text->length - sizeof(int32);
    char* read = (char*) palloc(sizeof(char) * (length + 1));
    memcpy(read, text->text, length);
    read[length] = 0;
    return read;
}

char* write_number(int number) 
{
    char* result = palloc(sizeof(char) * 10);
    sprintf(result, "%d", number);
    return result;
}

int len_number(int number)
{
    char* text = write_number(number);
    int result = strlen(text);
    pfree(text);
    return result;
}

char* write_text(Translated* translated)
{
    int            number;
    char           *text_number;
    char           *result;
    int            length;
    int            index;
    TranslatedText *text;

    number = translated->number;
    length = 7 + len_number(number);

    text = (TranslatedText*)(((char *)translated) + (sizeof(int32) * 2));
    for (int i = 0; i < number; i++)
    {
        length += 1 + len_number(text->length - sizeof(int32)) + 1 + text->length - sizeof(int32);
        text = (TranslatedText*)(((char *)text) + text->length);
    }

    result = (char*) palloc(sizeof(char) * length);
    sprintf(result, "%%TRL(%d", number);
    index = 5 + len_number(number);
    
    text = (TranslatedText*)(((char *)translated) + (sizeof(int32) * 2));
    for (int i = 0; i < number; i++)
    {
        result[index] = ',';
        text_number = write_number(text->length - sizeof(int32));
        memcpy(result + index + 1, text_number, strlen(text_number));
        index += 1 + strlen(text_number);
        pfree(text_number);

        result[index] = ',';
        memcpy(result + index + 1, text->text, text->length - sizeof(int32));
        index += 1 + text->length - sizeof(int32);

        text = (TranslatedText*)(((char *)text) + text->length);
    }
    
    result[length - 2] = ')';
    result[length - 1] = 0;
    return result;
}

////////////////////////
//// INPUT FUNCTION ////
////////////////////////

PG_FUNCTION_INFO_V1(translated_in);

Datum
translated_in(PG_FUNCTION_ARGS)
{
    Translated      *result;
    TranslatedText  *text;
    TranslatedText  **texts;
    char            *bytes;
    char            *str = PG_GETARG_CSTRING(0);
    int32           length = strlen(str) + 1;
    int32           number;
    int32           index;
    int32           total_length = sizeof(int32) * 2;

    if (length < 6 || memcmp("%TRL(", str, 5) != 0)
         ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("Invalid input syntax for type translated: \"%s\"", str)));

    number = read_number(str, 5, length);    
    index = next_index(str, 5, length);

    if (number == -1 || index == -1 || number % 2 == 1) 
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("Invalid input syntax for type translated: \"%s\"", str)));

    texts = (TranslatedText **) palloc(sizeof(TranslatedText*) * number);

    for (int i = 0; i < number; i++) {
        text = read_text(str, index, length);
        index = next_text_index(str, text, index, length);
        if (index == -1)
            ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("Invalid input syntax for type translated: \"%s\"", str)));

        texts[i] = text;
        total_length += text->length;
    }

    result = (Translated *) palloc(total_length);
    SET_VARSIZE(result, total_length);
    result->number = number;
    bytes = result->texts;
    for (int i = 0; i < number; i++) {
        memcpy(bytes, texts[i], texts[i]->length);
        bytes += texts[i]->length;
    }

    // ereport(INFO, (errmsg("Length: %d", total_length)));

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

    ereport(INFO, (errmsg("nm: %d", translated->number)));
    result = psprintf("%s", write_text(translated));

    PG_RETURN_CSTRING(result);
}
