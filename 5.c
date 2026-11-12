// Structures - 2 (Extended: Rectangle Inventory and Geometry Analysis)
// Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 5.c -o 5

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECTS 1000
#define EPSILON 1e-5f

typedef struct {
    int id;
    float width;
    float height;
    float area;
    float perimeter;
} Rectangle;

void clear_input_buffer(void) {
    int ch = 0;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

void compute_rectangle_metrics(Rectangle *rect) {
    rect->area = rect->width * rect->height;
    rect->perimeter = 2.0f * (rect->width + rect->height);
}

bool is_square(const Rectangle *rect) {
    float diff = fabsf(rect->width - rect->height);
    float max_side = (rect->width > rect->height) ? rect->width : rect->height;

    return diff <= (EPSILON * max_side);
}

Rectangle *create_rectangle_list(int n) {
    if (n <= 0 || n > MAX_RECTS) {
        return NULL;
    }

    return (Rectangle *)malloc((size_t)n * sizeof(Rectangle));
}

bool resize_rectangle_list(Rectangle **rects, int *capacity) {
    int new_cap = (*capacity == 0) ? 4 : (*capacity * 2);

    if (new_cap <= *capacity || (size_t)new_cap > SIZE_MAX / sizeof(Rectangle)) {
        printf("Error: Capacity overflow prevented.\n");
        return false;
    }

    Rectangle *temp = (Rectangle *)realloc(*rects, (size_t)new_cap * sizeof(Rectangle));
    if (temp == NULL) {
        printf("Error: Memory reallocation failed.\n");
        return false;
    }

    *rects = temp;
    *capacity = new_cap;

    return true;
}

int find_rectangle_index(const Rectangle *rects, int size, int id) {
    for (int i = 0; i < size; i++) {
        if (rects[i].id == id) {
            return i;
        }
    }

    return -1;
}

void set_rectangle(Rectangle *rects, int i, int id, float width, float height) {
    rects[i].id = id;
    rects[i].width = width;
    rects[i].height = height;
    compute_rectangle_metrics(&rects[i]);
}

bool read_positive_dimensions(float *w_out, float *h_out) {
    printf("Enter width: ");
    if (scanf("%f", w_out) != 1 || !isfinite(*w_out) || *w_out <= 0.0f) {
        clear_input_buffer();
        printf("Error: Width must be a finite positive number.\n");
        return false;
    }
    clear_input_buffer();

    printf("Enter height: ");
    if (scanf("%f", h_out) != 1 || !isfinite(*h_out) || *h_out <= 0.0f) {
        clear_input_buffer();
        printf("Error: Height must be a finite positive number.\n");
        return false;
    }
    clear_input_buffer();

    return true;
}

void display_rectangles(const Rectangle *rects, int size) {
    if (size == 0) {
        printf("No rectangles in inventory.\n");
        return;
    }

    for (int i = 0; i < size; i++) {
        printf("ID: %d | %.2f x %.2f | Area: %.2f | Perimeter: %.2f | %s\n",
               rects[i].id,
               (double)rects[i].width,
               (double)rects[i].height,
               (double)rects[i].area,
               (double)rects[i].perimeter,
               is_square(&rects[i]) ? "Square" : "Non-Square");
    }
}

void search_rectangle(const Rectangle *rects, int size) {
    int id = 0;

    if (size == 0) {
        printf("Inventory is empty.\n");
        return;
    }

    printf("Enter Rectangle ID to search: ");
    if (scanf("%d", &id) != 1) {
        clear_input_buffer();
        printf("Error: Invalid ID.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_rectangle_index(rects, size, id);
    if (idx == -1) {
        printf("Rectangle ID %d not found.\n", id);
        return;
    }

    printf("Found -> ID: %d | %.2f x %.2f | Area: %.2f | Perimeter: %.2f | %s\n",
           rects[idx].id,
           (double)rects[idx].width,
           (double)rects[idx].height,
           (double)rects[idx].area,
           (double)rects[idx].perimeter,
           is_square(&rects[idx]) ? "Square" : "Non-Square");
}

void update_rectangle(Rectangle *rects, int size) {
    int id = 0;
    float w = 0.0f;
    float h = 0.0f;

    if (size == 0) {
        printf("Inventory is empty.\n");
        return;
    }

    printf("Enter Rectangle ID to update: ");
    if (scanf("%d", &id) != 1) {
        clear_input_buffer();
        printf("Error: Invalid ID.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_rectangle_index(rects, size, id);
    if (idx == -1) {
        printf("Rectangle ID %d not found.\n", id);
        return;
    }

    if (read_positive_dimensions(&w, &h)) {
        set_rectangle(rects, idx, id, w, h);
        printf("Updated Rectangle ID %d.\n", id);
    }
}

void delete_rectangle(Rectangle *rects, int *size) {
    int id = 0;

    if (*size == 0) {
        printf("Inventory is empty.\n");
        return;
    }

    printf("Enter Rectangle ID to delete: ");
    if (scanf("%d", &id) != 1) {
        clear_input_buffer();
        printf("Error: Invalid ID.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_rectangle_index(rects, *size, id);
    if (idx == -1) {
        printf("Rectangle ID %d not found.\n", id);
        return;
    }

    for (int i = idx; i < *size - 1; i++) {
        rects[i] = rects[i + 1];
    }

    (*size)--;
    printf("Deleted Rectangle ID %d.\n", id);
}

void display_sorted_and_filtered(const Rectangle *rects, int size) {
    if (size == 0) {
        printf("Inventory is empty.\n");
        return;
    }

    Rectangle *copy = (Rectangle *)malloc((size_t)size * sizeof(Rectangle));
    if (copy == NULL) {
        printf("Error: Allocation failed.\n");
        return;
    }

    memcpy(copy, rects, (size_t)size * sizeof(Rectangle));

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            bool swap = false;

            if (copy[j].area > copy[j + 1].area) {
                swap = true;
            } else if (copy[j].area == copy[j + 1].area && copy[j].id > copy[j + 1].id) {
                swap = true;
            }

            if (swap) {
                Rectangle temp = copy[j];
                copy[j] = copy[j + 1];
                copy[j + 1] = temp;
            }
        }
    }

    printf("--- Rectangles Sorted by Area (Ascending) ---\n");
    display_rectangles(copy, size);

    float min_area = 0.0f;
    printf("Enter minimum area threshold to filter: ");
    if (scanf("%f", &min_area) == 1 && isfinite(min_area)) {
        clear_input_buffer();
        printf("--- Rectangles with Area > %.2f ---\n", (double)min_area);
        for (int i = 0; i < size; i++) {
            if (copy[i].area > min_area) {
                printf("ID: %d | Area: %.2f\n", copy[i].id, (double)copy[i].area);
            }
        }
    } else {
        clear_input_buffer();
        printf("Error: Invalid threshold.\n");
    }

    free(copy);
}

void display_area_statistics(const Rectangle *rects, int size) {
    if (size == 0) {
        printf("Inventory is empty. Statistics unavailable.\n");
        return;
    }

    int max_i = 0;
    int min_i = 0;
    float total_area = 0.0f;

    for (int i = 0; i < size; i++) {
        total_area += rects[i].area;
        if (rects[i].area > rects[max_i].area) {
            max_i = i;
        }
        if (rects[i].area < rects[min_i].area) {
            min_i = i;
        }
    }

    printf("Largest Area : ID %d (%.2f)\n", rects[max_i].id, (double)rects[max_i].area);
    printf("Smallest Area: ID %d (%.2f)\n", rects[min_i].id, (double)rects[min_i].area);
    printf("Total Area   : %.2f\n", (double)total_area);
    printf("Average Area : %.2f\n", (double)(total_area / (float)size));
}

void free_rectangle_list(Rectangle *rects) {
    free(rects);
}

int main(void) {
    int capacity = 0;
    int count = 0;

    printf("Enter initial capacity (1-%d): ", MAX_RECTS);
    if (scanf("%d", &capacity) != 1 || capacity < 1 || capacity > MAX_RECTS) {
        printf("Error: Capacity must be between 1 and %d.\n", MAX_RECTS);
        return 1;
    }
    clear_input_buffer();

    Rectangle *rect_list = create_rectangle_list(capacity);
    if (rect_list == NULL) {
        printf("Error: Allocation failed.\n");
        return 1;
    }

    int choice = 0;
    while (true) {
        printf("\n1. Add  2. Display  3. Search  4. Update  5. Delete  6. Sort/Filter  7. Stats  0. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Error: Invalid choice.\n");
            continue;
        }
        clear_input_buffer();

        if (choice == 0) {
            break;
        } else if (choice == 1) {
            int id = 0;
            float w = 0.0f;
            float h = 0.0f;

            printf("Enter unique positive Rectangle ID: ");
            if (scanf("%d", &id) != 1 || id <= 0) {
                clear_input_buffer();
                printf("Error: ID must be positive.\n");
                continue;
            }
            clear_input_buffer();

            if (find_rectangle_index(rect_list, count, id) != -1) {
                printf("Error: Rectangle ID %d already exists.\n", id);
                continue;
            }

            if (read_positive_dimensions(&w, &h)) {
                if (count == capacity && !resize_rectangle_list(&rect_list, &capacity)) {
                    continue;
                }
                set_rectangle(rect_list, count, id, w, h);
                count++;
                printf("Rectangle added successfully.\n");
            }
        } else if (choice == 2) {
            display_rectangles(rect_list, count);
        } else if (choice == 3) {
            search_rectangle(rect_list, count);
        } else if (choice == 4) {
            update_rectangle(rect_list, count);
        } else if (choice == 5) {
            delete_rectangle(rect_list, &count);
        } else if (choice == 6) {
display_sorted_and_filtered(rect_list, count);
} else if (choice == 7) {
display_area_statistics(rect_list, count);
} else {
printf("Error: Unknown option.\n");
}
}
free_rectangle_list(rect_list);
return 0;
}
