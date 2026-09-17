#ifndef SYLLOGIST_IO_H
#define SYLLOGIST_IO_H

#include "fact.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*FactSourceFn)(const char *name, Value *out_val, void *user_data);
typedef bool (*ActionSinkFn)(const char *action_name, const Value *arg, void *user_data);

typedef struct IOInterface {
    FactSourceFn fact_source;
    ActionSinkFn action_sink;
    void *user_data;
} IOInterface;

/* Console action sink implementation */
void io_console_action_sink(const char *action, const Value *val, void *user_data);

/* Mock harness implementation for testing */
typedef struct MockAction {
    char *name;
    Value arg;
    struct MockAction *next;
} MockAction;

typedef struct IOMock {
    MockAction *head;
    size_t count;
} IOMock;

void io_mock_init(IOMock *mock);
void io_mock_free(IOMock *mock);
bool io_mock_action_sink(const char *action, const Value *val, void *user_data);
const MockAction* io_mock_get_action(const IOMock *mock, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* SYLLOGIST_IO_H */
