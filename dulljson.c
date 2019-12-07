#include "dulljson.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// JSON-text = ws value ws
// ws = *(%x20 / %x09 / %x0A / %x0D)
// value = null / false / true 
// null  = "null"
// false = "false"
// true  = "true"

// #define EXPECT(c, ch) do{ assert(*c->json == (ch)); c->json++; }while(0)

static void dull_parse_whitespace(dull_context* c)
{
    const char* p = c->json;
    while(*p == ' ' || *p == '\t'|| *p == '\n'|| *p == '\r')
        p++;
    c->json = p;
}

// static int dull_parse_null(dull_context* c, dull_value* v)
// {
//     EXPECT(c, "n");
//     if(c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//         return DULL_PARSE_INVALID_VALUE;
    
//     c->json += 3;
//     v->type = DULL_NULL;

//     return DULL_PARSE_OK;
// }

static int dull_parse_literal(dull_context* c, dull_value* v, const char* literal, dull_type t)
{
    assert(literal != NULL);
    
    int index = 0;
    int size = strlen(literal);
    int c_size = strlen(c->json);

    while(index < size)
    {
        if(index >= c_size || c->json[index] != literal[index])
        {
            return DULL_PARSE_INVALID_VALUE;
        }
        index++;
    }

    c->json += size;
    v->type = t;
    return DULL_PARSE_OK;
}

static int dull_parse_value(dull_context* c, dull_value* v)
{
    switch (*c->json)
    {
        case 'n': return dull_parse_literal(c, v, "null", DULL_NULL);
        case 't': return dull_parse_literal(c, v, "true", DULL_TRUE);
        case 'f': return dull_parse_literal(c, v, "false", DULL_FALSE);
        case '\0': return DULL_PARSE_EXPECT_VALUE;
        default: return DULL_PARSE_INVALID_VALUE;
    }
}

int dull_parse(dull_value* v, const char* json)
{
    dull_context c;
    c.json = json;
    v->type = DULL_NULL;
    dull_parse_whitespace(&c);
    int ret;
    if((ret = dull_parse_value(&c, v)) == DULL_PARSE_OK)
    {
        dull_parse_whitespace(&c);
        if (*c.json != '\0')
        {   
            return DULL_PARSE_ROOT_NOT_SINGULAR;
        }
        
    }
    return ret;
}

dull_type dull_get_type(const dull_value* v)
{
    assert(v != NULL);
    return v->type;
}