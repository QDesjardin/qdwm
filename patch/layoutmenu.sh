#!/bin/sh

cat <<EOF | xmenu
[]= Tiled Layout	0
><> Floating Layout	1
[M] Monocle Layout	2
||| Columns Layout	3
>M> Floating Master Layout	4
[D] Deck Layout	5
TTT Bottom Stack Layout	6
=== Bottom Stack Horizontal Layout	7
|M| Centered Master Layout	8
-M- Centered Floating Master Layout	9
::: Gappless Grid Layout	10
[\\] Fibonacci Dwindle Layout	11
(@) Fibonacci Spiral Layout	12
EOF

