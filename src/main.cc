extern load_file(char const*);

//------------------------------------------------------------------------------

int
main(
  int const argc,
  char const* const* const argv)
{
  load_file(argv[1]);
  return 0;
}


