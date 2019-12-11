#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dulljson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_null() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_boolean(&v, 0);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "null"));
    EXPECT_EQ_INT(DULL_NULL, dull_get_type(&v));
    dull_free(&v);
}

static void test_parse_true() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_boolean(&v, 0);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "true"));
    EXPECT_EQ_INT(DULL_TRUE, dull_get_type(&v));
    dull_free(&v);
}

static void test_parse_false() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_boolean(&v, 1);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "false"));
    EXPECT_EQ_INT(DULL_FALSE, dull_get_type(&v));
    dull_free(&v);
}

#define TEST_NUMBER(expect, json)\
    do {\
        dull_value v;\
        DULL_INIT(&v);\
        EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, json));\
        EXPECT_EQ_INT(DULL_NUMBER, dull_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, dull_get_number(&v));\
        dull_free(&v);\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        dull_value v;\
        DULL_INIT(&v);\
        EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, json));\
        EXPECT_EQ_INT(DULL_STRING, dull_get_type(&v));\
        EXPECT_EQ_STRING(expect, dull_get_string(&v), dull_get_string_length(&v));\
        dull_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
    size_t i, j;
    dull_value v;

    DULL_INIT(&v);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "[ ]"));
    EXPECT_EQ_INT(DULL_ARRAY, dull_get_type(&v));
    EXPECT_EQ_SIZE_T(0, dull_get_array_size(&v));
    dull_free(&v);

    DULL_INIT(&v);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(DULL_ARRAY, dull_get_type(&v));
    EXPECT_EQ_SIZE_T(5, dull_get_array_size(&v));
    EXPECT_EQ_INT(DULL_NULL,   dull_get_type(dull_get_array_element(&v, 0)));
    EXPECT_EQ_INT(DULL_FALSE,  dull_get_type(dull_get_array_element(&v, 1)));
    EXPECT_EQ_INT(DULL_TRUE,   dull_get_type(dull_get_array_element(&v, 2)));
    EXPECT_EQ_INT(DULL_NUMBER, dull_get_type(dull_get_array_element(&v, 3)));
    EXPECT_EQ_INT(DULL_STRING, dull_get_type(dull_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, dull_get_number(dull_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", dull_get_string(dull_get_array_element(&v, 4)), dull_get_string_length(dull_get_array_element(&v, 4)));
    dull_free(&v);

    DULL_INIT(&v);
    EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(DULL_ARRAY, dull_get_type(&v));
    EXPECT_EQ_SIZE_T(4, dull_get_array_size(&v));
    for (i = 0; i < 4; i++) {
        dull_value* a = dull_get_array_element(&v, i);
        EXPECT_EQ_INT(DULL_ARRAY, dull_get_type(a));
        EXPECT_EQ_SIZE_T(i, dull_get_array_size(a));
        for (j = 0; j < i; j++) {
            dull_value* e = dull_get_array_element(a, j);
            EXPECT_EQ_INT(DULL_NUMBER, dull_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, dull_get_number(e));
        }
    }
    dull_free(&v);
}

#define TEST_ERROR(error, json)\
    do {\
        dull_value v;\
        DULL_INIT(&v);\
        v.type = DULL_FALSE;\
        EXPECT_EQ_INT(error, dull_parse(&v, json));\
        EXPECT_EQ_INT(DULL_NULL, dull_get_type(&v));\
        dull_free(&v);\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(DULL_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(DULL_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "nan");

    /* invalid value in array */
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(DULL_PARSE_INVALID_VALUE, "[\"a\", nul]");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(DULL_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(DULL_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(DULL_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(DULL_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(DULL_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(DULL_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_miss_quotation_mark() {
    TEST_ERROR(DULL_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(DULL_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(DULL_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(DULL_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(DULL_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(DULL_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(DULL_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(DULL_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(DULL_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(DULL_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
}

static void test_access_null() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_string(&v, "a", 1);
    dull_set_null(&v);
    EXPECT_EQ_INT(DULL_NULL, dull_get_type(&v));
    dull_free(&v);
}

static void test_access_boolean() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_string(&v, "a", 1);

    dull_set_boolean(&v, 1);
    EXPECT_TRUE(dull_get_boolean(&v));
    dull_set_boolean(&v, 0);
    EXPECT_FALSE(dull_get_boolean(&v));
    dull_free(&v);
}

static void test_access_number() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_string(&v, "a", 1);
    dull_set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, dull_get_number(&v));
    dull_free(&v);
}

static void test_access_string() {
    dull_value v;
    DULL_INIT(&v);
    dull_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", dull_get_string(&v), dull_get_string_length(&v));
    dull_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", dull_get_string(&v), dull_get_string_length(&v));
    dull_free(&v);
}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    // char* expect = "Hello\nWorld";
    // char* json = "\"Hello\\nWorld\"";
    // dull_value v;
    // DULL_INIT(&v);
    // EXPECT_EQ_INT(DULL_PARSE_OK, dull_parse(&v, json));
    // EXPECT_EQ_INT(DULL_STRING, dull_get_type(&v));
    // EXPECT_EQ_STRING(expect, dull_get_string(&v), dull_get_string_length(&v));
    // dull_free(&v);
    // return 0;
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}