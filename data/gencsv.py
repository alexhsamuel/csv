#!/usr/bin/env python3

import os
import sys

# For shame!
sys.path.append(os.path.expanduser("~/dev/aslib/python"))

import aslib.pandas.random as rnd

gen = rnd.dataframe(
    id      =rnd.cumsum(rnd.uniform_int(1, 10)),
    name    =rnd.word(8, upper=True),
    normal0 =rnd.normal(),
    normal1 =rnd.normal(),
    normal2 =rnd.normal(),
    normal3 =rnd.normal(),
    normal4 =rnd.normal(),
    normal5 =rnd.normal(),
    normal6 =rnd.normal(),
    normal7 =rnd.normal(),
    uniform =rnd.uniform(),
)

len = int(sys.argv[1])
df = gen(len)
df.index.name = "index"
df.to_csv(sys.stdout, float_format="%f")

