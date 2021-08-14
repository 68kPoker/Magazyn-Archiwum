
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <datatypes/pictureclass.h>
#include <intuition/screens.h>
#include <exec/memory.h>

#include <clib/dos_protos.h>
#include <clib/iffparse_protos.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/diskfont_protos.h>

#define RGB(c) ((c)|((c)<<8)|((c)<<16)|((c)<<24))
#define RowBytes(w) ((((w)+15)>>4)<<1)

struct ColorMap *colormap;
struct BitMap   *graphics, *roombitmaps[2];
ULONG           *palette;
struct TextFont *font;
struct Screen   *screen;

struct Rectangle dclip = { 0, 0, 319, 255 };

struct TextAttr ta =
{
    "centurion.font",
    9,
    FS_NORMAL,
    FPF_DISKFONT|FPF_DESIGNED
};

ULONG modeID = PAL_MONITOR_ID|LORES_KEY;

static BOOL readColors(struct StoredProperty *sp)
{
    extern struct ColorMap *colormap;

    UBYTE *cmap = sp->sp_Data;
    LONG size = sp->sp_Size, colors = size / 3;

    if (colormap = GetColorMap(colors))
    {
        WORD i;

        for (i = 0; i < colors; i++)
        {
            UBYTE red = *cmap++, green = *cmap++, blue = *cmap++;

            SetRGB32CM(colormap, i, RGB(red), RGB(green), RGB(blue));
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL readRow(BYTE *dest, BYTE **srcptr, LONG *sizeptr, UBYTE cmp, WORD bpr)
{
    BYTE *src = *srcptr;
    LONG size = *sizeptr;

    if (cmp == cmpNone)
    {
        if (size < bpr)
        {
            return FALSE;
        }
        size -= bpr;
        CopyMem(src, dest, bpr);
        src += bpr;
    }
    else if (cmp == cmpByteRun1)
    {
        while (bpr > 0)
        {
            BYTE c;
            if (size < 1)
            {
                return FALSE;
            }
            size--;
            if ((c = *src++) >= 0)
            {
                WORD count = c + 1;
                if (size < count || bpr < count)
                {
                    return FALSE;
                }
                size -= count;
                bpr -= count;
                while (count-- > 0)
                {
                    *dest++ = *src++;
                }
            }
            else if (c != -128)
            {
                WORD count = (-c) + 1;
                BYTE data;
                if (size < 1 || bpr < count)
                {
                    return FALSE;
                }
                size--;
                bpr -= count;
                data = *src++;
                while (count-- > 0)
                {
                    *dest++ = data;
                }
            }
        }
    }
    else
    {
        return FALSE;
    }

    *srcptr = src;
    *sizeptr = size;

    return TRUE;
}

static BOOL readBitMap(struct BitMapHeader *bmhd, BYTE *buffer, LONG size)
{
    extern struct BitMap *graphics;

    WORD width = bmhd->bmh_Width, height = bmhd->bmh_Height;
    UBYTE depth = bmhd->bmh_Depth, cmp = bmhd->bmh_Compression;
    /* UBYTE msk = bmhd->bmh_Masking; */

    if (graphics = AllocBitMap(width, height, depth, BMF_INTERLEAVED, NULL))
    {
        PLANEPTR planes[9];
        WORD i, j;
        BOOL success = FALSE;
        WORD bpr = RowBytes(width);

        for (i = 0; i < depth; i++)
        {
            planes[i] = graphics->Planes[i];
        }

        for (j = 0; j < height; j++)
        {
            for (i = 0; i < depth; i++)
            {
                if (!(success = readRow(planes[i], &buffer, &size, cmp, bpr)))
                {
                    break;
                }
                planes[i] += graphics->BytesPerRow;
            }
            if (!success)
            {
                break;
            }
        }

        if (success)
        {
            return TRUE;
        }
        FreeBitMap(graphics);
    }
    return FALSE;
}

static BOOL readGfx(struct IFFHandle *iff)
{
    struct StoredProperty *sp;

    if (sp = FindProp(iff, ID_ILBM, ID_BMHD))
    {
        struct BitMapHeader *bmhd = (struct BitMapHeader *)sp->sp_Data;

        if (sp = FindProp(iff, ID_ILBM, ID_CMAP))
        {
            if (readColors(sp))
            {
                struct ContextNode *cn;

                if (cn = CurrentChunk(iff))
                {
                    BYTE *buffer;
                    LONG size = cn->cn_Size;

                    if (buffer = AllocMem(size, MEMF_PUBLIC))
                    {
                        if (ReadChunkBytes(iff, buffer, size) == size)
                        {
                            if (readBitMap(bmhd, buffer, size))
                            {
                                FreeMem(buffer, size);
                                return TRUE;
                            }
                        }
                        FreeMem(buffer, size);
                    }
                }
                FreeColorMap(colormap);
            }
        }
    }
    return FALSE;
}

BOOL loadGfx(void)
{
    struct IFFHandle *iff;
    STRPTR name = "Data/Graphics.iff";
    BOOL result = FALSE;

    if (iff = AllocIFF())
    {
        if (iff->iff_Stream = Open(name, MODE_OLDFILE))
        {
            InitIFFasDOS(iff);
            if (!OpenIFF(iff, IFFF_READ))
            {
                if (!PropChunk(iff, ID_ILBM, ID_BMHD))
                {
                    if (!PropChunk(iff, ID_ILBM, ID_CMAP))
                    {
                        if (!StopChunk(iff, ID_ILBM, ID_BODY))
                        {
                            if (!ParseIFF(iff, IFFPARSE_SCAN))
                            {
                                result = readGfx(iff);
                            }
                        }
                    }
                }
                CloseIFF(iff);
            }
            Close(iff->iff_Stream);
        }
        FreeIFF(iff);
    }
    return result;
}

BOOL prepRoom(void)
{
    if (roombitmaps[0] = AllocBitMap(320, 256, GetBitMapAttr(graphics, BMA_DEPTH), BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
    {
        if (roombitmaps[1] = AllocBitMap(320, 256, GetBitMapAttr(graphics, BMA_DEPTH), BMF_DISPLAYABLE|BMF_INTERLEAVED|BMF_CLEAR, NULL))
        {
            if (palette = AllocVec(((colormap->Count * 3) + 2) * sizeof(ULONG), MEMF_PUBLIC|MEMF_CLEAR))
            {
                palette[0] = colormap->Count << 16;
                GetRGB32(colormap, 0, colormap->Count, palette + 1);

                BltBitMap(graphics, 0, 0, roombitmaps[0], 0, 0, 320, 256, 0xc0, 0xff, NULL);
                return TRUE;
            }
            FreeBitMap(roombitmaps[1]);
        }
        FreeBitMap(roombitmaps[0]);
    }
    return FALSE;
}

BOOL openScreen(void)
{
    if (font = OpenDiskFont(&ta))
    {
        if (screen = OpenScreenTags(NULL,
            SA_BitMap,      roombitmaps[0],
            SA_Colors32,    palette,
            SA_DClip,       &dclip,
            SA_DisplayID,   modeID,
            SA_Font,        &ta,
            SA_Quiet,       TRUE,
            SA_Exclusive,   TRUE,
            SA_ShowTitle,   FALSE,
            SA_BackFill,    LAYERS_NOBACKFILL,
            TAG_DONE))
        {
            return TRUE;
        }
        CloseFont(font);
    }
    return FALSE;
}

void closeScreen(void)
{
    CloseScreen(screen);
    CloseFont(font);
}

void closeRoom(void)
{
    FreeVec(palette);
    FreeBitMap(roombitmaps[1]);
    FreeBitMap(roombitmaps[0]);
}

void unloadGfx(void)
{
    extern struct ColorMap *colormap;
    extern struct BitMap *graphics;

    FreeBitMap(graphics);
    FreeColorMap(colormap);
}
