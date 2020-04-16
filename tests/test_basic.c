
#define TEENYCSS_IMPLEMENTATION
#include "teenycss.h"

#include "testmain.h"

START_TEST(basic)
{
    teenycss_ruleset *ruleset = teenycss_Parse(
        "widget1, abc.widget2 {width:50px;}"
    );
    assert(ruleset != NULL);
    assert(ruleset->rules_count == 2);
    teenycss_rule **rules = ruleset->rules;
    assert(rules[0]->filters_count == 1);
    assert(rules[0]->filters[0]->attribute_selectors_count == 1);
    assert(rules[0]->filters[0]->attribute_selectors[0].filtertype ==
           TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHFULL);
    assert(strcmp(
        rules[0]->filters[0]->attribute_selectors[0].name,
        "tag"
    ) == 0);
    assert(strcmp(
        rules[0]->filters[0]->attribute_selectors[0].value,
        "widget1"
    ) == 0);
    assert(rules[1]->filters_count == 1);
    assert(rules[1]->filters[0]->attribute_selectors_count == 2);
    assert(rules[1]->filters[0]->attribute_selectors[0].filtertype ==
           TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHFULL);
    assert(strcmp(
        rules[1]->filters[0]->attribute_selectors[0].name,
        "tag"
    ) == 0);
    assert(strcmp(
        rules[1]->filters[0]->attribute_selectors[0].value,
        "abc"
    ) == 0);
    assert(rules[1]->filters[0]->attribute_selectors[1].filtertype ==
           TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHFULL);
    assert(strcmp(
        rules[1]->filters[0]->attribute_selectors[1].name,
        "class"
    ) == 0);
    assert(strcmp(
        rules[1]->filters[0]->attribute_selectors[1].value,
        "widget2"
    ) == 0);
}
END_TEST

TESTS_MAIN(basic)
