# x-screenshot
takes a screenshot of a window and dumps it to standard output

# installation

```
$ make
# make install   # or just put the executable somewhere in your local path
# make uninstall # to remove
```

# usage

Screenshot of the root window (i.e., everything):

```
$ x-screenshot > /path/to/destination.png
$ x-screenshot | feh --scale-down -
```

Screenshot of a window:

```
$ wmctrl -l # pick a window ID from the list
...
$ x-screenshot ID | feh --scale-down -

```

# requirements

Xlib, libpng