# Task 5: How the extension works

## How Python reaches the C code
Python cannot call a C function directly, because every Python value is a PyObject on the heap and C functions expect raw types such as double. The CPython C API is the bridge. In faststats.c each exposed function has the signature `PyObject *fs_mean(PyObject *self, PyObject *args)`. Python passes the arguments as a tuple, and `PyArg_ParseTuple` unpacks them ("O" for any object, "d" for a C double). The function returns a new Python object (`PyFloat_FromDouble`, `PyLong_FromSsize_t`, `Py_BuildValue`) or returns NULL after setting an exception with `PyErr_SetString`, which Python then raises.

The functions are listed in a `PyMethodDef` table (name, C function, calling convention METH_VARARGS, docstring), which ends with a NULL sentinel. A `PyModuleDef` describes the module, and `PyInit_faststats` is the entry point the interpreter looks for when it executes `import faststats`. It must be named PyInit_ followed by the module name.

## Conversion and memory management
`to_doubles()` converts a Python sequence into a malloc'd array of C doubles. It rejects str and bytes, uses `PySequence_Fast` to get a list or tuple view, rejects bool explicitly, and calls `PyFloat_AsDouble` on each item. Two kinds of memory are involved. The C array is released with `free()` on every exit path, including errors. Python objects are managed by reference counting: `PySequence_Fast` returns a new reference, which is released with `Py_DECREF`. In `normalize`, the output list is released with `Py_DECREF` if creating an item fails, and `PyList_SET_ITEM` steals the reference to each item.

## From source to an importable module
`setup.py` uses setuptools. `python3 setup.py build_ext --inplace` runs two commands, both visible in compile_log.txt. First, gcc compiles faststats.c to an object file, using `-I/usr/include/python3.12` to find Python.h and `-fPIC` for position independent code. Second, gcc links it with `-shared` into `faststats.cpython-312-x86_64-linux-gnu.so`. The file name encodes the Python version and platform, which is how the import system finds a matching build. `import faststats` then loads the shared library, calls PyInit_faststats, and exposes the functions as module attributes.

## Trade-offs
- Speed: 7x to 47x faster in my measurements (see benchmark table).
- Safety: a bug in C (a missed free, a wrong reference count, a buffer overrun) can leak memory or crash the whole interpreter, where Python code would raise an exception.
- Portability: the .so is tied to one Python version and platform and must be rebuilt elsewhere. It needs a compiler and Python headers.
- Maintenance: the C version is about ten times longer than the Python one and harder to change.
- Memory: the C functions copy the data into a second array of doubles.
- Behaviour differences to control: Python compares int and float exactly, C compares after converting to double. Beyond 2^53 they can disagree.