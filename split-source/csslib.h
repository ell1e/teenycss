#ifndef TEENYCSS_CSSLIB_H_
#define TEENYCSS_CSSLIB_H_

/* teenycss, a header-only compatible tiny CSS lib

  Copyright (C) 2020  Ellie June

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/


typedef struct teenycss_ruleset teenycss_ruleset;

teenycss_ruleset *teenycss_Parse(const char *contents);

int teenycss_ParseAdditional(
    teenycss_ruleset *rules, const char *contents
);


typedef struct teenycss_hashmap teenycss_hashmap;

#define TEENYCSS_ATTRIBUTEFILTERTYPE_ANY 0
#define TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHFULL 1
#define TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHBEGIN 2
#define TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHEND 3
#define TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHCONTAINS 4

typedef struct teenycss_attributeselector {
    int filtertype;
    char *value;
    char *name;
} teenycss_attributeselector;

typedef struct teenycss_filteritem {
    int attribute_selectors_count;
    teenycss_attributeselector *attribute_selectors;
} teenycss_filteritem;


typedef struct teenycss_rule {
    int filters_count;
    teenycss_filteritem **filters;

    teenycss_hashmap *attribute_values;
} teenycss_rule;

typedef struct teenycss_ruleset {
    int rules_count;
    teenycss_rule **rules;
} teenycss_ruleset;


void teenycss_Free(teenycss_ruleset *ruleset);

#endif  // TEENYCSS_CSSLIB_H_
