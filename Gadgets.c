
#include <devices/inputevent.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

#include "Gadgets.h"

VOID initWindow(struct myWindow *w)
{
	NewList(&w->gadlist);
}

VOID addGadget(struct myWindow *w, struct myGadget *gad)
{
	AddTail(&w->gadlist, &gad->node);
}

struct myGadget *findGadget(struct myWindow *w, WORD x, WORD y)
{
	struct myGadget *gad;

	for (gad = (struct myGadget *)w->gadlist.lh_Head;
		 gad->node.ln_Succ != NULL;
		 gad = (struct myGadget *)gad->node.ln_Succ)
	{
		if (x >= gad->rect.MinX && x <= gad->rect.MaxX && y >= gad->rect.MinY && y <= gad->rect.MaxY)
		{
			return(gad);
		}
	}
	return(NULL);
}

LONG doAction(struct myGadget *gad, WORD mx, WORD my, WORD code)
{
	return(gad->action(gad, mx - gad->rect.MinX, my - gad->rect.MinY, code));
}

LONG doButton(struct myGadget *gad, WORD mx, WORD my, WORD code)
{
	if (code == IECODE_LBUTTON)
	{
		/* Push */
		gad->status = GAD_INSIDE;
		return(IMMEDIATE_GAD);
	}
	else if (code == (IECODE_LBUTTON|IECODE_UP_PREFIX))
	{
		/* Deactivate */
		if (gad->status == GAD_INSIDE)
		{
			return(RELEASE_GAD);
		}
		else
		{
			return(DEACTIVATE_GAD);
		}
	}
	else if (code == IECODE_NOBUTTON)
	{
		if (mx >= 0 && mx <= gad->rect.MaxX && my >= 0 && my <= gad->rect.MaxY)
		{
			/* Inside area */
			if (gad->status == GAD_OUTSIDE)
			{
				gad->status = GAD_INSIDE;
				return(INSIDE_GAD);
			}
			else
			{
				return(NOTHING_GAD);
			}
		}
		else
		{
			/* Outside area */
			if (gad->status == GAD_INSIDE)
			{
				gad->status = GAD_OUTSIDE;
				return(OUTSIDE_GAD);
			}
			else
			{
				return(NOTHING_GAD);
			}
		}
	}

	return(NOTHING_GAD);
}
