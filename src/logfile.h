#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#define LOGFILE_NAME "mips_emu.log"

#define LOGCAT_MASK     0xfff0

#define LOGCAT_EMU      (1<< 4)

#define LOGCAT_CPU_EXE  (1<< 5)
#define LOGCAT_CPU_MEM  (1<< 6)

#define LOGCAT_IO_UART  (1<< 7)
#define LOGCAT_IO_FDC   (1<< 8)
#define LOGCAT_IO_DMAC  (1<< 9)
#define LOGCAT_IO_PIC   (1<<10)
#define LOGCAT_IO_VIDEO (1<<11)
#define LOGCAT_IO_CMOS  (1<<12)
#define LOGCAT_IO_TIMER (1<<13)
#define LOGCAT_IO_MISC  (1<<15)


#define LOGLV_MASK    0xf

#define LOGLV_VERBOSE 0x0
#define LOGLV_INFO    0x1
#define LOGLV_INFO2   0x2
#define LOGLV_INFO3   0x3
#define LOGLV_INFO4   0x4
#define LOGLV_NOTICE  0xc
#define LOGLV_WARNING 0xe
#define LOGLV_ERROR   0xf

void logfile_init(unsigned int enCategory, unsigned int loglv);
void logfile_close(void);
void logfile_printf(unsigned int loglevel, const char *format, ... );
void logfile_printf_without_header(unsigned int loglevel, const char *format, ... );


#endif



