#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Led.h"

#define LED_GPIO GPIO_NUM_2

typedef enum {
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_UNKNOWN,
    CMD_INVALID
} Command_t;

Command_t CommandParser_Parse(const char* input) {
    char cmd1[16], cmd2[16];
    
    if (sscanf(input, "%15s %15s", cmd1, cmd2) != 2) {
        return CMD_INVALID;
    }
    
    if (strcmp(cmd1, "led") != 0) {
        return CMD_UNKNOWN;
    }
    
    if (strcmp(cmd2, "on") == 0) {
        return CMD_LED_ON;
    } else if (strcmp(cmd2, "off") == 0) {
        return CMD_LED_OFF;
    }
    
    return CMD_UNKNOWN;
}

void CommandLoop_Run(void) {
    char line[64];
    int pos = 0;

    printf("> ");
    fflush(stdout);

    while (1) {
        int c = getchar();
        
        if (c == EOF) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            continue;
        }
        
        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            printf("\n");
            line[pos] = '\0';
            
            if (pos == 0) {
                printf("> ");
                fflush(stdout);
                continue;
            }
            
            Command_t cmd = CommandParser_Parse(line);
            
            switch (cmd) {
                case CMD_LED_ON:
                    Led_On();
                    printf("[OK] LED ON\n");
                    break;
                case CMD_LED_OFF:
                    Led_Off();
                    printf("[OK] LED OFF\n");
                    break;
                case CMD_INVALID:
                    printf("[ERROR] Invalid input\n");
                    break;
                case CMD_UNKNOWN:
                    printf("[ERROR] Unknown command\n");
                    break;
            }
            
            pos = 0;
            printf("> ");
            fflush(stdout);
            continue;
        }
        
        if (pos < sizeof(line) - 1 && c >= 32 && c <= 126) {
            line[pos++] = c;
            putchar(c);
            fflush(stdout);
        }
    }
}

void app_main(void) {
    Led_Init(LED_GPIO);

    printf("STDIO LED Control\n");
    printf("Commands:\n");
    printf("  led on\n");
    printf("  led off\n");

    CommandLoop_Run();
}
