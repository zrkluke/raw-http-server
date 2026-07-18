# Binary echo fixture

`image.bmp` is a valid 1×1, 24-bit BMP (58 bytes).  Its pixel bytes include
non-ASCII octets and the file contains NUL and LF bytes, so it must be loaded
as bytes rather than text.  `sha256.txt` records the exact fixture digest.

The TCP acceptance payload is `image.bmp` followed by the two raw bytes
`\r\n`.  That suffix is intentionally not part of the image file: it proves a
body may contain the delimiter used by the HTTP/1.1 text head without ending
the body early.
