#include <utio.h>
using namespace utio;
using namespace utio::gdt;

static EColor fg = green, bg = black;
static Point2d g_Pos (0, 0);

static void DrawBox (CGC& gc, Point2d pos)
{
    gc.Bar (pos[0], pos[1], 12, 4, gc.GraphicChar (acs_Board));
    gc.Box (pos[0], pos[1], 12, 4);
    gc.Bar (pos[0], pos[1] + 4, 4, 4, gc.GraphicChar (acs_Board));
    gc.Bar (pos[0] + 8, pos[1] + 4, 4, 4, gc.GraphicChar (acs_Board));
    gc.Bar (pos[0], pos[1] + 8, 12, 4, gc.GraphicChar (acs_Board));
    gc.Text (pos[0] + 1, pos[1] + 1, "GC demo");
    gc.FgColor (lightcyan);
    gc.Char (pos[0] + 1, pos[1] + 2, gc.GraphicChar(acs_LeftArrow));
    gc.Char (pos[0] + 2, pos[1] + 2, gc.GraphicChar(acs_DownArrow));
    gc.Char (pos[0] + 3, pos[1] + 2, gc.GraphicChar(acs_UpArrow));
    gc.Char (pos[0] + 4, pos[1] + 2, gc.GraphicChar(acs_RightArrow));
    gc.FgColor (green);
    gc.Text (pos[0] + 6, pos[1] + 2, "Move");
    gc.Text (pos[0] + 1, pos[1] + 3, "q to quit");
}

static void Draw (CGC& gc)
{
    gc.BgColor (black);
    gc.FgColor (green);
    gc.Clear();
    gc.BgColor (brown);
    gc.FgColor (black);
    DrawBox (gc, Point2d (10, 5));
    gc.BgColor (bg);
    gc.FgColor (fg);
    DrawBox (gc, g_Pos);
}

int main (void)
{
    CTerminfo ti;
    CKeyboard kb;
    ti.Load();
    kb.Open (ti);

    wchar_t key = 0;
    CGC gc, screen;
    gc.Resize (ti.Width(), ti.Height());
    screen.Resize (ti.Width(), ti.Height());

    cout << ti.HideCursor();
    while (key != 'q') {
	Draw (gc);
	gc.MakeDiffFrom (screen);
	cout << ti.Image (0, 0, gc.Width(), gc.Height(), gc.Canvas().begin());
	cout.flush();
	screen.Image (gc);
	gc.Image (screen);

	key = kb.GetKey();
	if ((key == kv_Up || key == 'k') && g_Pos[1] > 0)
	    -- g_Pos[1];
	else if ((key == kv_Down || key == 'j') && g_Pos[1] < gc.Height() - 12)
	    ++ g_Pos[1];
	else if ((key == kv_Left || key == 'h') && g_Pos[0] > 0)
	    -- g_Pos[0];
	else if ((key == kv_Right || key == 'l') && g_Pos[0] < gc.Width() - 12)
	    ++ g_Pos[0];
	else if (key == 'f')
	    fg += 1;
	else if (key == 'b')
	    bg += 1;
    }
    cout << ti.AllAttrsOff();
    cout << ti.Color (green, black) << endl;
    cout << ti.ShowCursor();
    return (0);
}

