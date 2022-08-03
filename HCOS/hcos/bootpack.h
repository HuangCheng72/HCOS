/* asmhead.nas */
#include "asmhead.h"

/* naskfunc.nas */
#include "naskfunc.h"

/* fifo.c */
#include "fifo.h"

/* graphic.c */
#include "graphic.h"

/* dsctbl.c */
#include "dsctbl.h"

/* int.c */
#include "int.h"

/* InputDevice.c */
#include "InputDevice.h"

/* memory.c */
#include "memory.h"

/* sheet.c */
#include "sheet.h"

/* timer.c */
#include "timer.h"

/* mtask.c */
#include "mtask.h"

/* window.c */
#include "window.h"

/* console.c */
#include "console.h"

/* file.c */
#include "file.h"

/* tek.c */
#include "tek.h"

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);
