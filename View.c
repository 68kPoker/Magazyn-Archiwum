
#include <stdio.h>
#include "debug.h"

#include <exec/memory.h>
#include <exec/interrupts.h>
#include <graphics/view.h>
#include <graphics/videocontrol.h>
#include <graphics/gfxmacros.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>

#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>

#include "View.h"

extern __far struct Custom custom;

extern VOID myCopper();

struct Interrupt cis;
struct copperData cd;

/* Prepare View to work. Attach MonitorSpec and ViewExtra. */
BOOL initView(struct View *v, ULONG modeID)
{
    struct MonitorSpec *ms;
    struct MonitorInfo mi;

    if (!(ms = OpenMonitor(NULL, modeID)))
        printf("Couldn't open monitor!\n");
    else
    {
        struct ViewExtra *ve;
        if (!(ve = GfxNew(VIEW_EXTRA_TYPE)))
            printf("Couldn't alloc viewextra!\n");
        else
        {
            if (!(GetDisplayInfoData(NULL, (UBYTE *)&mi, sizeof(mi), DTAG_MNTR, modeID) > 0))
                printf("Couldn't get monitorinfo!\n");
            {
                ve->Monitor = ms;
                InitView(v);
                v->DxOffset = mi.ViewPosition.x;
                v->DyOffset = mi.ViewPosition.y;
                GfxAssociate(v, ve);
                D(bug("ViewPosition = (%d, %d)\n", v->DxOffset, v->DyOffset));
                return(TRUE);
            }
            GfxFree(ve);
        }
        CloseMonitor(ms);
    }
    return(FALSE);
}

VOID freeView(struct View *v)
{
    struct ViewExtra *ve = GfxLookUp(v);
    struct MonitorSpec *ms = ve->Monitor;

    GfxFree(ve);
    CloseMonitor(ms);

    if (v->LOFCprList)
        FreeCprList(v->LOFCprList);

    if (v->SHFCprList)
        FreeCprList(v->SHFCprList);
}

BOOL initViewPort(struct View *v, struct ViewPort *vp, struct RasInfo *ri, ULONG modeID, UBYTE depth)
{
    DisplayInfoHandle dih;
    struct DimensionInfo dims;

    v->ViewPort = vp;

    if (!(dih = FindDisplayInfo(modeID)))
        printf("Couldn't find displayinfo!\n");
    else
    {
        if (!(GetDisplayInfoData(NULL, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, modeID) > 0))
            printf("Couldn't get dimensioninfo!\n");
        else
        {
            struct ColorMap *cm;

            InitVPort(vp);
            vp->DxOffset = 0;
            vp->DyOffset = 0;
            vp->DWidth = dims.Nominal.MaxX - dims.Nominal.MinX + 1;
            vp->DHeight = dims.Nominal.MaxY - dims.Nominal.MinY + 1;
            vp->RasInfo = ri;
            vp->Next = NULL;

            if (!(cm = GetColorMap(1 << depth)))
                printf("Couldn't get colormap!\n");
            else
            {
                struct ViewPortExtra *vpe;
                if (!(vpe = GfxNew(VIEWPORT_EXTRA_TYPE)))
                    printf("Couldn't alloc viewportextra!\n");
                {
                    vpe->DisplayClip = dims.Nominal;

                    if (VideoControlTags(cm,
                        VTAG_ATTACH_CM_SET,     vp,
                        VTAG_VIEWPORTEXTRA_SET, vpe,
                        VTAG_NORMAL_DISP_SET,   dih,
                        VTAG_END_CM))
                        printf("VideoControl error!\n");
                    else
                    {
                        ri->RxOffset = ri->RyOffset = 0;
                        ri->Next = NULL;
                        if (!(ri->BitMap = AllocBitMap(vp->DWidth, vp->DHeight, depth, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL)))
                            printf("Couldn't alloc bitmap!\n");
                        else
                        {
                            if (MakeVPort(v, vp) != MVP_OK)
                                printf("MakeVPort error!\n");
                            else
                            {
                                if (!((cd.signal = AllocSignal(-1)) != -1))
                                    printf("Couldn't alloc signal!\n");
                                else
                                {
                                    struct UCopList *ucl;
                                    if (!(ucl = AllocMem(sizeof(*ucl), MEMF_PUBLIC|MEMF_CLEAR)))
                                        printf("Out of memory!\n");
                                    else
                                    {
                                        CINIT(ucl, 3);
                                        CWAIT(ucl, 0, 0);
                                        CMOVE(ucl, custom.intreq, INTF_SETCLR|INTF_COPER);
                                        CEND(ucl);

                                        Forbid();
                                        vp->UCopIns = ucl;
                                        Permit();

                                        if (MrgCop(v) != MCOP_OK)
                                            printf("MrgCop error!\n");
                                        else
                                        {
                                            cd.vp = vp;
                                            cd.task = FindTask(NULL);

                                            cis.is_Code = myCopper;
                                            cis.is_Data = (APTR)&cd;
                                            cis.is_Node.ln_Pri = 0;

                                            if (!(AttachPalExtra(cm, vp) == 0))
                                                printf("Couldn't attach pal!\n");
                                            else
                                            {
                                                AddIntServer(INTB_COPER, &cis);
                                                return(TRUE);
                                            }
                                        }
                                    }
                                    FreeSignal(cd.signal);
                                }
                                FreeVPortCopLists(vp);
                            }
                            FreeBitMap(ri->BitMap);
                        }
                    }
                    GfxFree(vpe);
                }
                FreeColorMap(cm);
            }
        }
    }
    return(FALSE);
}

VOID freeViewPort(struct ViewPort *vp)
{
    struct TagItem vctags[] =
    {
        VTAG_VIEWPORTEXTRA_GET, 0,
        VTAG_END_CM, 0
    };

    RemIntServer(INTB_COPER, &cis);
    FreeSignal(cd.signal);

    FreeVPortCopLists(vp);
    FreeBitMap(vp->RasInfo->BitMap);

    if (VideoControl(vp->ColorMap, vctags))
        printf("VideoControl error!\n");
    else
    {
        GfxFree((struct ExtendedNode *)vctags[0].ti_Data);
        FreeColorMap(vp->ColorMap);
    }
}
