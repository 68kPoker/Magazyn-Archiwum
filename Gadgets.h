
#include <exec/nodes.h>
#include <graphics/gfx.h>

enum
{
	NOTHING_GAD, /* Nothing happened */
	IMMEDIATE_GAD, /* Gadget down */
	RELEASE_GAD, /* Gadget up */
	DEACTIVATE_GAD, /* Gadget deactivation */
	INSIDE_GAD, /* Mouse over */
	OUTSIDE_GAD /* Mouse not over */
};

enum
{
	GAD_INSIDE,
	GAD_OUTSIDE
};

struct myGadget
{
	struct Node node;
	struct Rectangle rect;
	LONG (*action)(struct myGadget *gad, WORD mx, WORD my, WORD code);
	WORD status;
};

struct myWindow
{
	struct List gadlist;
};

VOID initWindow(struct myWindow *w);
VOID addGadget(struct myWindow *w, struct myGadget *gad);
struct myGadget *findGadget(struct myWindow *w, WORD x, WORD y);
LONG doButton(struct myGadget *gad, WORD mx, WORD my, WORD code);
