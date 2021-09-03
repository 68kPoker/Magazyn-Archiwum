
#include <devices/input.h>
#include <devices/inputevent.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>

#include "Input.h"

extern struct IntuitionBase *IntuitionBase;

struct Interrupt ih;

__saveds struct InputEvent *myInput(register __a0 struct InputEvent *ie, register __a1 struct inputInfo *ii)
{
    struct InputEvent *cur = ie, *next, *prev = NULL;
    BOOL change = FALSE;

    while (cur)
    {
        UBYTE class;
        BOOL skip = FALSE;

        next = cur->ie_NextEvent;

        if ((class = cur->ie_Class) == IECLASS_RAWKEY)
        {
            WORD key;
            change = TRUE;

            ObtainSemaphore(&ii->ss);
            if ((key = ii->curKey) < KEYS)
            {
                ii->keys[key++] = cur->ie_Code;

                ii->curKey = key;
            }
            ReleaseSemaphore(&ii->ss);

            skip = TRUE;

            if (prev)
            {
                prev->ie_NextEvent = next;
            }
            else
            {
                ie = next;
            }
        }
        else if (class == IECLASS_RAWMOUSE)
        {
            change = TRUE;
            if (cur->ie_Code == IECODE_NOBUTTON)
            {
                ObtainSemaphore(&ii->ss);

                ii->mouseX += cur->ie_X;
                ii->mouseY += cur->ie_Y;

                if (ii->mouseX < 0)
                    ii->mouseX = 0;
                else if (ii->mouseX >= 640)
                    ii->mouseX = 639;

                if (ii->mouseY < 0)
                    ii->mouseY = 0;
                else if (ii->mouseY >= 512)
                    ii->mouseY = 511;

                ReleaseSemaphore(&ii->ss);
            }
            else
            {
                WORD button;

                ObtainSemaphore(&ii->ss);

                if ((button = ii->curButton) < BUTTONS)
                {
                    ii->buttons[button].mouseX = ii->mouseX;
                    ii->buttons[button].mouseY = ii->mouseY;
                    ii->buttons[button].code = cur->ie_Code;

                    ii->curButton++;
                }
                ReleaseSemaphore(&ii->ss);

				skip = TRUE;
                if (prev)
                {
                    prev->ie_NextEvent = next;
                }
                else
                {
                    ie = next;
                }
            }
        }
        if (!skip)
        {
        	prev = cur;
        }
        cur = next;
    }

    if (change)
    {
        Signal(ii->task, 1L << ii->signal);
    }

    return(ie);
}

struct IOStdReq *openInput(struct inputInfo *ii)
{
    struct MsgPort *mp;
    if (mp = CreateMsgPort())
    {
        struct IOStdReq *io;
        if (io = CreateIORequest(mp, sizeof(*io)))
        {
            if (!OpenDevice("input.device", 0, (struct IORequest *)io, 0))
            {
                if ((ii->signal = AllocSignal(-1)) != -1)
                {
                    ULONG lock;

                    ii->task = FindTask(NULL);

                    InitSemaphore(&ii->ss);
                    ii->curKey = ii->curButton = 0;

                    lock = LockIBase(0);

                    ii->mouseX = IntuitionBase->MouseX;
                    ii->mouseY = IntuitionBase->MouseY;

                    ih.is_Code = (void(*)())myInput;
                    ih.is_Data = (APTR)ii;
                    ih.is_Node.ln_Pri = 75;

                    io->io_Data = (APTR)&ih;
                    io->io_Command = IND_ADDHANDLER;
                    DoIO((struct IORequest *)io);

                    UnlockIBase(lock);

                    return(io);
                }
                CloseDevice((struct IORequest *)io);
            }
            DeleteIORequest((struct IORequest *)io);
        }
        DeleteMsgPort(mp);
    }
    return(NULL);
}

VOID closeInput(struct IOStdReq *io, struct inputInfo *ii)
{
    struct MsgPort *mp = (struct MsgPort *)io->io_Message.mn_ReplyPort;

    io->io_Data = (APTR)&ih;
    io->io_Command = IND_REMHANDLER;
    DoIO((struct IORequest *)io);

    FreeSignal(ii->signal);
    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(mp);
}
