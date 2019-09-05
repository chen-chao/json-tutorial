#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <math.h>
#include <errno.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)  ((ch) >= '0' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

typedef struct {
  char *keyword;
  lept_type type;
} lept_keyword;

const lept_keyword NULL_TYPE = {"null", LEPT_NULL};
const lept_keyword FALSE_TYPE = {"false", LEPT_FALSE};
const lept_keyword TRUE_TYPE = {"true", LEPT_TRUE};

static int lept_parse_literal(lept_context* c, lept_value* v, const lept_keyword k) {
  char* word = k.keyword;
  int i;
  EXPECT(c, *word++);
  for (i = 0; (*word) != '\0'; i++, word++)
    if (c->json[i] != *word)
      return LEPT_PARSE_INVALID_VALUE;
  c->json += i;
  v->type = k.type;
  return LEPT_PARSE_OK;
}

static const char* lept_validate_number(const char* json) {
  const char* ch = json;
  if (*ch == '-')
    ch++;

  if (!ISDIGIT(*ch)) {
    return json;
  }

  if (*ch == '0') {
    ch++;
  } else {
    for (ch++; ISDIGIT(*ch); ch++)
      ;
  }

  // fraction
  if (*ch == '.') {
    // at least one digit after .
    ch++;
    if (!ISDIGIT(*ch)) {
      return json;
    }
    for (ch++; ISDIGIT(*ch); ch++)
      ;
  }

  // scientific notation
  if (*ch == 'e' || *ch == 'E') {
    ch++;
    if (*ch == '+' || *ch == '-') {
      ch++;
    }
    for (; ISDIGIT(*ch); ch++)
      ;
  }
  return ch;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
  const char* end = lept_validate_number(c->json);
  if (c->json == end)
    return LEPT_PARSE_INVALID_VALUE;

  errno = 0;
  v->n = strtod(c->json, NULL);
  if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
    return LEPT_PARSE_NUMBER_TOO_BIG;

  c->json = end;
  v->type = LEPT_NUMBER;
  return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, TRUE_TYPE);
    case 'f':  return lept_parse_literal(c, v, FALSE_TYPE);
    case 'n':  return lept_parse_literal(c, v, NULL_TYPE);
    default:   return lept_parse_number(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
