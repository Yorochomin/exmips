#ifndef __TERMINAL_H__
#define __TERMINAL_H__


#define TERM_RGB_COLOR(r,g,b)   ((1<<15) + (((r)>>4)<<8) + (((g)>>4)<<4) + (((b)>>4)<<0))
#define TERM_COLOR_R(c)         (((c)>>4)&0xf0)
#define TERM_COLOR_G(c)         ( (c)    &0xf0)
#define TERM_COLOR_B(c)         (((c)<<4)&0xf0)
#define IS_TERM_RGB_COLOR(c)    ((c)&(1<<15))

enum terminalCharColor {
    charColorBlack   = 30,
    charColorRed     = 31,
    charColorGreen   = 32,
    charColorYellow  = 33,
    charColorBlue    = 34,
    charColorMagenta = 35,
    charColorCyan    = 36,
    charColorWhite   = 37,
    charColorGray          = 90,
    charColorBrightBlack   = 90,
    charColorBrightRed     = 91,
    charColorBrightGreen   = 92,
    charColorBrightYellow  = 93,
    charColorBrightBlue    = 94,
    charColorBrightMagenta = 95,
    charColorBrightCyan    = 96,
    charColorBrightWhite   = 97,
    //----------------------------------
    charColorBrown   = TERM_RGB_COLOR(134, 74, 43),
    //----------------------------------
    charColorReset   = 39
};

enum terminalBGColor {
    BGColorBlack   = 40,
    BGColorRed     = 41,
    BGColorGreen   = 42,
    BGColorYellow  = 43,
    BGColorBlue    = 44,
    BGColorMagenta = 45,
    BGColorCyan    = 46,
    BGColorWhite   = 47,
    BGColorGray          = 100, //40,
    BGColorBrightBlack   = 100, //40,
    BGColorBrightRed     = 101, //41,
    BGColorBrightGreen   = 102, //42,
    BGColorBrightYellow  = 103, //43,
    BGColorBrightBlue    = 104, //44,
    BGColorBrightMagenta = 105, //45,
    BGColorBrightCyan    = 106, //46,
    BGColorBrightWhite   = 107, //47,
    //----------------------------------
    BGColorBrown   = TERM_RGB_COLOR(134, 74, 43),
    //----------------------------------
    BGColorReset   = 49
};

void termSetCharColor(enum terminalCharColor eCharColor);
void termSetBGColor(enum terminalBGColor eBGColor);
void termResetColor(void);
void termSetBlinkOff(void);
void termResetBlink(void);
void termGoTo(int x, int y);
void termClear(void);

void termResetSettingForExit(void);

extern const int fcolor[];
extern const int bgcolor[];

#endif
