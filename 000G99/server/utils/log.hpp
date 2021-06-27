#include <stdio.h>

#define debug(...) debug_log("DEBUG", __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
 
void debug_log(
			const char *logLevel,
			const char *time,
			const char *file,
            const char *func,    
            const int   iLine,
            const char *format ,...)
{
		#include <stdarg.h>
 
		static char output[10240]={0};
        va_list arglst;
        va_start(arglst,format);
        vsnprintf(output,sizeof(output),format, arglst);
        printf("[%s][%s][%s][%s][%d]:%s\n",time, logLevel, file, func, iLine, output);
        va_end(arglst);
}