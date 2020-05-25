# Useful Terminal I/O Library

utio is a terminal text graphics library using terminfo descriptions.
It is intended as a C++ alternative to curses.

Compilation requires [uSTL library 2.6+](https://github.com/msharov/ustl),
and a c++14-supporting compiler, such as gcc 5.

```sh
% ./configure && make && make install
```

Running utio applications requires the terminfo database that
is usually distributed as part of the ncurses package and is
installed in /usr/share/terminfo.

See [docs/index.html](https://msharov.github.io/utio) for API description.

Report bugs on the [project bugtracker](https://github.com/msharov/utio/issues)
