/*
 * Problem 4 (Extended): Interactive Command-Driven Task Manager
 * File: 7.c
 * Standard: ISO C11
 * Compilation: gcc -std=c11 -Wall -Wextra -Wpedantic 7.c -o 7
 *
 * Description:
 *   Converts the console command interpreter into a dynamic in-memory task
 *   manager using a CommandType enumeration and TaskStatus enumeration.
 *   Supports case-insensitive commands (ADD, DELETE, VIEW, UPDATE, COMPLETE,
 *   FIND, HELP, EXIT) with bounded fgets input, overflow guards, and clean
 *   memory deallocation.
 *
 * Sample Run:
 *   > add        -> ID: 101, Desc: "Finish C assignment" -> Added task 101.
 *   > ADD        -> ID: 102, Desc: "Push to GitHub"      -> Added task 102.
 *   > view       -> Lists task 101 [PENDING] and task 102 [PENDING].
 *   > complete   -> ID: 101 -> Marked task 101 as DONE.
 *   > find       -> ID: 101 -> Displays task 101 [DONE].
 *   > update     -> ID: 102, Desc: "Verify repository"   -> Updated task 102.
 *   > delete     -> ID: 102 -> Deleted task 102.
 *   > help       -> Lists all recognized commands.
 *   > exit       -> Releases memory and exits cleanly.
 *
 * Edge Cases Tested:
 *   - Mixed-case commands (add, Add, ADD)
 *   - Unknown command rejection without terminating session
 *   - Overlong command or description line flushed safely via fgets
 *   - Duplicate task ID rejection and nonexistent task ID handling
 *   - Spaces in task descriptions and empty description rejection
 *   - Malformed numeric IDs (non-digits, negative numbers, overflow)
 *   - Empty list operations, dynamic realloc growth, and EOF termination
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DESC_LEN 128
#define CMD_BUFFER_SIZE 64
#define INPUT_BUFFER_SIZE 256
#define INITIAL_CAPACITY 4

/* Supported console commands */
typedef enum {
    CMD_ADD,
    CMD_DELETE,
    CMD_VIEW,
    CMD_UPDATE,
    CMD_COMPLETE,
    CMD_FIND,
    CMD_HELP,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

/* Task completion state */
typedef enum {
    STATUS_PENDING,
    STATUS_DONE
} TaskStatus;

/* Individual task record */
typedef struct {
    int id;
    char description[MAX_DESC_LEN];
    TaskStatus status;
} Task;

/* Dynamic array holding all in-session tasks */
typedef struct {
    Task *tasks;
    size_t count;
    size_t capacity;
} TaskManager;

/* Flushes leftover characters when input exceeds buffer size */
static void clear_remaining_line(void) {
    int ch = 0;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

/* Reads a bounded line and trims whitespace; returns 1=ok, 0=overlong, -1=EOF */
static int read_bounded_line(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return -1;
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }

    size_t len = strlen(buffer);

    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    } else if (!feof(stdin)) {
        clear_remaining_line();
        return 0;
    }

    size_t start = 0;

    while (start < len && isspace((unsigned char)buffer[start])) {
        start++;
    }

    size_t end = len;

    while (end > start && isspace((unsigned char)buffer[end - 1])) {
        end--;
    }

    size_t trimmed = end - start;

    if (start > 0 && trimmed > 0) {
        memmove(buffer, buffer + start, trimmed);
    }

    buffer[trimmed] = '\0';

    return 1;
}

/* Converts a command string to uppercase for case-insensitive matching */
static void str_to_upper(char *str) {
    if (str == NULL) {
        return;
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        str[i] = (char)toupper((unsigned char)str[i]);
    }
}

/* Maps a raw command string to its CommandType enum constant */
static CommandType parse_command(const char *raw_cmd) {
    if (raw_cmd == NULL || raw_cmd[0] == '\0') {
        return CMD_UNKNOWN;
    }

    char upper[CMD_BUFFER_SIZE];
    strncpy(upper, raw_cmd, sizeof(upper) - 1);
    upper[sizeof(upper) - 1] = '\0';
    str_to_upper(upper);

    if (strcmp(upper, "ADD") == 0) {
        return CMD_ADD;
    }
    if (strcmp(upper, "DELETE") == 0) {
        return CMD_DELETE;
    }
    if (strcmp(upper, "VIEW") == 0) {
        return CMD_VIEW;
    }
    if (strcmp(upper, "UPDATE") == 0) {
        return CMD_UPDATE;
    }
    if (strcmp(upper, "COMPLETE") == 0) {
        return CMD_COMPLETE;
    }
    if (strcmp(upper, "FIND") == 0) {
        return CMD_FIND;
    }
    if (strcmp(upper, "HELP") == 0) {
        return CMD_HELP;
    }
    if (strcmp(upper, "EXIT") == 0) {
        return CMD_EXIT;
    }

    return CMD_UNKNOWN;
}

/* Reads and validates a positive integer task ID */
static bool prompt_positive_id(const char *prompt, int *out_id) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);
        int status = read_bounded_line(buf, sizeof(buf));

        if (status == -1) {
            return false;
        }
        if (status == 0) {
            printf("Error: Input line too long.\n");
            continue;
        }
        if (buf[0] == '\0') {
            printf("Error: Task ID cannot be empty.\n");
            continue;
        }

        char *endptr = NULL;
        errno = 0;
        long val = strtol(buf, &endptr, 10);

        if (errno == ERANGE || *endptr != '\0' || val <= 0 || val > INT_MAX) {
            printf("Error: Enter a valid positive integer ID.\n");
            continue;
        }

        *out_id = (int)val;

        return true;
    }
}

/* Reads a non-empty bounded task description (spaces allowed) */
static bool prompt_description(const char *prompt, char *out_desc, size_t max_len) {
    char buf[INPUT_BUFFER_SIZE];

    while (true) {
        printf("%s", prompt);
        int status = read_bounded_line(buf, sizeof(buf));

        if (status == -1) {
            return false;
        }
        if (status == 0) {
            printf("Error: Description input exceeded buffer limit.\n");
            continue;
        }

        size_t len = strlen(buf);

        if (len == 0) {
            printf("Error: Description cannot be empty.\n");
            continue;
        }
        if (len >= max_len) {
            printf("Error: Description exceeds max length of %zu characters.\n", max_len - 1);
            continue;
        }

        memcpy(out_desc, buf, len + 1);

        return true;
    }
}

/* Allocates initial task storage */
static int init_manager(TaskManager *mgr) {
    if (mgr == NULL) {
        return -1;
    }

    mgr->tasks = (Task *)malloc(INITIAL_CAPACITY * sizeof(Task));

    if (mgr->tasks == NULL) {
        return -1;
    }

    mgr->count = 0;
    mgr->capacity = INITIAL_CAPACITY;

    return 0;
}

/* Frees all dynamically allocated task memory */
static void free_manager(TaskManager *mgr) {
    if (mgr != NULL) {
        free(mgr->tasks);
        mgr->tasks = NULL;
        mgr->count = 0;
        mgr->capacity = 0;
    }
}

/* Doubles capacity via realloc when full, preserving old pointer on failure */
static int ensure_capacity(TaskManager *mgr) {
    if (mgr->count < mgr->capacity) {
        return 0;
    }

    size_t new_cap = (mgr->capacity == 0) ? 4 : mgr->capacity * 2;

    if (new_cap < mgr->capacity || new_cap > SIZE_MAX / sizeof(Task)) {
        return -1;
    }

    Task *resized = (Task *)realloc(mgr->tasks, new_cap * sizeof(Task));

    if (resized == NULL) {
        return -1;
    }

    mgr->tasks = resized;
    mgr->capacity = new_cap;

    return 0;
}

/* Finds array index of a task by ID; returns -1 if not found */
static int find_task_index(const TaskManager *mgr, int id) {
    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->tasks[i].id == id) {
            return (int)i;
        }
    }

    return -1;
}

static void print_task_row(const Task *t) {
    printf("%-8d | %-10s | %s\n",
           t->id,
           (t->status == STATUS_DONE) ? "DONE" : "PENDING",
           t->description);
}

static void cmd_add(TaskManager *mgr) {
    int id = 0;
    char desc[MAX_DESC_LEN];

    if (!prompt_positive_id("Enter unique positive Task ID: ", &id)) {
        return;
    }
    if (find_task_index(mgr, id) != -1) {
        printf("Error: Task ID %d already exists.\n", id);
        return;
    }
    if (!prompt_description("Enter task description: ", desc, MAX_DESC_LEN)) {
        return;
    }
    if (ensure_capacity(mgr) != 0) {
        fprintf(stderr, "Error: Memory allocation failed while adding task.\n");
        return;
    }

    Task *t = &mgr->tasks[mgr->count];
    t->id = id;
    memcpy(t->description, desc, strlen(desc) + 1);
    t->status = STATUS_PENDING;
    mgr->count++;

    printf("Success: Added task %d.\n", id);
}

static void cmd_delete(TaskManager *mgr) {
    if (mgr->count == 0) {
        printf("Task list is empty. Nothing to delete.\n");
        return;
    }

    int id = 0;

    if (!prompt_positive_id("Enter Task ID to delete: ", &id)) {
        return;
    }

    int idx = find_task_index(mgr, id);

    if (idx == -1) {
        printf("Error: Task ID %d not found.\n", id);
        return;
    }

    for (size_t i = (size_t)idx; i + 1 < mgr->count; i++) {
        mgr->tasks[i] = mgr->tasks[i + 1];
    }

    mgr->count--;
    printf("Success: Deleted task %d.\n", id);
}

static void cmd_update(TaskManager *mgr) {
    if (mgr->count == 0) {
        printf("Task list is empty. Nothing to update.\n");
        return;
    }

    int id = 0;

    if (!prompt_positive_id("Enter Task ID to update: ", &id)) {
        return;
    }

    int idx = find_task_index(mgr, id);

    if (idx == -1) {
        printf("Error: Task ID %d not found.\n", id);
        return;
    }

    char new_desc[MAX_DESC_LEN];

    if (!prompt_description("Enter new task description: ", new_desc, MAX_DESC_LEN)) {
        return;
    }

memcpy(mgr->tasks[idx].description, new_desc, strlen(new_desc) + 1);
printf("Success: Updated task %d.\n", id);
}
static void cmd_complete(TaskManager *mgr) {
if (mgr->count == 0) {
printf("Task list is empty. Nothing to complete.\n");
return;
}
int id = 0;
if (!prompt_positive_id("Enter Task ID to mark DONE: ", &id)) {
return;
}
int idx = find_task_index(mgr, id);
if (idx == -1) {
printf("Error: Task ID %d not found.\n", id);
return;
}
mgr->tasks[idx].status = STATUS_DONE;
printf("Success: Marked task %d as DONE.\n", id);
}
static void cmd_find(const TaskManager *mgr) {
if (mgr->count == 0) {
printf("Task list is empty.\n");
return;
}
int id = 0;
if (!prompt_positive_id("Enter Task ID to find: ", &id)) {
return;
}
int idx = find_task_index(mgr, id);
if (idx == -1) {
printf("Not Found: Task ID %d does not exist.\n", id);
return;
}
printf("\n%-8s | %-10s | %s\n", "Task ID", "Status", "Description");
printf("---------+------------+------------------------------------\n");
print_task_row(&mgr->tasks[idx]);
}
static void cmd_view(const TaskManager *mgr) {
if (mgr->count == 0) {
printf("Task list is currently empty.\n");
return;
}
printf("\n%-8s | %-10s | %s\n", "Task ID", "Status", "Description");
printf("---------+------------+------------------------------------\n");
for (size_t i = 0; i < mgr->count; i++) {
print_task_row(&mgr->tasks[i]);
}
}
static void cmd_help(void) {
printf("\nAvailable Commands (case-insensitive):\n");
printf("  ADD      - Add a new task with unique ID and description\n");
printf("  DELETE   - Remove an existing task by ID\n");
printf("  VIEW     - Display all tasks and their status\n");
printf("  UPDATE   - Modify the description of an existing task\n");
printf("  COMPLETE - Mark an existing task as DONE\n");
printf("  FIND     - Search and display a single task by ID\n");
printf("  HELP     - Show this command reference\n");
printf("  EXIT     - Release memory and terminate the program\n");
}
int main(void) {
TaskManager mgr = {NULL, 0, 0};
if (init_manager(&mgr) != 0) {
fprintf(stderr, "Fatal Error: Initial memory allocation failed.\n");
return 1;
}
printf("=== Interactive Command-Driven Task Manager ===\n");
printf("Type HELP for a list of commands.\n");
char cmd_buf[CMD_BUFFER_SIZE];
while (true) {
printf("\nCommand> ");
int status = read_bounded_line(cmd_buf, sizeof(cmd_buf));
if (status == -1) {
printf("\nEOF reached. Releasing memory and exiting...\n");
break;
}
if (status == 0) {
printf("Error: Command input exceeded %d characters. Type HELP.\n",
CMD_BUFFER_SIZE - 1);
continue;
}
CommandType cmd = parse_command(cmd_buf);
switch (cmd) {
case CMD_ADD: {
cmd_add(&mgr);
break;
}
case CMD_DELETE: {
cmd_delete(&mgr);
break;
}
case CMD_VIEW: {
cmd_view(&mgr);
break;
}
case CMD_UPDATE: {
cmd_update(&mgr);
break;
}
case CMD_COMPLETE: {
cmd_complete(&mgr);
break;
}
case CMD_FIND: {
cmd_find(&mgr);
break;
}
case CMD_HELP: {
cmd_help();
break;
}
case CMD_EXIT: {
printf("Exiting and freeing allocated memory...\n");
free_manager(&mgr);
return 0;
}
case CMD_UNKNOWN:
default: {
printf("Error: Unknown command '%s'. Type HELP for valid commands.\n", cmd_buf);
break;
}
}
}
free_manager(&mgr);
return 0;
}