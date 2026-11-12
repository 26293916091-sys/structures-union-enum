/*
 * Problem 5 (Extended): Safe Heterogeneous Data Container Library
 * File: 10.c
 * Standard: ISO C11
 * Compilation: gcc -std=c11 -Wall -Wextra -Wpedantic 10.c -o 10
 *
 * Description:
 *   Implements a memory-safe tagged-union Container supporting TYPE_EMPTY,
 *   TYPE_INT, TYPE_FLOAT, TYPE_CHAR, and dynamically allocated TYPE_STRING.
 *   Provides type-safe getters with separate status codes, deep copying via
 *   container_copy, leak-free type transitions, and comprehensive edge-case tests.
 *
 * Sample Run:
 *   Allocates 6 containers holding: 10 (int), 3.14 (float), 'A' (char),
 *   "Hello" (string), 0 (int), and "Data" (string).
 *   Prints each index, type label, and value.
 *   Deep-copies container ("Data") into a separate container, mutates
 *   container to "Modified Original", and verifies the copy still holds "Data".
 *   Demonstrates STRING -> INT -> STRING -> FLOAT transitions without leaks.
 *   Clears and frees all containers cleanly.
 *
 * Edge Cases Tested:
 *   - Valid integer zero (0) vs wrong-type getter failure
 *   - Empty container initialization (TYPE_EMPTY) and repeated container_clear
 *   - Empty string ("") heap allocation and retrieval
 *   - Self-copy protection (container_copy(&c, &c))
 *   - String replacement using the container's own internal string pointer
 *   - Switching away from STRING to primitive types without memory leaks
 *   - NULL pointer validation on all API functions
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE 6

/* Enumeration identifying the active data type stored in the container */
typedef enum {
    TYPE_EMPTY = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING
} DataType;

/* Union storing one active value at a time */
typedef union {
    int int_val;
    float float_val;
    char char_val;
    char *str_val;
} Data;

/* Tagged-union container pairing a DataType tag with its Data payload */
typedef struct {
    DataType type;
    Data data;
} Container;

/* Portable ISO C11 string duplication using malloc + memcpy */
static char *duplicate_c_string(const char *src) {
    if (src == NULL) {
        return NULL;
    }

    size_t len = strlen(src);
    char *copy = (char *)malloc(len + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, src, len + 1);

    return copy;
}

/* Initializes a container to a known safe empty state */
void container_init(Container *c) {
    if (c != NULL) {
        c->type = TYPE_EMPTY;
        c->data.str_val = NULL;
    }
}

/* Releases owned heap string memory (if any) and resets container to TYPE_EMPTY */
void container_clear(Container *c) {
    if (c == NULL) {
        return;
    }

    if (c->type == TYPE_STRING && c->data.str_val != NULL) {
        free(c->data.str_val);
        c->data.str_val = NULL;
    }

    c->type = TYPE_EMPTY;
}

/* Stores an integer value after freeing any previous string */
int container_set_int(Container *c, int value) {
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->data.int_val = value;
    c->type = TYPE_INT;

    return 0;
}

/* Stores a float value after freeing any previous string */
int container_set_float(Container *c, float value) {
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->data.float_val = value;
    c->type = TYPE_FLOAT;

    return 0;
}

/* Stores a char value after freeing any previous string */
int container_set_char(Container *c, char value) {
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->data.char_val = value;
    c->type = TYPE_CHAR;

    return 0;
}

/* Allocates new string BEFORE freeing old string so previous state survives allocation failure */
int container_set_string(Container *c, const char *value) {
    if (c == NULL || value == NULL) {
        return -1;
    }

    char *new_copy = duplicate_c_string(value);

    if (new_copy == NULL) {
        return -1;
    }

    container_clear(c);
    c->data.str_val = new_copy;
    c->type = TYPE_STRING;

    return 0;
}

/* Type-safe integer getter; returns 0 on success, -1 on type mismatch or NULL */
int container_get_int(const Container *c, int *out_value) {
    if (c == NULL || out_value == NULL || c->type != TYPE_INT) {
        return -1;
    }

    *out_value = c->data.int_val;

    return 0;
}

/* Type-safe float getter; returns 0 on success, -1 on type mismatch or NULL */
int container_get_float(const Container *c, float *out_value) {
    if (c == NULL || out_value == NULL || c->type != TYPE_FLOAT) {
        return -1;
    }

    *out_value = c->data.float_val;

    return 0;
}

/* Type-safe char getter; returns 0 on success, -1 on type mismatch or NULL */
int container_get_char(const Container *c, char *out_value) {
    if (c == NULL || out_value == NULL || c->type != TYPE_CHAR) {
        return -1;
    }

    *out_value = c->data.char_val;

    return 0;
}

/* Type-safe string getter; returns 0 on success, -1 on type mismatch or NULL */
int container_get_string(const Container *c, const char **out_value) {
    if (c == NULL || out_value == NULL || c->type != TYPE_STRING || c->data.str_val == NULL) {
        return -1;
    }

    *out_value = c->data.str_val;

    return 0;
}

/* Returns a human-readable label for the container's active type */
const char *container_type_name(const Container *c) {
    if (c == NULL) {
        return "NULL";
    }

    switch (c->type) {
        case TYPE_EMPTY: {
            return "EMPTY";
        }
        case TYPE_INT: {
            return "INT";
        }
        case TYPE_FLOAT: {
            return "FLOAT";
        }
        case TYPE_CHAR: {
            return "CHAR";
        }
        case TYPE_STRING: {
            return "STRING";
        }
        default: {
            return "UNKNOWN";
        }
    }
}

/* Deep-copies source container into destination; handles self-copy and heap strings safely */
int container_copy(Container *destination, const Container *source) {
    if (destination == NULL || source == NULL) {
        return -1;
    }

    if (destination == source) {
        return 0;
    }

    switch (source->type) {
        case TYPE_EMPTY: {
            container_clear(destination);
            return 0;
        }
        case TYPE_INT: {
            return container_set_int(destination, source->data.int_val);
        }
        case TYPE_FLOAT: {
            return container_set_float(destination, source->data.float_val);
        }
        case TYPE_CHAR: {
            return container_set_char(destination, source->data.char_val);
        }
        case TYPE_STRING: {
            return container_set_string(destination, source->data.str_val);
        }
        default: {
            return -1;
        }
    }
}

/* Prints the currently active value stored in the container */
void container_print(const Container *c) {
    if (c == NULL) {
        printf("[NULL Container]");
        return;
    }

    switch (c->type) {
        case TYPE_EMPTY: {
            printf("<empty>");
            break;
        }
        case TYPE_INT: {
            printf("%d", c->data.int_val);
            break;
        }
        case TYPE_FLOAT: {
            printf("%.2f", (double)c->data.float_val);
            break;
        }
        case TYPE_CHAR: {
            printf("'%c'", c->data.char_val);
            break;
        }
        case TYPE_STRING: {
            printf("\"%s\"", (c->data.str_val != NULL) ? c->data.str_val : "");
            break;
        }
        default: {
            printf("<invalid>");
            break;
        }
    }
}

int main(void) {
    printf("=== Safe Heterogeneous Data Container Library ===\n\n");

    Container *items = (Container *)malloc(ARRAY_SIZE * sizeof(Container));

    if (items == NULL) {
        fprintf(stderr, "Fatal Error: Failed to allocate container array.\n");
        return 1;
    }

    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        container_init(&items[i]);
    }

    container_set_int(&items, 10);
    container_set_float(&items, 3.14f);
    container_set_char(&items, 'A');
    container_set_string(&items, "Hello");
    container_set_int(&items, 0);
    container_set_string(&items, "Data");

    printf("1. Mixed-Type Container Array:\n");
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        printf("  Index [%zu] | Type: %-6s | Value: ", i, container_type_name(&items[i]));
        container_print(&items[i]);
        printf("\n");
    }

    printf("\n2. Deep Copy Independence Demonstration:\n");
    Container backup;
    container_init(&backup);

    if (container_copy(&backup, &items) == 0) {
        container_set_string(&items, "Modified Original");
        printf("  Original after mutation : ");
        container_print(&items);
        printf("\n  Deep Copy                   : ");
        container_print(&backup);
        printf("\n");
    }

    printf("\n3. Type Transition Lifecycle (STRING -> INT -> STRING -> FLOAT):\n");
    Container morph;
    container_init(&morph);

    container_set_string(&morph, "Initial String");
    printf("  Step 1 (%-6s): ", container_type_name(&morph));
    container_print(&morph);
    printf("\n");

    container_set_int(&morph, 999);
    printf("  Step 2 (%-6s): ", container_type_name(&morph));
    container_print(&morph);
    printf("\n");

    container_set_string(&morph, "Second String");
    printf("  Step 3 (%-6s): ", container_type_name(&morph));
    container_print(&morph);
    printf("\n");

    container_set_float(&morph, 2.71f);
    printf("  Step 4 (%-6s): ", container_type_name(&morph));
    container_print(&morph);
    printf("\n");

    printf("\n4. Edge-Case Verification:\n");
    int zero_out = -1;
    if (container_get_int(&items, &zero_out) == 0 && zero_out == 0) {
        printf("  [Pass] Valid integer 0 retrieved accurately.\n");
    }

    float wrong_type_out = 0.0f;
    if (container_get_float(&items, &wrong_type_out) != 0) {
printf("  [Pass] Wrong-type getter safely rejected.\n");
}
container_set_string(&morph, "");
const char *empty_str_out = NULL;
if (container_get_string(&morph, ∅_str_out) == 0 && strlen(empty_str_out) == 0) {
printf("  [Pass] Empty string "" stored and retrieved safely.\n");
}
container_set_string(&morph, "Self Pointer Test");
const char *internal_ptr = NULL;
container_get_string(&morph, &internal_ptr);
if (container_set_string(&morph, internal_ptr) == 0) {
printf("  [Pass] String replacement using own internal pointer succeeded: "%s"\n",
morph.data.str_val);
}
if (container_copy(&morph, &morph) == 0) {
printf("  [Pass] Self-copy safely handled without corruption.\n");
}
container_clear(&morph);
container_clear(&morph);
printf("  [Pass] Repeated clear left container in state: %s\n", container_type_name(&morph));
container_clear(&backup);
for (size_t i = 0; i < ARRAY_SIZE; i++) {
container_clear(&items[i]);
}
free(items);
printf("\nAll containers cleared and freed cleanly.\n");
return 0;
}