from   glob import glob
from   setuptools import setup, Extension

setup(
    name            ="tabcsv",
    version         ="0.0.0",

    packages        =["tabcsv"],
    ext_modules     =[
        Extension(
            "tabcsv.ext",
            extra_compile_args  =["-std=c++14"],
            include_dirs        =["./src"],
            sources             =glob("tabcsv/ext/*.cc"),
            library_dirs        =["./src"],
            libraries           =["csv2"],
            depends             =glob("src/*.hh"),
        ),
    ],
)

