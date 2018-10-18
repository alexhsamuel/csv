#include <assert.h>
#include <fcntl.h>
#include <Python.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL TABCSV_ARRAY_API
#include <numpy/arrayobject.h>

#include "ThreadPool.hh"
#include "csv2.hh"

PyObject*
load_file(
  char const* const path)
{
  int const fd = open(path, O_RDONLY);
  assert(fd >= 0);

  struct stat info;
  int res = fstat(fd, &info);
  if (res != 0)
    return PyErr_SetFromErrno(PyExc_IOError);
  std::cout << "st_size = " << info.st_size << std::endl;
  
  void* ptr = mmap(nullptr, info.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED)
    return PyErr_SetFromErrno(PyExc_IOError);

  Buffer buf{static_cast<char const*>(ptr), (size_t) info.st_size};

  auto const cols = split_columns(buf);
  std::cerr << "done with split\n";

  PyObject* ndas = PyDict_New();
  assert(ndas != NULL);

  {
    ThreadPool pool(5);
    std::vector<std::future<Array>> results;
    // FIXME: Use std::transform.
    for (auto const& col : cols)
      results.push_back(pool.enqueue(parse_array, &col, true));

    for (auto&& result : results) {
      Array const& arr = result.get();
      std::cout << arr;

      std::string name;
      PyObject* nda = NULL;

      if (arr.variant() == Array::VARIANT_INT) {
        auto const& int_arr = arr.int_arr();
        name = int_arr.name;
        npy_intp len = int_arr.len;
        nda = PyArray_EMPTY(1, &len, NPY_INT64, 0);
        auto const data = PyArray_DATA((PyArrayObject*) nda);
        memcpy(data, &int_arr.vals[0], len * sizeof(int64_t));
      }
      else if (arr.variant() == Array::VARIANT_FLOAT) {
        auto const& float_arr = arr.float_arr();
        name = float_arr.name;
        npy_intp len = float_arr.len;
        nda = PyArray_EMPTY(1, &len, NPY_FLOAT64, 0);
        auto const data = PyArray_DATA((PyArrayObject*) nda);
        memcpy(data, &float_arr.vals[0], len * sizeof(float64_t));
      }
      else if (arr.variant() == Array::VARIANT_STRING) {
        // FIXME
        Py_INCREF(Py_None);
        nda = Py_None;
      }
      else
        assert(false);
      PyDict_SetItemString(ndas, name.c_str(), nda);
    }
  }

  // FIXME: Do this earlier.
  res = munmap(ptr, info.st_size);
  if (res != 0) {
    PyErr_SetFromErrno(PyExc_IOError);
    close(fd);
    return NULL;
  }

  res = close(fd);
  if (res != 0)
    return PyErr_SetFromErrno(PyExc_IOError);

  return ndas;
}



//------------------------------------------------------------------------------

extern "C" {

static PyObject*
method_load_file(
  PyObject* const self,
  PyObject* const args,
  PyObject* const kw_args)
{
  static char const* const keywords[] = {"path", NULL};
  char const* path;

  if (!PyArg_ParseTupleAndKeywords(
        args, kw_args, "s", (char**) keywords, &path))
    return NULL;

  return load_file(path);
}


static PyMethodDef methods[] = {
  {"load_file", (PyCFunction) method_load_file, METH_VARARGS | METH_KEYWORDS, NULL},
  {NULL, NULL, 0, NULL}
};


static struct PyModuleDef
module_def = {
  PyModuleDef_HEAD_INIT,
  "tabcsv.ext",  
  NULL,
  -1,      
  methods,
};


PyMODINIT_FUNC
PyInit_ext(void)
{
  import_array();

  PyObject* module = PyModule_Create(&module_def);
  assert(module != NULL);
  return module;
}


}  // extern "C"

