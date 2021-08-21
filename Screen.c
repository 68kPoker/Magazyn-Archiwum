
/* $Log$ */

#include <intuition/screens.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/diskfont_protos.h>

#include "Screen.h"

static struct screenUD sud;
static struct Rectangle dclip = { 0, 0, 319, 255 }; /* Display rectangle */
static struct TextAttr ta =
{
    "centurion.font",
    9,
    FS_NORMAL,
    FPF_DISKFONT|FPF_DESIGNED
};

/*
** Open and setup double-buffered screen for game display.
*/
struct screenUD
*openScreen(void)
{
    struct screenUD *p = &sud;

    /*
    ** Allocate buffers.
    */
    if (p->bm[0] = AllocBitMap(320, 256, DEPTH, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
    {
        if (p->bm[1] = AllocBitMap(320, 256, DEPTH, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
        {
            /*
            ** Open custom font.
            */
            if (p->font = OpenDiskFont(&ta))
            {
                if (p->s = OpenScreenTags(NULL,
                    SA_DClip,       &dclip,
                    SA_DisplayID,   LORES_KEY,
                    SA_BitMap,      p->bm[0],
                    SA_Font,        &ta,
                    SA_Quiet,       TRUE,
                    SA_Exclusive,   TRUE,
                    SA_BackFill,    LAYERS_NOBACKFILL,
                    SA_ShowTitle,   FALSE,
                    TAG_DONE))
                {
                    /*
                    ** Add double-buffering info (DBuf.c)
                    */
                    if (addDBuf(p))
                    {
                        return(p);
                    }
                    CloseScreen(p->s);
                }
                CloseFont(p->font);
            }
            FreeBitMap(p->bm[1]);
        }
        FreeBitMap(p->bm[0]);
    }
    return(NULL);
}

/*
** Close and free display.
*/
void
closeScreen(struct screenUD *p)
{
    remDBuf(p);
    CloseScreen(p->s);
    CloseFont(p->font);
    FreeBitMap(p->bm[1]);
    FreeBitMap(p->bm[0]);
}
