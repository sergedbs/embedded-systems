#ifndef APP_TYPES_H
#define APP_TYPES_H

typedef enum {
    CONTROL_MODE_ON_OFF = 0,
    CONTROL_MODE_PID,
} control_mode_t;

typedef enum {
    SYSTEM_STATUS_OK = 0,
    SYSTEM_STATUS_WARN,
    SYSTEM_STATUS_ALERT,
} system_status_t;

typedef enum {
    COMMAND_KIND_NONE = 0,
    COMMAND_KIND_MODE,
    COMMAND_KIND_SETPOINT,
    COMMAND_KIND_HYSTERESIS,
    COMMAND_KIND_PID_GAIN,
    COMMAND_KIND_PLOT,
    COMMAND_KIND_STATUS,
    COMMAND_KIND_INVALID,
} command_kind_t;

#endif // APP_TYPES_H
