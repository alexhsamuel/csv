import struct
import sys

_, base, exp10 = sys.argv
base = int(base)
exp10 = int(exp10)
print(f"base={base} exp10={exp10}")

if exp10 == 0:
    val = float(base)

elif exp10 > 0 and exp10.bit_length() + 10 * exp10 // 3 < 64:
    # Fits in a 64-bit integer.
    val = float(base * 10 ** exp10)

else:
    neg = base < 0
    if neg:
        base = -base

    mant = base
    exp2 = 0

    def show():
        print(f"{mant:088b} {exp2:3d} {mant * 2.0**exp2}")
    show()

    shift = 64
    mant = base << shift
    exp2 = -shift
    show()

    if exp10 > 0:
        mant *= 5 ** exp10
        exp2 += exp10
    else:
        pow5 = 5 ** -exp10
        # FIXME: Round?
        # rem = mant % pow5
        mant //= pow5
        # if rem > pow5 // 2:
        #     print("round up!")
        #     mant += 1
        exp2 -= -exp10
    show()

    # Shift mantissa to normalized position.  The MSB should be in position 53.
    bits = mant.bit_length()
    if bits >= 53:
        n = bits - 53
        m = (1 << n) - 1
        # FIXME: Does this rounding interact correctly with division rounding?
        drop = mant & m
        round_up = drop > (1 << (n - 1))
        mant >>= n
        exp2 += n
        show()
        if round_up:
            print(f"round up {drop:x} {1 << (n - 1):x}")
            mant += 1
        show()
    else:
        # FIXME: We shouldn't allow this to happen.
        print("WARNING: fix shift")
        mant <<= 53 - bits
        exp2 -= 53 - bits
        show()

    # Drop MSB; it's implied.
    assert mant.bit_length() == 53
    mant &= ~(1 << 52)
    exp2 += 52
    # Build the IEEE double.
    val = (
        ((1 if neg else 0) << 63)
        | ((exp2 + 1023) << 52)
        | mant
    )
    val, = struct.unpack("d", struct.pack("q", val))

act = float(f"{base}e{exp10}")
print(f"act {act:20.17e} {act.hex()}")
print(f"val {val:20.17e} {val.hex()}")


