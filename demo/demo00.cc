#include <utio.h>
using namespace utio;

int main (void)
{
    CTerminfo ti;
    ti.Load();

    cout << ios::hex;
    for (uint32_t y = black; y < color_Last; ++ y) {
	for (uint32_t x = black; x < color_Last; ++ x)
	    cout << ti.Color (EColor(x), EColor(y)) << x;
	cout << ti.Color (green, black);
	cout << "\tHello World!";
	cout << ti.AllAttrsOff();
	cout << endl;
    }
    return (0);
}

