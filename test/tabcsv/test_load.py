import csv
from   pathlib import Path
import tabcsv

def compare_to_csv(path):
    with open(path) as file:
        reader = csv.reader(file)
        header = next(reader)
        csv_arrs = dict(zip(header, zip(*reader)))

    arrs = tabcsv.load_file(path)

    # Same keys.
    assert len(arrs) == len(csv_arrs)
    assert list(arrs) == list(csv_arrs)

    length = len(next(iter(csv_arrs.values())))
    assert all( len(a) == length for a in csv_arrs.values() )

    for name, arr in arrs.items():
        assert len(arr) == length

        csv_arr = csv_arrs[name]
        for a, b in zip(arr, csv_arr):
            assert a == type(a)(b)


def test_basic0():
    compare_to_csv(Path(__file__).parent / "basic0.csv")


