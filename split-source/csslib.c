
#include <assert.h>
#include <stdlib.h>
#include <string.h>


teenycss_ruleset *teenycss_Parse(const char *contents) {
    teenycss_ruleset *ruleset = malloc(sizeof(*ruleset));
    if (!ruleset)
        return NULL;
    memset(ruleset, 0, sizeof(*ruleset));

    if (!teenycss_ParseAdditional(ruleset, contents)) {
        teenycss_Free(ruleset);
        return NULL;
    }
    return ruleset;
}

static void teenycss_skipwhitespace(const char *contents, int *i) {
    while (contents[*i] == ' ' ||
            contents[*i] == '\t' ||
            contents[*i] == '\n' ||
            contents[*i] == '\r') {
        (*i)++;
    }
}

typedef struct teenycss_singlefilterchain {
    int filters_count;
    teenycss_filteritem **filters;
} teenycss_singlefilterchain;

static int _teenycss_FreeFilterItem_FreeValue(
        teenycss_hashmap *map, uint64_t number,
        void *userdata
        ) {
    teenycss_attributeselectorvalue *value = (
        (void*)(uintptr_t)number
    );
    if (value) {
        if (value->filtervalue)
            free(value->filtervalue);
        free(value);
    }
}

void teenycss_FreeFilterItem(teenycss_filteritem *filteritem) {
    if (!filteritem)
        return;
    if (filteritem->tagname)
        free(filteritem->tagname);
    if (filteritem->attribute_values) { 
        teenycss_hash_BytesMapIterate(
            filteritem->attribute_values,
            &_teenycss_FreeFilterItem_FreeValue,
            NULL
        );
        teenycss_hash_FreeMap(filteritem->attribute_values);
    }
}

void teenycss_FreeRule(teenycss_rule *rule) {
    if (!rule)
        return;
    int i = 0;
    while (i < rule->filters_count) {
        teenycss_FreeFilterItem(rule->filters[i]);
        i++;
    }
    if (rule->attribute_values) {
        // XXX: no need to free values, this is an STS map which does this
        teenycss_hash_FreeMap(rule->attribute_values);
    }
    free(rule);
}

teenycss_filteritem *teenycss_FilterItemParse(
        const char *contents, int *len
        ) {
    int i = 0;
    teenycss_skipwhitespace(contents, &i);

    teenycss_filteritem *newfilter = malloc(
        sizeof(*newfilter)
    );
    if (!newfilter)
        return NULL;
    memset(newfilter, 0, sizeof(*newfilter));

    char itembuf[2048];
    int itemlen = 0;
    while (contents[i] != '.' && contents[i] != '[' &&
            contents[i] != '#' && contents[i] != '\0') {
        if (itemlen + 1 < sizeof(itembuf)) {
            itembuf[itemlen + 1] = '\0';
            itembuf[itemlen] = contents[i];
        }
    }
    teenycss_skipwhitespace(contents, &i);
    if (itemlen > 0) {
        newfilter->tagname = strdup(itembuf);
        if (!newfilter->tagname) {
            teenycss_FreeFilterItem(newfilter):
            return NULL;
        }
    }
    while (contents[i] != '\0') {
        if (contents[i] == '.' || contents[i] == '#') {
            char typechar = contents[i];
            itemlen = 0;
            while (contents[i] != '.' && contents[i] != '[' &&
                    contents[i] != '#' && contents[i] != '\0') {
                if (itemlen + 1 < sizeof(itembuf)) {
                    itembuf[itemlen + 1] = '\0';
                    itembuf[itemlen] = contents[i];
                }
            }
            if (typechar == '.') {

            } else if (typechar == '#') {

            }
        } else {
            break;
        }
    }
    if (len)
        *len = i;
    return newfilter;
}

int teenycss_ParseAdditional(
        teenycss_ruleset *ruleset, const char *contents
        ) {
    int i = 0;
    teenycss_skipwhitespace(contents, &i);

    char itembuf[2048] = "";
    int itemlen = 0;
    int chains_count = 0;
    teenycss_singlefilterchain *chains;
    int currentchain_full = 0;
    teenycss_rule *current_rule = NULL;

    while (contents[i] != '\0') {
        while (contents[i] != ',' && contents[i] != ' ' &&
                contents[i] != '\t' && contents[i] != '\r' &&
                contents[i] != '\n' &&
                contents[i] != '{' && contents[i] != '\0') {
            if (itemlen + 1 < sizeof(itembuf)) {
                itembuf[itemlen + 1] = '\0';
                itembuf[itemlen] = contents[i];
                itemlen++;
            }
            i++;
        }
        teenycss_skipwhitespace(contents, &i);
        if (itemlen > 0) {
            if (chains_count == 0 || currentchain_full) {
                teenycss_singlefilterchain *new_chains = realloc(
                    chains, sizeof(*chains) * (chains_count + 1)
                );
                if (!new_chains) {
                    errorquit: ;
                    int k = 0;
                    while (k < chains_count) {
                        int z = 0;
                        while (z < chains[k].filters_count) {
                            teenycss_FreeFilterItem(
                                chains[k].filters[z]
                            );
                            z++;
                        }
                        if (chains[k].filters)
                            free(chains[k].filters);
                        k++;
                    }
                    if (chains) free(chains);
                    if (current_rule) teenycss_FreeRule(current_rule);
                    return 0;
                }
                chains = new_chains;
                memset(
                    &chains[chains_count],
                    0, sizeof(*chains)
                );
                chains_count++;
                currentchain_full = 0;
            }
            teenycss_singlefilterchain *current = (
                &chains[chains_count - 1]
            );
            teenycss_filteritem *newfilteritem = (
                teenycss_FilterItemParse(itembuf, &filteritemlen)
            );
            if (!newfilteritem)
                goto errororquit;

            teenycss_filteritem *new_filters = realloc(
                current->filters,
                sizeof(current->filters) * (current->filters_count + 1)
            );
            if (!new_filters) {
                free(newfilter);
                goto errororquit;
            }
            current->filters = new_filters;
            current->filters[current->filters_count] = newfilteritem;
            current->filters_count += 1;
        }
        itemlen = 0;
        itembuf[0] = '\0';
        if (contents[i] == '\0')
            break;
        if (contents[i] == ',') {
            currentchain_full = 1;
            continue;
        }
        if (contents[i] != '{')
            continue;
        assert(contents[i] == '{');
        if (chains_count == 0) {
            // Just skip the next rule:
            while (contents[i] != '}' && contents[i] != '\0') {
                teenycss_skipwhitespace(contents, &i)
                i++;
            }
            continue;
        }
        // Now parse the current rule entry:
        if (!current_rule) {
            current_rule = malloc(sizeof(*current_rule));
            if (!current_rule)
                goto errororquit;
            memset(current_rule, 0, sizeof(*current_rule));
        }
        while (contents[i] != '}' && contents[i] != '\0') {
            teenycss_skipwhitespace(contents, &i);
            itemlen = 0;
            itembuf[0] = '\0';
            while (contents[i] != ':'' && contents[i] != ';' &&
                    contents[i] != '}' && contents[i] != '\0') {
                if (itemlen + 1 < sizeof(itembuf)) {
                    itembuf[itemlen + 1] = '\0';
                    itembuf[itemlen] = contents[i];
                    itemlen++;
                }
                i++;
            }
            if (contents[i] == ';') {
                i++;
                continue;
            }
            if (contents[i] != ':') break;
            char item2buf[2048] = "";
            int item2len = 0;
            teenycss_skipwhitespace(contents, &i);
            while (contents[i] != ';' &&
                    contents[i] != '}' && contents[i] != '\0') {
                if (item2len + 1 < sizeof(item2buf)) {
                    item2buf[item2len + 1] = '\0';
                    item2buf[item2len] = contents[i];
                    item2len++;
                }
                i++;
            }
            if (itemlen > 0 && item2len > 0) {
                if (!current_rule->attribute_values) {
                    current_rule->attribute_values = (
                        teenycss_hash_NewStringToStringMap(
                            teenycss_hash_NewStringMap(64)
                        ));
                    if (!current_rule->attribute_values)
                        goto errororquit;
                }
                if (!teenycss_hash_STSMapSet(
                        current_rule->attribute_values,
                        itembuf, item2buf))
                    goto errororquit;
                
            }
        }
        itemlen = 0;
        itembuf[0] = '\0';
        if (contents == '}') {
            i++;
        }
        
    }
    return 1;
}

char *teenycss_Dump(teenycss_ruleset *ruleset) {
    
}

void teenycss_Free(teenycss_ruleset *ruleset) {
    free(ruleset);
}
