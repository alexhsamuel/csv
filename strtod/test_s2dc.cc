#include <cstring>
#include <iomanip>
#include <iostream>

#include "double-conversion/double-conversion.h"

static double_conversion::StringToDoubleConverter const 
S2DC{
  double_conversion::StringToDoubleConverter::NO_FLAGS,
  -100,
  -101,
  "infinity",
  "NaN",
  double_conversion::StringToDoubleConverter::kNoSeparator
};

int main(int const argc, char const* const* const argv)
{
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " NUMBER\n";
    return 1;
  }
  
  char const* const input = argv[1];
  int const length = strlen(input);
  std::cout << "input='" << input << "' length=" << length << "\n";
  int count = -1;
  double val = S2DC.StringToDouble(input, length, &count);
  std::cout << "count=" << count 
            << std::fixed << std::setw(30) << std::setprecision(20)
            << " val=" << val << "\n";
  return 0;
}

