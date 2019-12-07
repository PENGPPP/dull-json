#ifndef DULLJSON_H__
#define DULLJSON_H__

typedef enum {DULL_NULL, DULL_TRUE, DULL_FALSE, DULL_NUMBER, DULL_STRING, DULL_ARRAY, DULL_OBJECT} dull_type;

typedef struct 
{
    dull_type type;
} dull_value;

typedef struct
{
    const char* json;
} dull_context;

enum 
{
    DULL_PARSE_OK,
    DULL_PARSE_EXPECT_VALUE,
    DULL_PARSE_INVALID_VALUE,
    DULL_PARSE_ROOT_NOT_SINGULAR
};

int dull_parse(dull_value* v, const char* json);
dull_type dull_get_type(const dull_value* v);

#endif /* DULLJSON_H__ */