#include "io.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void io_console_action_sink(const char *action, const Value *val, void *user_data) {
    (void)user_data;
    if (!action) {
        return;
    }
    printf("[ACTION] %s", action);
    if (val && val->type != VAL_NONE) {
        printf("(");
        value_print(val, stdout);
        printf(")");
    }
    printf("\n");
}

void io_mock_init(IOMock *mock) {
    if (!mock) {
        return;
    }
    mock->head = NULL;
    mock->count = 0;
}

void io_mock_free(IOMock *mock) {
    if (!mock) {
        return;
    }
    MockAction *curr = mock->head;
    while (curr) {
        MockAction *next = curr->next;
        free(curr->name);
        value_free(&curr->arg);
        free(curr);
        curr = next;
    }
    mock->head = NULL;
    mock->count = 0;
}

bool io_mock_action_sink(const char *action, const Value *val, void *user_data) {
    if (!user_data || !action) {
        return false;
    }
    IOMock *mock = (IOMock *)user_data;
    MockAction *ma = (MockAction *)malloc(sizeof(MockAction));
    if (!ma) {
        return false;
    }
    ma->name = syllogist_strdup(action);
    if (!ma->name) {
        free(ma);
        return false;
    }
    ma->arg = val ? value_clone(val) : value_none();
    ma->next = NULL;

    if (!mock->head) {
        mock->head = ma;
    } else {
        MockAction *curr = mock->head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = ma;
    }
    mock->count++;
    return true;
}

const MockAction* io_mock_get_action(const IOMock *mock, size_t index) {
    if (!mock || index >= mock->count) {
        return NULL;
    }
    const MockAction *curr = mock->head;
    size_t i = 0;
    while (curr && i < index) {
        curr = curr->next;
        i++;
    }
    return curr;
}
