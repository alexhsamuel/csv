```py
In [2]: %timeit pandas.read_csv("data/gencsv-1m.csv")
4.45 s ± 34.4 ms per loop (mean ± std. dev. of 7 runs, 1 loop each)
```

commit 7398cf6
```py
v$ time src/csv1 data/gencsv-1m.csv 
st_size = 199986495
int64 column len=1000001 min=0 max=999999
int64 column len=1000001 min=6 max=4996954
str column len=1000001 width=8
float64 column len=1000001 min=-4.68093 max=4.73605
float64 column len=1000001 min=-4.93754 max=4.91713
float64 column len=1000001 min=-5.20252 max=5.04421
float64 column len=1000001 min=-4.67527 max=5.12737
float64 column len=1000001 min=-4.91289 max=5.1631
float64 column len=1000001 min=-4.74838 max=5.16228
float64 column len=1000001 min=-5.01482 max=4.61823
float64 column len=1000001 min=-4.43455 max=4.56303
float64 column len=1000001 min=1.29199e-06 max=0.999998

real	0m5.702s
user	0m5.287s
sys	0m0.382s
```

