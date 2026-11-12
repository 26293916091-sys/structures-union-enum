// Structures - 1 (Extended: Student Records and Academic Analytics System)
// Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 4.c -o 4

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_MAX 50

typedef struct {
    int roll_number;
    char name[NAME_MAX];
    int age;
    float gpa;
} Student;

void clear_input_buffer(void) {
    int ch = 0;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

Student *create_student_list(int n) {
    if (n <= 0 || (size_t)n > SIZE_MAX / sizeof(Student)) {
        return NULL;
    }

    Student *studs = (Student *)malloc((size_t)n * sizeof(Student));

    return studs;
}

bool resize_student_list(Student **studs, int *capacity) {
    int new_capacity = (*capacity == 0) ? 2 : (*capacity * 2);

    if (new_capacity <= *capacity || (size_t)new_capacity > SIZE_MAX / sizeof(Student)) {
        printf("Error: Cannot expand capacity further (overflow).\n");
        return false;
    }

    Student *temp = (Student *)realloc(*studs, (size_t)new_capacity * sizeof(Student));
    if (temp == NULL) {
        printf("Error: Memory reallocation failed.\n");
        return false;
    }

    *studs = temp;
    *capacity = new_capacity;
    printf("Capacity expanded to %d.\n", *capacity);

    return true;
}

int find_student_index(const Student *students, int size, int roll_number) {
    for (int i = 0; i < size; i++) {
        if (students[i].roll_number == roll_number) {
            return i;
        }
    }

    return -1;
}

void add_student(Student *studs, int i, int roll_number, const char *name, int age, float gpa) {
    studs[i].roll_number = roll_number;
    strncpy(studs[i].name, name, NAME_MAX - 1);
    studs[i].name[NAME_MAX - 1] = '\0';
    studs[i].age = age;
    studs[i].gpa = gpa;
}

bool read_valid_student_details(char *name_out, size_t name_size, int *age_out, float *gpa_out) {
    char buffer[256];

    printf("Enter Name (max 49 chars): ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return false;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    } else {
        clear_input_buffer();
        printf("Error: Name is too long (max 49 characters).\n");
        return false;
    }

    if (len == 0 || len >= name_size) {
        printf("Error: Name must be between 1 and 49 characters.\n");
        return false;
    }

    memcpy(name_out, buffer, len + 1);

    printf("Enter Age (16-100): ");
    if (scanf("%d", age_out) != 1 || *age_out < 16 || *age_out > 100) {
        clear_input_buffer();
        printf("Error: Age must be between 16 and 100.\n");
        return false;
    }
    clear_input_buffer();

    printf("Enter GPA (0.00-4.00): ");
    if (scanf("%f", gpa_out) != 1 || *gpa_out < 0.0f || *gpa_out > 4.0f) {
        clear_input_buffer();
        printf("Error: GPA must be between 0.00 and 4.00.\n");
        return false;
    }
    clear_input_buffer();

    return true;
}

void display_students(const Student *students, int size) {
    if (size == 0) {
        printf("No student records available.\n");
        return;
    }

    for (int i = 0; i < size; i++) {
        printf("Roll: %d, Name: %s, Age: %d, GPA: %.2f\n",
               students[i].roll_number, students[i].name, students[i].age, (double)students[i].gpa);
    }
}

void search_student(const Student *students, int size) {
    int roll = 0;

    if (size == 0) {
        printf("No student records available.\n");
        return;
    }

    printf("Enter Roll Number to search: ");
    if (scanf("%d", &roll) != 1) {
        clear_input_buffer();
        printf("Error: Invalid roll number.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_student_index(students, size, roll);
    if (idx == -1) {
        printf("Student with Roll %d not found.\n", roll);
        return;
    }

    printf("Found -> Roll: %d, Name: %s, Age: %d, GPA: %.2f\n",
           students[idx].roll_number, students[idx].name, students[idx].age, (double)students[idx].gpa);
}

void update_student(Student *students, int size) {
    int roll = 0;
    char name[NAME_MAX];
    int age = 0;
    float gpa = 0.0f;

    if (size == 0) {
        printf("No student records available.\n");
        return;
    }

    printf("Enter Roll Number to update: ");
    if (scanf("%d", &roll) != 1) {
        clear_input_buffer();
        printf("Error: Invalid roll number.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_student_index(students, size, roll);
    if (idx == -1) {
        printf("Student with Roll %d not found.\n", roll);
        return;
    }

    if (read_valid_student_details(name, sizeof(name), &age, &gpa)) {
        add_student(students, idx, roll, name, age, gpa);
        printf("Record updated successfully.\n");
    }
}

void delete_student(Student *students, int *size) {
    int roll = 0;

    if (*size == 0) {
        printf("No student records available.\n");
        return;
    }

    printf("Enter Roll Number to delete: ");
    if (scanf("%d", &roll) != 1) {
        clear_input_buffer();
        printf("Error: Invalid roll number.\n");
        return;
    }
    clear_input_buffer();

    int idx = find_student_index(students, *size, roll);
    if (idx == -1) {
        printf("Student with Roll %d not found.\n", roll);
        return;
    }

    for (int i = idx; i < *size - 1; i++) {
        students[i] = students[i + 1];
    }

    (*size)--;
    printf("Student with Roll %d deleted.\n", roll);
}

/* Sorts on a temporary copy so the original array order is unchanged */
void display_sorted_students(const Student *students, int size) {
    if (size == 0) {
        printf("No student records available.\n");
        return;
    }

    Student *copy = create_student_list(size);
    if (copy == NULL) {
        printf("Error: Memory allocation failed for sorting.\n");
        return;
    }

    memcpy(copy, students, (size_t)size * sizeof(Student));

    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            bool should_swap = false;

            if (copy[j].gpa < copy[j + 1].gpa) {
                should_swap = true;
            } else if (copy[j].gpa == copy[j + 1].gpa && copy[j].roll_number > copy[j + 1].roll_number) {
                should_swap = true;
            }

            if (should_swap) {
                Student temp = copy[j];
                copy[j] = copy[j + 1];
                copy[j + 1] = temp;
            }
        }
    }

    printf("--- Students Sorted by GPA (Descending) ---\n");
    display_students(copy, size);
    free(copy);
}

void display_gpa_summary(const Student *students, int size) {
    if (size == 0) {
        printf("No student records available for GPA summary.\n");
        return;
    }

    int high_idx = 0;
    int low_idx = 0;
    float total_gpa = 0.0f;

    for (int i = 0; i < size; i++) {
        total_gpa += students[i].gpa;
        if (students[i].gpa > students[high_idx].gpa) {
            high_idx = i;
        }
        if (students[i].gpa < students[low_idx].gpa) {
            low_idx = i;
        }
    }

    printf("Highest GPA: %.2f (%s, Roll %d)\n",
           (double)students[high_idx].gpa, students[high_idx].name, students[high_idx].roll_number);
    printf("Lowest GPA : %.2f (%s, Roll %d)\n",
           (double)students[low_idx].gpa, students[low_idx].name, students[low_idx].roll_number);
    printf("Average GPA: %.2f\n", (double)(total_gpa / (float)size));
}

void free_student_list(Student *students) {
    free(students);
}

int main(void) {
    int capacity = 0;
    int count = 0;

    printf("Enter initial capacity: ");
    if (scanf("%d", &capacity) != 1 || capacity <= 0) {
        printf("Error: Invalid initial capacity.\n");
        return 1;
    }
    clear_input_buffer();

    Student *student_list = create_student_list(capacity);
    if (student_list == NULL) {
        printf("Error: Memory allocation failed.\n");
        return 1;
    }

    int choice = 0;
    while (true) {
        printf("\n1. Add  2. Display  3. Search  4. Update  5. Delete  6. Sort  7. Summary  0. Exit\n");
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
            int roll = 0;
            char name[NAME_MAX];
            int age = 0;
            float gpa = 0.0f;

            printf("Enter unique positive Roll Number: ");
            if (scanf("%d", &roll) != 1 || roll <= 0) {
                clear_input_buffer();
                printf("Error: Roll number must be positive.\n");
                continue;
            }
            clear_input_buffer();

            if (find_student_index(student_list, count, roll) != -1) {
                printf("Error: Roll number %d already exists.\n", roll);
                continue;
            }

            if (read_valid_student_details(name, sizeof(name), &age, &gpa)) {
                if (count == capacity && !resize_student_list(&student_list, &capacity)) {
                    continue;
                }
                add_student(student_list, count, roll, name, age, gpa);
                count++;
                printf("Student added successfully.\n");
            }
        } else if (choice == 2) {
            display_students(student_list, count);
        } else if (choice == 3) {
            search_student(student_list, count);
        } else if (choice == 4) {
            update_student(student_list, count);
        } else if (choice == 5) {
            delete_student(student_list, &count);
        } else if (choice == 6) {
            display_sorted_students(student_list, count);
} else if (choice == 7) {
display_gpa_summary(student_list, count);
} else {
printf("Error: Unknown option.\n");
}
}
free_student_list(student_list);
return 0;
}