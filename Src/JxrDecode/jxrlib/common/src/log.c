#include "../include/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void (*g_pfn_log_func)(JxrLogLevel, const char*) = NULL;

void JxrLibLog(JxrLogLevel log_level, const char* format, ...)
{
    void (*log_func)(JxrLogLevel, const char*) = g_pfn_log_func;
    if (log_func == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    char string_on_stack[1024];
    int length = vsnprintf(string_on_stack,sizeof(string_on_stack), format, args);
    if (length + 1 > sizeof(string_on_stack))
    {
        char* string_on_heap = (char*)malloc(length + 1);
        if (string_on_heap != NULL)
        {
            vsnprintf(string_on_heap, length + 1, format, args);
            log_func(log_level, string_on_heap);
            free(string_on_heap);
        }
    }
    else
    {
        log_func(log_level, string_on_stack);
    }

    va_end(args);
}

void SetJxrLogFunction(void (*log_func)(JxrLogLevel log_level, const char* log_text))
{
    g_pfn_log_func = log_func;
}