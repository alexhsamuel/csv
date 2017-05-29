- scientific notation
- `parse_double()` tests
  - test harness
  - basic test cases
  - very large and very small input cases
  - 10000000000000000000000000000000000000e-360
  - erroneous input cases
  - check last digit vs. `strtod()`
- `parse_double()` special case optimizations
  - optimize leading zeros (00000000001)
  - optimize leading decimal point (.123456)
  - optimize leading decimal point and zeros (0.00000000001)
- csv thread pool chunking... does it matter?


# Notes

Possibly, we don't need the `parse_double()` API with an end pointer.  Instead,
use the `strtod()` API and check that the returned end pointer is in the correct
place.  The problem is with the very last double in the file; `strtod()` may
read past the end of the buffer.

