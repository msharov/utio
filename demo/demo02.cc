#include <utio.h>
using namespace utio;
using namespace utio::gdt;

static EColor fg = green, bg = black;
static Point2d g_Pos (0, 0);

static void DrawBox (CGC& gc, Point2d pos)
{
    Rect outer (0, 0, 12, 12), inner (4, 4, 8, 8);
    outer += pos;
    inner += pos;
    gc.Bar (outer, gc.GraphicChar (acs_Board));
    gc.Bar (inner, 0);
    gc.Box (outer);
    gc.Box (inner);
    Point2d textPos (pos);
    ++ textPos[0];
    ++ textPos[1];
    gc.Text (textPos, "GC demo");
    ++ textPos[1];
    gc.FgColor (lightcyan);
    gc.Char (textPos[0], textPos[1], gc.GraphicChar(acs_LeftArrow));
    gc.Char (textPos[0] + 1, textPos[1], gc.GraphicChar(acs_DownArrow));
    gc.Char (textPos[0] + 2, textPos[1], gc.GraphicChar(acs_UpArrow));
    gc.Char (textPos[0] + 3, textPos[1], gc.GraphicChar(acs_RightArrow));
    gc.FgColor (green);
    gc.Text (textPos[0] + 5, textPos[1], "Move");
    ++ textPos[1];
    gc.Text (textPos, "q to quit");
}

static void Draw (CGC& gc)
{
    gc.Clear();
    gc.BgColor (yellow);
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
    CGC gc;
    gc.Resize (ti.Columns(), ti.Rows());

    cout << ti.HideCursor();
    while (key != 'q') {
	Draw (gc);
	cout << ti.Image (0, 0, gc.Width(), gc.Height(), gc.Canvas().begin());
	cout.flush();
	key = kb.GetKey();
	if (key == kv_Up && g_Pos[1] > 0)
	    -- g_Pos[1];
	else if (key == kv_Down && g_Pos[1] < gc.Height() - 12)
	    ++ g_Pos[1];
	else if (key == kv_Left && g_Pos[0] > 0)
	    -- g_Pos[0];
	else if (key == kv_Right && g_Pos[0] < gc.Width() - 12)
	    ++ g_Pos[0];
	else if (key == 'f')
	    fg += 1;
	else if (key == 'b')
	    bg += 1;
    }
    cout << ti.ShowCursor();
    return (0);
}

