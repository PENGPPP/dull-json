#include "dulljson.h"

int dull_parse(dull_value* v, const char* json){
    return DULL_PARSE_OK;
}

dull_type dull_get_type(const dull_value* v){
    return v->type;
}