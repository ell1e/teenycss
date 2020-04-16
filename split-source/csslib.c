
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


void teenycss_FreeFilterItem(teenycss_filteritem *filteritem) {
    if (!filteritem)
        return;
    if (filteritem->attribute_selectors) { 
        int i = 0;
        while (i < filteritem->attribute_selectors_count) {
            if (filteritem->attribute_selectors[i].value)
                free(filteritem->attribute_selectors[i].value);
            if (filteritem->attribute_selectors[i].name)
                free(filteritem->attribute_selectors[i].name);
            i++;
        }
        free(filteritem->attribute_selectors);
    }
}

void teenycss_FreeRule(teenycss_rule *rule) {
    if (!rule)
        return;
    int i = 0;
    while (i < rule->filters_count) {
        if (rule->filters[i])
            teenycss_FreeFilterItem(rule->filters[i]);
        i++;
    }
    if (rule->attribute_values) {
        // XXX: no need to free values, this is an STS map which does this
        teenycss_hash_FreeMap(rule->attribute_values);
    }
    free(rule);
}

int _teenycss_DuplicateRule_AttrIterCopy(
        teenycss_hashmap *map, const char *key, const char *value,
        void *ud) {
    teenycss_hashmap *new_map = ud;
    return teenycss_hash_STSMapSet(new_map, key, value);
}

teenycss_rule *teenycss_DuplicateRule(teenycss_rule *rule) {
    if (!rule)
        return NULL;
    if (rule->filters_count > 0)
        return NULL;  // FIXME: handle this.
    teenycss_rule *copyrule = malloc(sizeof(*copyrule));
    if (!copyrule)
        return NULL;
    memset(copyrule, 0, sizeof(*copyrule));
    if (rule->attribute_values) {
        copyrule->attribute_values = (
            teenycss_hash_NewStringToStringMap(64)
        );
        if (!copyrule->attribute_values) {
            teenycss_FreeRule(copyrule);
            return NULL;
        }
        if (!teenycss_hash_STSMapIterate(
                rule->attribute_values,
                &_teenycss_DuplicateRule_AttrIterCopy,
                copyrule->attribute_values)) {
            teenycss_FreeRule(copyrule);
            return NULL;
        }
    }
    return copyrule;
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
    newfilter->attribute_selectors = NULL;
    newfilter->attribute_selectors_count = 0;

    int itemlen = 0;
    char itembuf[2048] = "";

    teenycss_skipwhitespace(contents, &i);
    while (contents[i] != '\0') {
        int orig_i = i;
        int nameset = 0;
        int filtertype = TEENYCSS_ATTRIBUTEFILTERTYPE_MATCHFULL;
        char *name = NULL;
        if (contents[i] == '.') {
            nameset = 1;
            name = strdup("class");
            i++;
        } else if (contents[i] == '#') {
            nameset = 1;
            name = strdup("id");
            i++;
        } else if (contents[i] != ':' && contents[i] != '[') {
            nameset = 1;
            name = strdup("tag");
        }
        if (!name && nameset) {
            teenycss_FreeFilterItem(newfilter);
            return NULL;
        }
        itemlen = 0;
        while (contents[i] != '.' && contents[i] != '[' &&
                contents[i] != '#' && contents[i] != '\0' &&
                contents[i] != ':') {
            if (itemlen + 1 < sizeof(itembuf)) {
                itembuf[itemlen + 1] = '\0';
                itembuf[itemlen] = contents[i];
            }
            i++;
        }
        while (itemlen > 0 && (
                itembuf[itemlen - 1] == ' ' ||
                itembuf[itemlen - 1] == '\t' ||
                itembuf[itemlen - 1] == '\r' ||
                itembuf[itemlen - 1] == '\n')) {
            itemlen--;
            itembuf[itemlen] = '\0';
        }
        if (!name && itemlen == 1 && strcmp(itembuf, "*") == 0) {
            name = strdup("tag");
            if (!name) {
                teenycss_FreeFilterItem(newfilter);
                return NULL;
            }
            filtertype = TEENYCSS_ATTRIBUTEFILTERTYPE_ANY;
        }
        if (name && itemlen > 0) {
            teenycss_attributeselector *new_selectors = realloc(
                newfilter->attribute_selectors,
                sizeof(*newfilter->attribute_selectors) *
                (newfilter->attribute_selectors_count + 1)
            );
            if (!new_selectors) {
                teenycss_FreeFilterItem(newfilter);
                return NULL;
            }
            newfilter->attribute_selectors = new_selectors;
            memset(
                &newfilter->attribute_selectors[
                    newfilter->attribute_selectors_count
                ], 0, sizeof(*newfilter->attribute_selectors)
            );
            newfilter->attribute_selectors_count++;
            newfilter->attribute_selectors[
                newfilter->attribute_selectors_count
            ].name = strdup(name);
            if (!newfilter->attribute_selectors[
                    newfilter->attribute_selectors_count
                    ].name) {
                teenycss_FreeFilterItem(newfilter);
                return NULL;
            }
            if (filtertype != TEENYCSS_ATTRIBUTEFILTERTYPE_ANY) {
                newfilter->attribute_selectors[
                    newfilter->attribute_selectors_count
                ].value = strdup(itembuf);
                if (!newfilter->attribute_selectors[
                        newfilter->attribute_selectors_count
                        ].value) {
                    teenycss_FreeFilterItem(newfilter);
                    return NULL;
                }
            }
            newfilter->attribute_selectors[
                newfilter->attribute_selectors_count
            ].filtertype = filtertype;
        } else if (orig_i == i && contents[i] != '\0') {
            i++;
            teenycss_skipwhitespace(contents, &i);
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
    teenycss_singlefilterchain *chains = NULL;
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
            int filteritemlen = 0;
            teenycss_filteritem *newfilteritem = (
                teenycss_FilterItemParse(itembuf, &filteritemlen)
            );
            if (!newfilteritem)
                goto errorquit;

            teenycss_filteritem **new_filters = realloc(
                current->filters,
                sizeof(current->filters) * (current->filters_count + 1)
            );
            if (!new_filters) {
                teenycss_FreeFilterItem(newfilteritem);
                goto errorquit;
            }
            current->filters = new_filters;
            current->filters[current->filters_count] = newfilteritem;
            current->filters_count += 1;
        }
        itemlen = 0;
        itembuf[0] = '\0';
        teenycss_skipwhitespace(contents, &i);
        if (contents[i] == '\0')
            break;
        if (contents[i] == ',') {
            if (chains_count <= 0 ||
                    chains[chains_count - 1].filters_count > 0
                    )
                currentchain_full = 1;
            i++;
            continue;
        }
        if (contents[i] != '{')
            continue;
        assert(contents[i] == '{');
        i++;  // skip '{'
        if (chains_count == 0) {
            // Just skip the next rule:
            while (contents[i] != '}' && contents[i] != '\0') {
                teenycss_skipwhitespace(contents, &i);
                i++;
            }
            continue;
        }
        // Now parse the current rule entry's rules:
        if (!current_rule) {
            current_rule = malloc(sizeof(*current_rule));
            if (!current_rule)
                goto errorquit;
            memset(current_rule, 0, sizeof(*current_rule));
        }
        while (contents[i] != '}' && contents[i] != '\0') {
            teenycss_skipwhitespace(contents, &i);
            itemlen = 0;
            itembuf[0] = '\0';
            while (contents[i] != ':' && contents[i] != ';' &&
                    contents[i] != '}' && contents[i] != '\0') {
                if (itemlen + 1 < sizeof(itembuf)) {
                    itembuf[itemlen + 1] = '\0';
                    itembuf[itemlen] = contents[i];
                    itemlen++;
                }
                i++;
            }
            teenycss_skipwhitespace(contents, &i);
            if (contents[i] == ';') {
                i++;
                continue;
            }
            if (contents[i] != ':') break;
            i++;  // skip ':'
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
                        teenycss_hash_NewStringToStringMap(64)
                    );
                    if (!current_rule->attribute_values)
                        goto errorquit;
                }
                if (!teenycss_hash_STSMapSet(
                        current_rule->attribute_values,
                        itembuf, item2buf))
                    goto errorquit;
                
            }
        }
        itemlen = 0;
        itembuf[0] = '\0';
        teenycss_skipwhitespace(contents, &i);
        int j = 0;
        while (j < chains_count) {
            teenycss_rule **newrules = realloc(
                ruleset->rules,
                sizeof(*ruleset->rules) * (ruleset->rules_count + 1)
            );
            if (!newrules) {
                if (j > 0)
                    current_rule = NULL;  // owned by ruleset
                goto errorquit;
            }
            ruleset->rules = newrules;
            if (j == 0) {
                current_rule->filters = chains[j].filters;
                current_rule->filters_count = chains[j].filters_count;
                chains[j].filters = NULL;
                chains[j].filters_count = 0;
                ruleset->rules[ruleset->rules_count] = current_rule;
            } else {
                teenycss_filteritem **oldfilters = current_rule->filters;
                int oldfilterscount = current_rule->filters_count;
                current_rule->filters = NULL;
                current_rule->filters_count = 0;
                teenycss_rule *dup_rule = teenycss_DuplicateRule(
                    current_rule
                );
                current_rule->filters = oldfilters;
                current_rule->filters_count = oldfilterscount;
                if (!dup_rule) {
                    current_rule = NULL;  // owned by ruleset
                    goto errorquit;
                }
                dup_rule->filters = chains[j].filters;
                dup_rule->filters_count = chains[j].filters_count;
                chains[j].filters = NULL;
                chains[j].filters_count = 0;
                ruleset->rules[ruleset->rules_count] = dup_rule;
            }
            ruleset->rules_count++;
            j++;
        }
        current_rule = NULL;  // now owned by ruleset.
        if (contents[i] == '}') {
            i++;
        }
        
    }
    if (chains) {
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
        free(chains);
    }
    if (current_rule) teenycss_FreeRule(current_rule);
    return 1;
}

char *teenycss_Dump(teenycss_ruleset *ruleset) {
    if (!ruleset)
        return NULL;

    return NULL;
}

void teenycss_Free(teenycss_ruleset *ruleset) {
    if (!ruleset)
        return;
    int i = 0;
    while (i < ruleset->rules_count) {
        if (ruleset->rules[i])
            teenycss_FreeRule(ruleset->rules[i]);
        i++;
    }
    if (ruleset->rules)
        free(ruleset->rules);
    free(ruleset);
}
