#include <stdio.h>

#define debug(...) debug_log("DEBUG", __DATE__, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define print_log(...) PRINT_LOG("LOG", __DATE__, __TIME__, __VA_ARGS__)
 
void debug_log(
			const char *logLevel,
            const char *date,
			const char *time,
			const char *file,
            const char *func,    
            const int   iLine,
            const char *format ,...)
{
		#include <stdarg.h>
 
		static char output[1024]={0};
        va_list arglst;
        va_start(arglst,format);
        vsnprintf(output,sizeof(output),format, arglst);
        printf("[%s %s] [%s] [%s] [%s] [%d] %s\n", date, time, logLevel, file, func, iLine, output);
        va_end(arglst);
}

void PRINT_LOG(
			const char *logLevel,
            const char *date,
			const char *time,
            const char *format ,...)
{
		#include <stdarg.h>
 
		static char output[1024]={0};
        va_list arglst;
        va_start(arglst,format);
        vsnprintf(output,sizeof(output), format, arglst);
        printf("[%s %s] [%s] %s\n", date, time, logLevel, output);
        va_end(arglst);
}