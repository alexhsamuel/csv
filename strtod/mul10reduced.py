import random
import struct
import sys

def scale(base, exp10, *, debug=False):
    if exp10 == 0:
        return float(base)

    elif exp10 > 0 and base.bit_length() + 10 * exp10 // 3 < 64:
        # Fits in a 64-bit integer.
        return float(base * 10 ** exp10)

    else:
        neg = base < 0
        if neg:
            base = -base

        def show():
            if debug:
                print(f"{mant:0128b} {exp2:4d} {mant * 2.0**exp2}")

        if exp10 > 0:
            mant = base * 5 ** exp10
            exp2 = exp10
            show()

        else:
            shift = 127 - base.bit_length()
            pow5 = 5 ** -exp10
            mant = (base << shift) // pow5
            exp2 = exp10 - shift
            show()

        # Shift mantissa to normalized position.  The MSB should be in position
        # 53.
        bits = mant.bit_length()
        if bits == 53:
            pass
        elif bits > 53:
            n = bits - 53
            round_up = (mant >> (n - 1)) & 1 > 0
            mant >>= n
            exp2 += n
            show()
            if round_up:
                mant += 1
            show()
        else:
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
        return val


if __name__ == "__main__":
    if len(sys.argv) == 3:
        _, base, exp10 = sys.argv
        base = int(base)
        exp10 = int(exp10)
        print(f"base={base} exp10={exp10}")

        act = float(f"{base}e{exp10}")
        val = scale(base, exp10, debug=True)
        print(f"act {act:23.30e} {act.hex()}")
        print(f"val {val:23.30e} {val.hex()}")

    else:
        for _ in range(100000):
            base = random.randint(1, 10 ** random.randint(1, 16))
            exp10 = random.randint(-17, 17)
            if exp10 >= 0:
                dec = str(base * 10 ** exp10)
            elif exp10 < 0:
                dec = str(base)
                if len(dec) < -exp10:
                    dec = "0" * (-exp10 - len(dec)) + dec
                dec = dec[: exp10] + "." + dec[exp10 :]

            if len(dec) > 18:
                continue

            act = float(dec)
            val = scale(base, exp10)
            if act != val:
                print(f"{base:20d} {exp10:3d} {dec:40s} {act:23.30e} {val:23.30e}")

