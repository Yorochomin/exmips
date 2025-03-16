#ifndef __MISC_H__ 
#define __MISC_H__ 

extern const char *regStr[32];

void log_printReg32(int loglevel, struct stReg *preg);

uint32_t parseHex(char *str);
uint32_t parseDec(char *str);

#endif
