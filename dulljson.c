#include "dulljson.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#define EXPECT(c, ch) do{ assert(*c->json == (ch)); c->json++; }while(0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do{*(char*)dull_context_push((c), sizeof(char)) = (ch);} while(0)
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

typedef struct
{
    const char* json;
    char* stack;
    size_t top, size;
} dull_context;


void dull_free(dull_value* v)
{
    assert(v != NULL);
    switch (v->type)
    {
    case DULL_STRING:
        free(v->u.s.s);
        break;
    case DULL_ARRAY:
        for(int i = 0 ; i < v->u.a.size; i++)
            dull_free(&v->u.a.e[i]);
        free(v->u.a.e);
        break;
    default:
        break;
    }
    v->type = DULL_NULL;
}

static void dull_parse_whitespace(dull_context* c)
{
    const char* p = c->json;
    while(*p == ' ' || *p == '\t'|| *p == '\n'|| *p == '\r')
        p++;
    c->json = p;
}

static int dull_parse_literal(dull_context* c, dull_value* v, const char* literal, dull_type t)
{
    assert(literal != NULL);
    
    size_t index = 0;
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
    // size_t i;
    // EXPECT(c, literal[0]);
    // for (i = 0; literal[i + 1]; i++)
    //     if (c->json[i] != literal[i + 1])
    //         return DULL_PARSE_INVALID_VALUE;
    // c->json += i;
    // v->type = t;
    // return DULL_PARSE_OK;
}

static int dull_parse_number(dull_context* c, dull_value* v)
{
    const char* p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else
    {
        if(!ISDIGIT1TO9(*p)) return DULL_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }

    if(*p == '.')
    {
        p++;
        if(!ISDIGIT(*p)) return DULL_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }

    if(*p == 'e' || *p == 'E')
    {
        p++;
        if(*p == '-' || *p == '+') p++;
        if(!ISDIGIT(*p)) return DULL_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
        
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return DULL_PARSE_NUMBER_TOO_BIG;
    v->type = DULL_NUMBER;
    c->json = p;
    return DULL_PARSE_OK;
}

void dull_set_boolean(dull_value* v, int b)
{
    assert(v != NULL);
    dull_free(v);
    v->type = b ? DULL_TRUE : DULL_FALSE;
}

int dull_get_boolean(const dull_value* v)
{
    assert(v != NULL && (v->type == DULL_TRUE || v->type == DULL_FALSE));
    return v->type == DULL_TRUE;
}

void dull_set_number(dull_value*v, double b)
{
    assert(v !=NULL);
    dull_free(v);
    v->u.n = b;
    v->type = DULL_NUMBER;
}

double dull_get_number(const dull_value* v)
{
    assert(v !=NULL && v->type == DULL_NUMBER);
    return v->u.n;
}

const char* dull_get_string(const dull_value* v)
{
    assert(v !=NULL && v->type == DULL_STRING);
    return v->u.s.s;
}

int dull_get_string_length(const dull_value* v)
{
    assert(v != NULL && v->type == DULL_STRING);
    return v->u.s.len;
}

void dull_set_string(dull_value* v, const char* c, size_t len)
{
    assert(v != NULL && (c != NULL || len == 0));
    dull_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, c, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = DULL_STRING;
}

int dull_get_string_len(dull_value* v)
{
    assert(v!= NULL && v->type == DULL_STRING);
    return v->u.s.len;
}

static void* dull_context_push(dull_context* c, size_t size)
{
    assert(c != NULL && size > 0);

    void* ret;
    if (c->top + size >= c->size)
    {
        if(c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;

        while(c->top + size >= c->size)
            c->size += c->size  >> 1;
            
        c->stack = (char*)realloc(c->stack, c->size);
    }

    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* dull_context_pop(dull_context* c, size_t size)
{
    assert(c != NULL && size >= 0);
    return c->stack + (c->top-=size);
}

static const char* dull_parse_hex4(const char* p, unsigned* u)
{
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}

static void dull_encode_utf8(dull_context* c, unsigned u) {
    if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

static int dull_parse_string(dull_context* c, dull_value* v)
{
    size_t head = c->top, len;
    EXPECT(c, '\"');
    const char* p = c->json;
    unsigned u, u2;

    for(;;)
    {
        char ch = *p++;
        switch(ch)
        {
            case '\"':
                len = c->top - head;
                const char* tc = (const char*)dull_context_pop(c, len);
                dull_set_string(v, tc, len);
                c->json = p;
                return DULL_PARSE_OK;
            case '\\':
                switch (*p++)
                {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/': PUTC(c, '/'); break;
                    case 'b': PUTC(c, '\b'); break;
                    case 'f': PUTC(c, '\f'); break;
                    case 'n': PUTC(c, '\n'); break;
                    case 'r': PUTC(c, '\r'); break;
                    case 't': PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = dull_parse_hex4(p, &u)))
                            STRING_ERROR(DULL_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = dull_parse_hex4(p, &u2)))
                                STRING_ERROR(DULL_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        dull_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(DULL_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0': 
                STRING_ERROR(DULL_PARSE_MISS_QUOTATION_MARK);
            default : 
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(DULL_PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}

static int dull_parse_array(dull_context* c, dull_value* v);

static int dull_parse_value(dull_context* c, dull_value* v)
{
    switch (*c->json)
    {
        case 'n': return dull_parse_literal(c, v, "null", DULL_NULL);
        case 't': return dull_parse_literal(c, v, "true", DULL_TRUE);
        case 'f': return dull_parse_literal(c, v, "false", DULL_FALSE);
        case '"' : return dull_parse_string(c, v);
        case '[': return dull_parse_array(c, v);
        default : return dull_parse_number(c, v);
        case '\0': return DULL_PARSE_EXPECT_VALUE;
    }
}

// ["adb",[1,2],3,"c"]
static int dull_parse_array(dull_context* c, dull_value* v)
{
    // size_t i, size = 0;
    // int ret;
    // EXPECT(c, '[');
    // dull_parse_whitespace(c);
    // if (*c->json == ']') {
    //     c->json++;
    //     v->type = DULL_ARRAY;
    //     v->u.a.size = 0;
    //     v->u.a.e = NULL;
    //     return DULL_PARSE_OK;
    // }
    // for (;;) {
    //     dull_value e;
    //     DULL_INIT(&e);
    //     if ((ret = dull_parse_value(c, &e)) != DULL_PARSE_OK)
    //         break;
    //     memcpy(dull_context_push(c, sizeof(dull_value)), &e, sizeof(dull_value));
    //     size++;
    //     dull_parse_whitespace(c);
    //     if (*c->json == ',') {
    //         c->json++;
    //         dull_parse_whitespace(c);
    //     }
    //     else if (*c->json == ']') {
    //         c->json++;
    //         v->type = DULL_ARRAY;
    //         v->u.a.size = size;
    //         size *= sizeof(dull_value);
    //         memcpy(v->u.a.e = (dull_value*)malloc(size), dull_context_pop(c, size), size);
    //         return DULL_PARSE_OK;
    //     }
    //     else {
    //         ret = DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    //         break;
    //     }
    // }
    // /* Pop and free values on the stack */
    // for (i = 0; i < size; i++)
    //     dull_free((dull_value*)dull_context_pop(c, sizeof(dull_value)));
    // return ret;

    EXPECT(c, '[');
    dull_parse_whitespace(c);
    if(*c->json == ']')
    {
        c->json++;
        v->type = DULL_ARRAY;
        v->u.a.e = NULL;
        v->u.a.size = 0;
        return DULL_PARSE_OK;
    }

    int ret, head = c->top;
    size_t size = 0, len;
    for(;;)
    {
        dull_value new_v;
        DULL_INIT(&new_v);
        dull_parse_whitespace(c);

        if((ret = dull_parse_value(c, &new_v)) != DULL_PARSE_OK)
            break;
            
        *(dull_value*)dull_context_push(c, sizeof(dull_value)) = new_v;
        // memcpy(dull_context_push(c, sizeof(dull_value)), &new_v, sizeof(dull_value));
        size++;

        dull_parse_whitespace(c);
        if(*c->json == ',')
        {
            c->json++;
        }
        else if(*c->json == ']')
        {   
            len = c->top - head;
            v->u.a.e = (dull_value*)malloc(len);
            memcpy(v->u.a.e,dull_context_pop(c, len), len);
            v->u.a.size = size;
            v->type = DULL_ARRAY;
            c->json++;
            return DULL_PARSE_OK;
        }
        else {
            ret = DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }

    for (int i = 0; i < size; i++)
        dull_free((dull_value*)dull_context_pop(c, sizeof(dull_value)));

    c->top = head;
    return ret;
}

size_t dull_get_array_size(dull_value* v)
{
    assert(v != NULL && v->type == DULL_ARRAY);
    return v->u.a.size;
}
dull_value* dull_get_array_element(dull_value* v, size_t index)
{
    assert(v != NULL && v->type == DULL_ARRAY && index >=0);
    if (index < v->u.a.size)
        return v->u.a.e + index;
    else
        return NULL;
}

int dull_parse(dull_value* v, const char* json)
{
    assert(v != NULL);

    dull_context c;
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    DULL_INIT(v);
    dull_parse_whitespace(&c);
    int ret;
    if((ret = dull_parse_value(&c, v)) == DULL_PARSE_OK)
    {
        dull_parse_whitespace(&c);
        if (*c.json != '\0')
        {   
            v->type = DULL_NULL;
            ret = DULL_PARSE_ROOT_NOT_SINGULAR;
        }
        
    }
    assert(c.top == 0);
    free(c.stack);

    return ret;
}

dull_type dull_get_type(const dull_value* v)
{
    assert(v != NULL);
    return v->type;
}