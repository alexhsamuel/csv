#include <assert.h>
#include <Python.h>

#include "csv2.hh"


extern "C" {

static PyObject*
method_load_file(
  PyObject* const self,
  PyObject* const args,
  PyObject* const kw_args)
{
  static char* keywords[] = {"path", NULL};
  char const* path;

  if (!PyArg_ParseTupleAndKeywords(args, kw_args, "s", keywords, &path))
    return NULL;

  fprintf(stderr, "path: %s\n", path);
  load_file(path);

  Py_INCREF(Py_None);
  return Py_None;
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
  PyObject* module = PyModule_Create(&module_def);
  assert(module != NULL);
  return module;
}


}  // extern "C"

