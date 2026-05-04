#ifndef APP_TYPES_H
#define APP_TYPES_H

typedef enum {
    SYSTEM_STATUS_OK = 0,
    SYSTEM_STATUS_WARN,
    SYSTEM_STATUS_ALERT,
} system_status_t;

typedef enum {
    COMMAND_KIND_NONE = 0,
    COMMAND_KIND_BIN,
    COMMAND_KIND_MOTOR,
    COMMAND_KIND_STOP,
    COMMAND_KIND_STATUS,
    COMMAND_KIND_ALERT,
    COMMAND_KIND_INVALID,
} command_kind_t;

#endif // APP_TYPES_H
