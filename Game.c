
#include <graphics/layers.h>
#include <graphics/clip.h>

#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>
#include <clib/diskfont_protos.h>
#include <clib/exec_protos.h>

#include "View.h"
#include "Input.h"
#include "Gadgets.h"

#define ESC_KEY 0x45

enum
{
	CTRL_C,
	COPPER_SIG,
	INPUT_SIG,
	SAFE_SIG,
	SIGNALS
};

STRPTR states[] =
{
	"Nothing   ",
	"Immediate ",
	"Release   ",
	"Deactivate",
	"Inside    ",
	"Outside   "
};

VOID waitSafe(struct DBufInfo *dbi, BOOL *safe)
{
	struct MsgPort *mp = dbi->dbi_SafeMessage.mn_ReplyPort;

	if (!*safe)
	{
		while (!GetMsg(mp))
		{
			WaitPort(mp);
		}
		*safe = TRUE;
	}
}

VOID loop(struct ViewPort *vp, struct BitMap *bm[], struct inputInfo *ii, struct DBufInfo *dbi, BOOL *safe, WORD *frame, struct Layer_Info *li, struct Layer *basel, WORD *pens)
{
	ULONG signals[SIGNALS] =
	{
		SIGBREAKF_CTRL_C,
		1 << cd.signal,
		1 << ii->signal,
		1 << dbi->dbi_SafeMessage.mn_ReplyPort->mp_SigBit
	}, total = signals[0]|signals[1]|signals[2]|signals[3];

	BOOL done = FALSE;
	WORD line = 0;

	struct RastPort *rp = basel->rp;
	UBYTE text[40];

	WORD counter = 0;
	struct myWindow w;
	struct myGadget gad, *active = NULL;
	WORD prevx = -1, prevy = -1;

	initWindow(&w);
	gad.rect.MinX = 0;
	gad.rect.MinY = 0;
	gad.rect.MaxX = 319;
	gad.rect.MaxY = 15;
	gad.action = doButton;

	addGadget(&w, &gad);

	while (!done)
	{
		ULONG result = Wait(total);

		if (result & signals[CTRL_C])
			break;

		if (result & signals[SAFE_SIG])
		{
			/* Wait and draw */
			waitSafe(dbi, safe);

			Move(rp, 0, (rp->Font->tf_YSize * 3) + rp->Font->tf_Baseline);
			sprintf(text, "%4d", counter);
			SetABPenDrMd(rp, pens[0], 0, JAM2);
			Text(rp, text, 4);
		}

		if (result & signals[COPPER_SIG])
		{
			counter++;
		}

		if (result & signals[INPUT_SIG])
		{
			WORD key, button;

			ObtainSemaphore(&ii->ss);

			for (key = 0; key < ii->curKey; key++)
			{
				if (ii->keys[key] == ESC_KEY)
				{
					done = TRUE;
				}
			}

			ii->curKey = 0;

			for (button = 0; button < ii->curButton; button++)
			{
				WORD mx = ii->buttons[button].mouseX >> 1,
				     my = ii->buttons[button].mouseY >> 1,
				     code = ii->buttons[button].code;

				if (code == IECODE_LBUTTON)
				{
					if (active = findGadget(&w, mx, my))
					{
						WORD res = doAction(active, mx, my, code);
						waitSafe(dbi, safe);

						Move(rp, 0, rp->Font->tf_Baseline);
						SetAPen(rp, pens[0]);
						Text(rp, states[res], 10);
					}
				}
				else if (code == (IECODE_LBUTTON|IECODE_UP_PREFIX))
				{
					if (active)
					{
						WORD res = doAction(active, mx, my, code);

						waitSafe(dbi, safe);

						Move(rp, 0, rp->Font->tf_Baseline);
						SetAPen(rp, pens[0]);
						Text(rp, states[res], 10);

						active = NULL;
					}
				}
			}

			ii->curButton = 0;

			WORD mx = ii->mouseX >> 1, my = ii->mouseY >> 1;

			if (mx != prevx || my != prevy)
			{
				prevx = mx;
				prevy = my;

				if (active)
				{
					WORD res = doAction(active, mx, my, IECODE_NOBUTTON);

					if (res != NOTHING_GAD)
					{
						waitSafe(dbi, safe);

						Move(rp, 0, rp->Font->tf_Baseline);
						SetAPen(rp, pens[0]);
						Text(rp, states[res], 10);
					}
				}
			}

			ReleaseSemaphore(&ii->ss);
		}
	}
}

int main(void)
{
	struct View v;
	struct ViewPort vp;
	struct RasInfo ri;
	ULONG modeID = LORES_KEY;
	UBYTE depth = 5;
	struct Layer_Info *li;
	struct Layer *basel;

	WORD i = 0, pen = -1;
	struct TextFont *font;
	struct TextAttr ta =
	{
		"centurion.font", 9, FS_NORMAL, FPF_DISKFONT|FPF_DESIGNED
	};
	struct BitMap *bm[2];
	struct DBufInfo *dbi;
	BOOL safe = TRUE;
	UWORD frame = 1;
	struct IOStdReq *inp;
	struct inputInfo ii;

	if (initView(&v, modeID))
	{
		if (initViewPort(&v, &vp, &ri, modeID, depth))
		{
			bm[0] = ri.BitMap;

			LoadView(&v);
			WaitTOF();
			WaitTOF();

			if (li = NewLayerInfo())
			{
				InstallLayerInfoHook(li, LAYERS_NOBACKFILL);
				if (basel = CreateUpfrontHookLayer(li, bm[0], 0, 0, vp.DWidth - 1, vp.DHeight - 1, LAYERSIMPLE, LAYERS_NOBACKFILL, NULL))
				{
					if (!(font = OpenDiskFont(&ta)))
						printf("Couldn't open %s size %d!\n", ta.ta_Name, ta.ta_YSize);
					else
					{
						/* SetFont(basel->rp, font); */
						if ((pen = ObtainPen(vp.ColorMap, 1, RGB(0xaa), RGB(0xaa), RGB(0xaa), 0)) == -1)
							printf("Couldn't obtain pen!\n");
						else
						{
							struct MsgPort *safeport;
							if (safeport = CreateMsgPort())
							{
								if (dbi = AllocDBufInfo(&vp))
								{
									dbi->dbi_SafeMessage.mn_ReplyPort = safeport;
									safe = TRUE;
									frame = 1;
									if (bm[1] = AllocBitMap(vp.DWidth, vp.DHeight, depth, BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
									{
										if (inp = openInput(&ii))
										{
											loop(&vp, bm, &ii, dbi, &safe, &frame, li, basel, &pen);
											closeInput(inp, &ii);
										}
										FreeBitMap(bm[1]);
									}
									waitSafe(dbi, &safe);
									FreeDBufInfo(dbi);
								}
								DeleteMsgPort(safeport);
							}
							ReleasePen(vp.ColorMap, pen);
						}
						CloseFont(font);
					}
					DeleteLayer(0, basel);
				}
				DisposeLayerInfo(li);
			}
			RethinkDisplay();

			freeViewPort(&vp);
		}
		freeView(&v);
	}
	return(0);
}
