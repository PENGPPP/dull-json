#ifndef DULLJSON_H__
#define DULLJSON_H__

#include <stddef.h>

#define DULL_INIT(v) do{(v)->type = DULL_NULL;}while(0)
#define dull_set_null(v) dull_free(v)

typedef enum {DULL_NULL, DULL_FALSE, DULL_TRUE, DULL_NUMBER, DULL_STRING, DULL_ARRAY, DULL_OBJECT} dull_type;

typedef struct dull_value dull_value;
struct dull_value
{
    dull_type type;
    union
    {
        struct { dull_value* e; size_t size;} a;
        struct { char* s; size_t len; } s;
        double n;
    } u;
};

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
    DULL_PARSE_INVALID_UNICODE_SURROGATE,
    DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
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

size_t dull_get_array_size(dull_value* v);
dull_value* dull_get_array_element(dull_value* v, size_t index);

#endif /* DULLJSON_H__ */