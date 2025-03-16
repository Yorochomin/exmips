#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "mips.h"
#include "logfile.h"

static FILE *fp_logfile = NULL;
static unsigned int enabled_category;
static unsigned int lowerlevel;

void logfile_init(unsigned int enCategory, unsigned int loglv){
    if(NULL != fp_logfile){
        fclose(fp_logfile);
    }

    enabled_category = (enCategory & LOGCAT_MASK);
    lowerlevel       = (loglv      & LOGLV_MASK);

    fp_logfile = fopen(LOGFILE_NAME, "a+");
}

void logfile_close(void){
    if( fp_logfile == NULL ) return;

    fclose(fp_logfile);
    fp_logfile = NULL;
}

void logfile_printf(unsigned int loglevel, const char *format, ... ){
    va_list ap;
    time_t  t;
    struct tm *tmp;

    if(fp_logfile == NULL) return;
    if( !DEBUG ){
        if( 0 == (loglevel & enabled_category) && (loglevel & LOGLV_MASK) < LOGLV_WARNING ) return;
        if( (loglevel & LOGLV_MASK) < lowerlevel ) return;
    }

    time(&t);
    tmp = localtime(&t);

    va_start(ap, format);
    fprintf(fp_logfile, "[%d/%02d/%02d %02d:%02d:%02d |%x] ", 1900 + tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (loglevel & LOGLV_MASK));
    vfprintf(fp_logfile, format, ap);
    va_end(ap);
}

void logfile_printf_without_header(unsigned int loglevel, const char *format, ... ){
    va_list ap;

    if(fp_logfile == NULL) return;
    if( !DEBUG ){
        if( 0 == (loglevel & enabled_category) && (loglevel & LOGLV_MASK) < LOGLV_WARNING ) return;
        if( (loglevel & LOGLV_MASK) < lowerlevel ) return;
    }

    va_start(ap, format);
    vfprintf(fp_logfile, format, ap);
    va_end(ap);
}

