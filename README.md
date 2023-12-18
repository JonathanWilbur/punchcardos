# Simpler Core Utils

This repo contains lightweight, simple rewrites of a select few of the GNU core
utils for the purposes of implementing an extremely small, almost unusable,
Linux distro that is only capable enough to minimally write C code, compile it,
hash it, encrypt it, and send it over HTTP, SMTP, or FTP.

Every program in the `programs/` folder is a single-file. Using `ls` as an
example, Compilation is as simple as:

```bash
gcc programs/ls.c -o ls
```

This Linux-distro-in-progress is called "PunchcardOS," harkening back to the
bootstrapping of computing by the use of
[punchcard programs](https://en.wikipedia.org/wiki/Punched_card).

## PunchcardOS

Provided that it proves to be feasible, PunchcardOS will start with
[Tilck](https://github.com/vvaltchev/tilck) as a lighter alternative to Linux,
[ChibiCC](https://github.com/rui314/chibicc) as a minimal C compiler, possibly
[NoLibC](https://github.com/wtarreau/nolibc) as a C library,
[lsh](https://github.com/brenns10/lsh) as a shell, and
[kilo](https://github.com/antirez/kilo) as a minimal text editor. With these
pre-compiled binaries alone, it should be possible to write and compile
further C programs.

## License

Some of the software in this repository was copied from other repositories, and
are governed by a different result. Unless otherwise noted, all software in
this repo is licensed under the MIT License whose text here:

```
MIT License

Copyright 2023 Jonathan M. Wilbur <jonathan@wilbur.space>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## See Also

Similar work has been done here: https://github.com/keiranrowan/tiny-core
