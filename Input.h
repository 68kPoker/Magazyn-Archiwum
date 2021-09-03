
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <devices/inputevent.h>

#define KEYS    4
#define BUTTONS 4

struct inputInfo
{
    struct SignalSemaphore ss;
    WORD signal;
    struct Task *task;
    WORD keys[KEYS]; /* Last keys */
    WORD curKey;
    struct
    {
        WORD mouseX, mouseY;
        WORD code;
    } buttons[BUTTONS];
    WORD curButton;

    WORD mouseX, mouseY; /* Current pos */
};

struct IOStdReq *openInput(struct inputInfo *ii);
VOID closeInput(struct IOStdReq *io, struct inputInfo *ii);
