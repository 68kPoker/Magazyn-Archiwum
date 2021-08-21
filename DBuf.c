
/* $Log$ */

#include <intuition/screens.h>

#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>

#include "Screen.h"

static struct safeUD
{
    BOOL  safe;
    UWORD frame;
} safe;

/*
** Prepare double-buffering information.
*/
BOOL
addDBuf(struct screenUD *p)
{
    struct ViewPort *vp = &p->s->ViewPort;

    if (p->dbi = AllocDBufInfo(vp))
    {
        struct MsgPort *mp;

        if (mp = CreateMsgPort())
        {
            p->dbi->dbi_SafeMessage.mn_ReplyPort = mp;
            p->dbi->dbi_UserData1 = (APTR)&safe; /* Unused */

            safe.safe  = TRUE; /* Safe to draw? */
            safe.frame = 1; /* Render into */

            if (addCopper(p))
            {
                return(TRUE);
            }
            DeleteMsgPort(mp);
        }
        FreeDBufInfo(p->dbi);
    }
    return(FALSE);
}

/*
** Wait until it's safe to draw.
*/
UWORD
safeToDraw(struct screenUD *p)
{
    struct MsgPort *mp = p->dbi->dbi_SafeMessage.mn_ReplyPort;

    if (!safe.safe)
    {
        while (!GetMsg(mp))
        {
            WaitPort(mp);
        }
        safe.safe = TRUE;
    }
    return(safe.frame);
}

/*
** Change buffers.
*/
void
changeBuffer(struct screenUD *p)
{
    if (safe.safe)
    {
        WaitBlit();
        ChangeVPBitMap(&p->s->ViewPort, p->bm[safe.frame], p->dbi);
        safe.frame ^= 1;
        safe.safe = FALSE;
    }
}

void
remDBuf(struct screenUD *p)
{
    struct MsgPort *mp = p->dbi->dbi_SafeMessage.mn_ReplyPort;

    remCopper(p);
    if (!safe.safe)
    {
        while (!GetMsg(mp))
        {
            WaitPort(mp);
        }
    }
    DeleteMsgPort(mp);
    FreeDBufInfo(p->dbi);
}
