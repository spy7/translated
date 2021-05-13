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
    int32           length;
    char            text[FLEXIBLE_ARRAY_MEMBER];
} TranslatedText;

typedef struct {
    int32           length;
    int32           number;
    char            texts[FLEXIBLE_ARRAY_MEMBER];
} Translated;


/////////////////
//// HELPERS ////
/////////////////

int read_number(char*, int, int);
int next_index(char*, int, int);
TranslatedText* read_text(char*, int, int);
int next_text_index(char*, TranslatedText*, int, int);
char* show_text(TranslatedText*);
int len_number(int);
char* write_number(int);
char* write_text(Translated*);
char* read_setting(char*);
char** read_setting_2(char*, char*);
char* get_language(void);
char* get_language_text(Translated*, char*);

// read number from text
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

// get index after read number and comma
int next_index(char* str, int index, int max) {
    while (index < max && str[index] != ',' && str[index] != ')') {
        index++;
    }
    if (index == max) {
        return -1;
    }
    return index + 1;
}

// read size and text into a TranslatedText
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

// get index after read size, text and comma
int next_text_index(char* str, TranslatedText* text, int index, int max) {
    int next = next_index(str, index, max);

    if (text == NULL || next == -1)
        return -1;

    next += text->length - sizeof(int32);
    if (next >= max || (str[next] != ',' && str[next] != ')'))
        return -1;
    return next + 1;
}

// return string from TranslatedText
char* show_text(TranslatedText* text) {
    int length = text->length - sizeof(int32);
    char* read = (char*) palloc(sizeof(char) * (length + 1));
    memcpy(read, text->text, length);
    read[length] = 0;
    return read;
}

// convert number to string
char* write_number(int number) 
{
    char* result = palloc(sizeof(char) * 10);
    sprintf(result, "%d", number);
    return result;
}

// return length of a number (after convert to text)
int len_number(int number)
{
    char* text = write_number(number);
    int result = strlen(text);
    pfree(text);
    return result;
}

// show Translated as a String
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

// read settings
char* read_setting(char* name)
{
    char *result = NULL;
    int  ret;
    int  proc;

    SPI_connect();

    ret = SPI_exec(psprintf("SELECT COALESCE(current_setting('%s', TRUE), '')", name), 0);

    proc = SPI_processed;

    if (ret > 0 && SPI_tuptable != NULL && proc > 0)
    {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        HeapTuple tuple = SPI_tuptable->vals[0];
        char      *response = SPI_getvalue(tuple, tupdesc, 1);
        result = palloc(sizeof(char) * strlen(response) + 1);
        sprintf(result, "%s", response);
    }
    
    SPI_finish();

    return result;
}

char** read_setting_2(char* name1, char* name2)
{
    char **result = NULL;
    int  ret;
    int  proc;

    SPI_connect();

    ret = SPI_exec(psprintf("SELECT COALESCE(current_setting('%s', TRUE), ''), COALESCE(current_setting('%s', TRUE), '')", name1, name2), 0);

    proc = SPI_processed;

    if (ret > 0 && SPI_tuptable != NULL && proc > 0)
    {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        HeapTuple tuple = SPI_tuptable->vals[0];
        char      *response;
        
        result = palloc(sizeof(char*) * 2);

        response = SPI_getvalue(tuple, tupdesc, 1);
        result[0] = palloc(sizeof(char) * strlen(response) + 1);
        sprintf(result[0], "%s", response);

        response = SPI_getvalue(tuple, tupdesc, 2);
        result[1] = palloc(sizeof(char) * strlen(response) + 1);
        sprintf(result[1], "%s", response);
    }
    
    SPI_finish();

    return result;
}

// get language
char* get_language()
{
    char *language = read_setting("trl.lang");

    if (language == NULL || strlen(language) < 1)
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_PARAMETER), errmsg("Undefined language. Use: SET trl.lang")));

    return language;
}

// get text from language (or first)
char* get_language_text(Translated* translated, char* language)
{
    int            number;
    TranslatedText *text;
    char           *first;
    char           *result;

    number = translated->number;
    if (number < 1) {
        result = palloc(sizeof(char));
        result[0] = 0;
        return result;
    }

    text = (TranslatedText*)(((char *)translated) + (sizeof(int32) * 2));
    for (int i = 0; i < number; i+=2)
    {
        if (memcmp(language, text->text, text->length - sizeof(int32)) == 0) {
            text = (TranslatedText*)(((char *)text) + text->length);
            return show_text(text);
        }
        text = (TranslatedText*)(((char *)text) + text->length);
        text = (TranslatedText*)(((char *)text) + text->length);
    }

    first = read_setting("trl.first");
    if (first != NULL && strcmp(first, "false") == 0) {
        result = palloc(sizeof(char));
        result[0] = 0;
        return result;
    }

    text = (TranslatedText*)(((char *)translated) + (sizeof(int32) * 2));
    text = (TranslatedText*)(((char *)text) + text->length);
    return show_text(text);
}

///////////////////
//// FUNCTIONS ////
///////////////////

PG_FUNCTION_INFO_V1(create_text);
PG_FUNCTION_INFO_V1(add_text);
PG_FUNCTION_INFO_V1(compare_string);
PG_FUNCTION_INFO_V1(compare_string_not);
PG_FUNCTION_INFO_V1(compare_text);
PG_FUNCTION_INFO_V1(compare_text_not);
        
Datum
create_text(PG_FUNCTION_ARGS)
{
    Translated *result;
    char       *right_arg = PG_GETARG_CSTRING(0);
    char       *bytes;
    char       *language;
    int32      language_length;
    int32      text_length;
    int32      total_length;

    language = get_language();
    language_length = strlen(language);
    text_length = strlen(right_arg);
    total_length = sizeof(int32) * 4 + language_length + text_length;

    result = (Translated *) palloc(total_length);
    SET_VARSIZE(result, total_length);
    result->number = 2;
    bytes = result->texts;
    ((TranslatedText*)bytes)->length = language_length + sizeof(int32);
    bytes += sizeof(int32);
    memcpy(bytes, language, language_length);
    bytes += language_length;
    ((TranslatedText*)bytes)->length = text_length + sizeof(int32);
    bytes += sizeof(int32);
    memcpy(bytes, right_arg, text_length);

    PG_RETURN_POINTER(result);
}

Datum
add_text(PG_FUNCTION_ARGS)
{
    Translated     *result;
    Translated     *left_arg = (Translated*) PG_GETARG_POINTER(0);
    char           *right_arg = PG_GETARG_CSTRING(1);
    char           *bytes;
    char           *language;
    TranslatedText *text;
    int32          language_length;
    int32          text_length;
    int32          length;
    int32          number;
    int32          index = -1;

    language = get_language();
    language_length = strlen(language);
    text_length = strlen(right_arg);

    length = VARSIZE_4B(left_arg) + language_length + text_length + sizeof(int32) * 2;
    number = left_arg->number;
    text = (TranslatedText*)(((char *)left_arg) + (sizeof(int32) * 2));
    for (int i = 0; i < number; i+=2)
    {
        if (memcmp(language, text->text, text->length - sizeof(int32)) == 0) {
            length -= text->length;
            text = (TranslatedText*)(((char *)text) + text->length);
            length -= text->length;
            index = i;
            break;
        }
        text = (TranslatedText*)(((char *)text) + text->length);
        text = (TranslatedText*)(((char *)text) + text->length);
    }

    result = (Translated *) palloc(length);
    SET_VARSIZE(result, length);
    result->number = number;
    bytes = result->texts;
    text = (TranslatedText*)(((char *)left_arg) + (sizeof(int32) * 2));
    for (int i = 0; i < number; i+=2) {
        memcpy(bytes, text, text->length);
        bytes += text->length;
        text = (TranslatedText*)(((char *)text) + text->length);

        if (i == index) {
            ((TranslatedText*)bytes)->length = text_length + sizeof(int32);
            bytes += sizeof(int32);
            memcpy(bytes, right_arg, text_length);
            bytes += text_length;
        }
        else 
        {
            memcpy(bytes, text, text->length);
            bytes += text->length;
        }
        text = (TranslatedText*)(((char *)text) + text->length);
    }
    if (index == -1) {
        ((TranslatedText*)bytes)->length = language_length + sizeof(int32);
        bytes += sizeof(int32);
        memcpy(bytes, language, language_length);
        bytes += language_length;
        ((TranslatedText*)bytes)->length = text_length + sizeof(int32);
        bytes += sizeof(int32);
        memcpy(bytes, right_arg, text_length);

        result->number += 2;
    }

    PG_RETURN_POINTER(result);
}

Datum
compare_string(PG_FUNCTION_ARGS)
{
    Translated *left_arg = (Translated *) PG_GETARG_POINTER(0);
    char       *right_arg = PG_GETARG_CSTRING(1);
    char       **settings;
    char       *text;

    settings = read_setting_2("trl.lang", "trl.output");
    if (settings == NULL || strlen(settings[0]) < 1 || strcmp(settings[1], "false") == 0)
        PG_RETURN_BOOL(strcmp(write_text(left_arg), right_arg) == 0);

    text = get_language_text(left_arg, settings[0]);
    PG_RETURN_BOOL(strcmp(text, right_arg) == 0);
}

Datum
compare_string_not(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(!compare_string(fcinfo));
}

Datum
compare_text(PG_FUNCTION_ARGS)
{
    Translated *left_arg = (Translated *) PG_GETARG_POINTER(0);
    Translated *right_arg = (Translated *) PG_GETARG_POINTER(1);
    char       **settings;
    char       *language;
    char       *text1;
    char       *text2;

    settings = read_setting_2("trl.lang", "trl.output");
    if (settings == NULL || strlen(settings[0]) < 1 || strcmp(settings[1], "false") == 0)
        PG_RETURN_BOOL(strcmp(write_text(left_arg), write_text(right_arg))) == 0;

    language = palloc(sizeof(char) * strlen(settings[0]) + 1);
    strcpy(language, settings[0]);

    text1 = get_language_text(left_arg, language);
    text2 = get_language_text(right_arg, language);
    PG_RETURN_BOOL(strcmp(text1, text2) == 0);
}

Datum
compare_text_not(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(!compare_text(fcinfo));
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

    if (length < 6 || memcmp("%TRL(", str, 5) != 0) {
        char* input = read_setting("trl.input");
        if (input != NULL && strcmp(input, "true") == 0)
            return create_text(fcinfo);
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("Invalid input syntax for type translated: \"%s\"", str)));
    }

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
    char       **settings;
    char       *result;

    settings = read_setting_2("trl.lang", "trl.output");
    if (settings == NULL || strlen(settings[0]) < 1 || strcmp(settings[1], "false") == 0)
        PG_RETURN_CSTRING(write_text(translated));

    result = get_language_text(translated, settings[0]);
    PG_RETURN_CSTRING(result);
}
