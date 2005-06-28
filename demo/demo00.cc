#include <utio.h>
using namespace utio;

int main (void)
{
    CTerminfo ti;
    ti.Load();

    cout << ios::hex;
    for (int y = black; y < color_Last; ++ y) {
	for (int x = black; x < color_Last; ++ x)
	    cout << ti.Color (EColor(x), EColor(y)) << x;
	cout << ti.Color (green, black) << "\tHello World!" << endl;
    }
    cout << ti.AllAttrsOff();
    return (0);
}

