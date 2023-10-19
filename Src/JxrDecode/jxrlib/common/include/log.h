#ifndef JXRLIB_COMMON_INCLUDE_LOG_H_
#define JXRLIB_COMMON_INCLUDE_LOG_H_

#if __cplusplus
extern "C" {
#endif

    typedef enum {
        JXR_LOG_LEVEL_INFO,
        JXR_LOG_LEVEL_WARNING,
        JXR_LOG_LEVEL_ERROR
    } JxrLogLevel;

    void JxrLibLog(JxrLogLevel log_level, const char* format, ...);

    void SetJxrLogFunction(void (*log_func)(JxrLogLevel log_level, const char* log_text));

#if __cplusplus
} // extern "C"
#endif

#endif // JXRLIB_COMMON_INCLUDE_LOG_H_
