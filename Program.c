
/* Main program */

#include <stdio.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include "Program.h"

struct Library *IntuitionBase;

BOOL openLibs(void)
{
    if (IntuitionBase = OpenLibrary("intuition.library", 39l))
    {
        return TRUE;
    }
    return FALSE;
}

void closeLibs(void)
{
    CloseLibrary(IntuitionBase);
}

BOOL initScreen(void)
{
    extern BOOL loadGfx(void), prepRoom(void), openScreen(void);
    extern void closeScreen(void), closeRoom(void), unloadGfx(void);

    if (loadGfx()) /* Read graphics and palette */
    {
        if (prepRoom()) /* Prepare screen contents */
        {
            if (openScreen())
            {
                return TRUE;
            }
            closeRoom();
        }
        unloadGfx();
    }
    return FALSE;
}

void cleanScreen(void)
{
    extern void closeScreen(void), closeRoom(void), unloadGfx(void);

    closeScreen();
    closeRoom();
    unloadGfx();
}

BOOL initWindows(void)
{
    extern BOOL initGadgets(void), openWindows(void);
    extern void closeWindows(void), cleanGadgets(void);

    if (initGadgets()) /* Prepare before adding to windows */
    {
        if (openWindows())
        {
            return TRUE;
        }
        cleanGadgets();
    }
    return FALSE;
}

void cleanWindows(void)
{
    extern void closeWindows(void), cleanGadgets(void);

    closeWindows();
    cleanGadgets();
}

BOOL initJoystick(void)
{
    extern BOOL openDevice(WORD type), checkJoystick(void);
    extern void setJoystickTrigger(void), clearJoystick(void), readJoystick(void);
    extern void releaseJoystick(void), closeDevice(WORD type);

    if (openDevice(GAMEPORT)) /* Open gameport.device */
    {
        if (checkJoystick()) /* Check and obtain controller */
        {
            setJoystickTrigger(); /* Set trigger conditions */
            clearJoystick(); /* Clear pending requests */
            readJoystick(); /* Read input event */
            return TRUE;
        }
        closeDevice(GAMEPORT);
    }
    return FALSE;
}

void cleanJoystick(void)
{
    extern void releaseJoystick(void), closeDevice(WORD type);

    releaseJoystick(); /* Release controller */
    closeDevice(GAMEPORT);
}

BOOL initAudio(void)
{
    extern BOOL openAudio(void), loadSfx(void);
    extern void unloadSfx(void), closeAudio(void);

    if (openAudio())
    {
        if (loadSfx()) /* Load sound effects */
        {
            return TRUE;
        }
        closeAudio();
    }
    return FALSE;
}

void cleanAudio(void)
{
    extern void unloadSfx(void), closeAudio(void);

    unloadSfx();
    closeAudio();
}

BOOL initAll(void)
{
    if (initScreen())
    {
        if (initWindows())
        {
            if (initJoystick())
            {
                if (initAudio()) /* Init audio.device */
                {
                    return TRUE;
                }
                cleanJoystick();
            }
            cleanWindows();
        }
        cleanScreen();
    }
    return FALSE;
}

void cleanAll(void)
{
    cleanAudio();
    cleanJoystick();
    cleanWindows();
    cleanScreen();
}

void play(void)
{
    /* Gameplay here */
    Delay(500);
}

int main(void)
{
    int result = RETURN_WARN;
    if (openLibs())
    {
        if (initAll())
        {
            play();
            result = RETURN_OK;
            cleanAll();
        }
        closeLibs();
    }
    return result;
}
