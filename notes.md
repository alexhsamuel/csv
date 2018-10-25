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
sys	    0m0.382s
```

- With threads, 3.9 s.
- Add simpler columns, 3.5 s.
- Add bitfields: 3.0s.

---

```
$ time src/csv1 data/gencsv-10m.csv 
st_size = 2019873529
done with split
int64 column 'index' len=10000001 min=0 max=9999999
int64 column 'id' len=10000001 min=4 max=50006625
str column len=10000001 width=8
float64 column 'normal0' len=10000001 min=-5.09962 max=5.22694
float64 column 'normal1' len=10000001 min=-5.50601 max=5.22783
float64 column 'normal2' len=10000001 min=-5.02299 max=5.26825
float64 column 'normal3' len=10000001 min=-5.24478 max=5.28144
float64 column 'normal4' len=10000001 min=-5.71266 max=5.24464
float64 column 'normal5' len=10000001 min=-5.11398 max=5.47894
float64 column 'normal6' len=10000001 min=-5.15393 max=5.14644
float64 column 'normal7' len=10000001 min=-4.98027 max=5.19251
float64 column 'uniform' len=10000001 min=1.52576e-07 max=1

real	0m31.571s
user	1m25.400s
sys	    0m3.729s
```

Pandas:

```
In [2]: %timeit pandas.read_csv("data/gencsv-10m.csv")
42.9 s ± 578 ms per loop (mean ± std. dev. of 7 runs, 1 loop each)
```

---

# API?

Requirements:
- Delimiter.
- How repeated delimiters are handled, particularly (or only) spaces.
- Quoting.  (And escaping?)
- Whether to convert columns to typed arrs.  Separate function?
- Columns to include or skip.
- Specify column names or use from file.
- Types of columns, or convert automatically.
- Fixed-length or object strings?  Or cut over automatically?
- mmap- and non mmap-based APIs.
- UTF-8 support

```py
from ntab.io.delimited import parse_arrays

arrs = parse_arrays(
  path,
  types={},
  ...
)
```

---

# strtod

- https://cs.stackexchange.com/questions/80952/convert-a-decimal-floating-point-number-into-a-binary-floating-point-number

- http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.451.6099&rep=rep1&type=pdf

Jaffer:
- https://arxiv.org/pdf/1310.8121v6.pdf

Gay:
- http://www.netlib.org/fp/

