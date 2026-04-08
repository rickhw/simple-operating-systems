#ifndef _STDARG_H
#define _STDARG_H

// 直接借用 GCC 編譯器內建的不定參數處理巨集
#define va_list         __builtin_va_list
#define va_start(ap, p) __builtin_va_start(ap, p)
#define va_arg(ap, t)   __builtin_va_arg(ap, t)
#define va_end(ap)      __builtin_va_end(ap)

#endif
