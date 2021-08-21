
/* $Log$ */

#include <intuition/screens.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <exec/memory.h>
#include <exec/interrupts.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>

#include "Screen.h"

extern void myCopper(void);
far extern struct Custom custom;

static struct Interrupt copper;
static struct
{
    struct ViewPort *vp;
    WORD            sig;
    struct Task     *task;
} data;

/*
** Make, so that Copper will signal us when the display Beam is
** at position (0,0) of the ViewPort.
*/
BOOL
addCopper(struct screenUD *p)
{
    copper.is_Code = myCopper;
    copper.is_Data = (APTR)&data;
    copper.is_Node.ln_Pri = 0;

    if ((data.sig = AllocSignal(-1)) != -1)
    {
        struct UCopList *ucl;

        if (ucl = AllocMem(sizeof(*ucl), MEMF_PUBLIC|MEMF_CLEAR))
        {
            CINIT(ucl, 3);
            CWAIT(ucl, 0, 0);
            CMOVE(ucl, custom.intreq, INTF_SETCLR|INTF_COPER);
            CEND(ucl);

            data.vp = &p->s->ViewPort;
            data.task = FindTask(NULL);

            Forbid();
            data.vp->UCopIns = ucl;
            Permit();

            RethinkDisplay();

            AddIntServer(INTB_COPER, &copper);

            p->sig = data.sig;
            return(TRUE);
        }
        FreeSignal(data.sig);
    }
    return(FALSE);
}

void
remCopper(struct screenUD *p)
{
    RemIntServer(INTB_COPER, &copper);
    FreeSignal(data.sig);
}
