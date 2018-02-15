# turbowalk

When recursing through a directory tree, one needs to know if a directory entry is a directory or not.
On windows, when reading a directory, the OS already provides that information but fs.readdir in node.js drops that information since it's API only provides file names.
So when implementing a recursion algorithm one has to stat each entry _again_, which has a significant performance cost.

This library implements directory recursion optimized for windows. I would assume an implementation for Linux and MacOS would not be slower than a readdir based approach though.

# Supported OS

* Windows

