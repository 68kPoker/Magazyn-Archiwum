
#include <exec/types.h>

#define RGB(c) ((c)|((c)<<8)|((c)<<16)|((c)<<24))

struct copperData
{
    struct ViewPort *vp;
    WORD signal;
    struct Task *task;
};

extern struct copperData cd;

BOOL initView(struct View *v, ULONG modeID);
VOID freeView(struct View *v);

BOOL initViewPort(struct View *v, struct ViewPort *vp, struct RasInfo *ri, ULONG modeID, UBYTE depth);
VOID freeViewPort(struct ViewPort *vp);
