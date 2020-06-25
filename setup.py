from   glob import glob
from   numpy.distutils.misc_util import get_numpy_include_dirs
from   setuptools import setup, Extension

setup(
    name            ="tabcsv",
    version         ="0.0.0",

    packages        =["tabcsv"],
    ext_modules     =[
        Extension(
            "tabcsv.ext",
            extra_compile_args  =["-std=c++14"],
            include_dirs        =[
                "./src",
                "./strtod",
                "./vendor/fast_double_parser/include",
                *get_numpy_include_dirs(),
            ],
            sources             =glob("tabcsv/ext/*.cc"),
            library_dirs        =["./src"],
            libraries           =["csv2"],
            depends             =glob("src/*.hh"),
        ),
    ],
)

