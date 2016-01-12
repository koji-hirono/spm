spm
===

Shogi replay and postmortem client for Unix

Requirements
------------
* USI engines
* Kifu (USI format)

Feature
-------
* Simple command line interface
* Analize kifu by USI engine

Usage
-----

```
$ spm [-d <debug level>] -e <engine_path> [-s <SFEN>] [<KIFUFILE>]
```

Screenshot
-----------

```
black(80/180)? s
#80/180 white: 8d7e (132)
9 8 7 6 5 4 3 2 1
. n . . . . . n l  a
. r . . . g k . .  b
. . . . s p . p .  c
. . P . p s p . p  d
l . g p . . b . .  e
p P R . P . . . .  f
. . N . . P P . P  g
. . G . S . S K .  h
L . . . . G . N L  i
black: Px2
white: Px3 Bx1

black(80/180)?
```

```
black(80/180)? l
  1. B 2g2f  (     0) 7g7f
  2. W 8c8d  (     0) 3c3d
  3. B 2f2e  (     0) 2f2e  o
...snip...
 65. B 7f7e  (   -50) P*7d
 66. W 8e8f  (    66) 8b7b
 67. B 8g8f  (    24) 7g8e
 68. W P*7d  (  -109) P*7d
 69. B 7e7f  (  -109) 7e7f  o
 70. W 9e9f  (  -117) 9e9f  o
 71. B P*9e  (   -66) 3g3f
 72. W 9d9e  (   -66) 9d9e  o
 73. B P*7e  (  -367) P*9d  ?
 74. W 7c8d  (  -367) 7c8d  o
 75. B 3i2h  (  -355) 3i2h  o
 76. W 6d6e  (  -222) 6d6e  o
 77. B 7e7d  (   -54) 7e7d  o
 78. W P*7e  (   113) 5c6d
 79. B 4h7e  (   113) 4h7e  o
 80. W 8d7e  (   132) 8d7e  o
black(80/180)?
```

License
-------
This software is released under the MIT License, see *LICENSE*
