#ifndef DULLJSON_H__
#define DULLJSON_H__

#include <stddef.h>

#define DULL_INIT(v) do{(v)->type = DULL_NULL;}while(0)

typedef enum {DULL_NULL, DULL_TRUE, DULL_FALSE, DULL_NUMBER, DULL_STRING, DULL_ARRAY, DULL_OBJECT} dull_type;

typedef struct 
{
    dull_type type;
    union
    {
        struct { char* s; size_t len; } s;
        double n;
    } u;
} dull_value;

enum {
    DULL_PARSE_OK = 0,
    DULL_PARSE_EXPECT_VALUE,
    DULL_PARSE_INVALID_VALUE,
    DULL_PARSE_ROOT_NOT_SINGULAR,
    DULL_PARSE_NUMBER_TOO_BIG,
    DULL_PARSE_MISS_QUOTATION_MARK,
    DULL_PARSE_INVALID_STRING_ESCAPE,
    DULL_PARSE_INVALID_STRING_CHAR,
    DULL_PARSE_INVALID_UNICODE_HEX,
    DULL_PARSE_INVALID_UNICODE_SURROGATE
};

int dull_parse(dull_value* v, const char* json);
dull_type dull_get_type(const dull_value* v);

double dull_get_number(const dull_value* v);
void dull_set_number(dull_value* v, double d);

int dull_get_boolean(const dull_value* v);
void dull_set_boolean(dull_value* v, int d);

const char* dull_get_string(const dull_value* v);
int dull_get_string_length(const dull_value* v);
void dull_set_string(dull_value* v, const char* c, size_t len);

void dull_free(dull_value* v);

#endif /* DULLJSON_H__ */