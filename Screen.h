
#ifndef SCREEN_H
#define SCREEN_H

#include <exec/types.h>

#define BITMAPS 2
#define DEPTH   5

struct screenUD
{
    struct Screen   *s;
    struct BitMap   *bm[BITMAPS];
    struct TextFont *font;
    struct DBufInfo *dbi;
    UWORD           sig; /* Copper signal */
};

struct screenUD *openScreen(void);
void closeScreen(struct screenUD *p);
BOOL addDBuf(struct screenUD *p);
void remDBuf(struct screenUD *p);
UWORD safeToDraw(struct screenUD *p);
void changeBuffer(struct screenUD *p);
BOOL addCopper(struct screenUD *p);
void remCopper(struct screenUD *p);

#endif
