#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>

/* Convert a Python sequence of int/float into a malloc'd array of doubles.
   On success returns the array and stores the length in *n_out.
   On failure returns NULL with a Python exception set.
   The caller must free() the returned array.
   An empty sequence returns a valid non-NULL pointer with *n_out == 0. */
static double *to_doubles(PyObject *obj, Py_ssize_t *n_out)
{
    if (PyUnicode_Check(obj) || PyBytes_Check(obj)) {
        PyErr_SetString(PyExc_TypeError,
                        "data must be a sequence of numbers, not a string");
        return NULL;
    }

    PyObject *seq = PySequence_Fast(obj, "data must be a sequence of numbers");
    if (seq == NULL) {
        return NULL;
    }

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    PyObject **items = PySequence_Fast_ITEMS(seq);

    /* malloc(0) may return NULL, so always ask for at least one slot. */
    double *buf = malloc((n > 0 ? (size_t)n : 1) * sizeof(double));
    if (buf == NULL) {
        Py_DECREF(seq);
        PyErr_NoMemory();
        return NULL;
    }

    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = items[i];

        /* bool is a subclass of int, so reject it explicitly. */
        if (PyBool_Check(item) ||
            !(PyLong_Check(item) || PyFloat_Check(item))) {
            PyErr_Format(PyExc_TypeError,
                         "all values must be int or float, got %.100s",
                         Py_TYPE(item)->tp_name);
            free(buf);
            Py_DECREF(seq);
            return NULL;
        }

        double v = PyFloat_AsDouble(item);
        if (v == -1.0 && PyErr_Occurred()) {
            free(buf);
            Py_DECREF(seq);
            return NULL;
        }
        buf[i] = v;
    }

    Py_DECREF(seq);
    *n_out = n;
    return buf;
}

/* Sum of buf[0..n-1] divided by n. Caller guarantees n > 0. */
static double mean_of(const double *buf, Py_ssize_t n)
{
    double total = 0.0;
    for (Py_ssize_t i = 0; i < n; i++) {
        total += buf[i];
    }
    return total / (double)n;
}

/* mean(data) -> float */
static PyObject *fs_mean(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    Py_ssize_t n;
    double *buf = to_doubles(data, &n);
    if (buf == NULL) {
        return NULL;
    }
    if (n == 0) {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "mean of empty dataset");
        return NULL;
    }

    double result = mean_of(buf, n);
    free(buf);
    return PyFloat_FromDouble(result);
}

/* variance(data) -> float (population variance, divides by n) */
static PyObject *fs_variance(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    Py_ssize_t n;
    double *buf = to_doubles(data, &n);
    if (buf == NULL) {
        return NULL;
    }
    if (n == 0) {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "variance of empty dataset");
        return NULL;
    }

    double m = mean_of(buf, n);
    double acc = 0.0;
    for (Py_ssize_t i = 0; i < n; i++) {
        double d = buf[i] - m;
        acc += d * d;
    }
    free(buf);
    return PyFloat_FromDouble(acc / (double)n);
}

/* minmax(data) -> (float, float) */
static PyObject *fs_minmax(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    Py_ssize_t n;
    double *buf = to_doubles(data, &n);
    if (buf == NULL) {
        return NULL;
    }
    if (n == 0) {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "minmax of empty dataset");
        return NULL;
    }

    double lo = buf[0], hi = buf[0];
    for (Py_ssize_t i = 1; i < n; i++) {
        if (buf[i] < lo) lo = buf[i];
        if (buf[i] > hi) hi = buf[i];
    }
    free(buf);
    return Py_BuildValue("(dd)", lo, hi);
}

/* count_outside(data, low, high) -> int
   Counts values with x < low or x > high. */
static PyObject *fs_count_outside(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *data;
    double low, high;
    if (!PyArg_ParseTuple(args, "Odd", &data, &low, &high)) {
        return NULL;
    }

    Py_ssize_t n;
    double *buf = to_doubles(data, &n);
    if (buf == NULL) {
        return NULL;
    }
    if (low > high) {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "low must not be greater than high");
        return NULL;
    }

    Py_ssize_t count = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (buf[i] < low || buf[i] > high) {
            count++;
        }
    }
    free(buf);
    return PyLong_FromSsize_t(count);
}

/* normalize(data) -> list of float, scaled to [0, 1] */
static PyObject *fs_normalize(PyObject *self, PyObject *args)
{
    (void)self;
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }

    Py_ssize_t n;
    double *buf = to_doubles(data, &n);
    if (buf == NULL) {
        return NULL;
    }
    if (n == 0) {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "normalize of empty dataset");
        return NULL;
    }

    double lo = buf[0], hi = buf[0];
    for (Py_ssize_t i = 1; i < n; i++) {
        if (buf[i] < lo) lo = buf[i];
        if (buf[i] > hi) hi = buf[i];
    }
    double span = hi - lo;

    PyObject *out = PyList_New(n);
    if (out == NULL) {
        free(buf);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < n; i++) {
        double v = (span == 0.0) ? 0.0 : (buf[i] - lo) / span;
        PyObject *item = PyFloat_FromDouble(v);
        if (item == NULL) {
            Py_DECREF(out);   /* frees the items already stored */
            free(buf);
            return NULL;
        }
        PyList_SET_ITEM(out, i, item);   /* steals the reference */
    }

    free(buf);
    return out;
}

static PyMethodDef FastStatsMethods[] = {
    {"mean", fs_mean, METH_VARARGS, "mean(data) -> float\n\nArithmetic mean."},
    {"variance", fs_variance, METH_VARARGS,
     "variance(data) -> float\n\nPopulation variance (divides by n)."},
    {"minmax", fs_minmax, METH_VARARGS,
     "minmax(data) -> (min, max)\n\nMinimum and maximum as floats."},
    {"count_outside", fs_count_outside, METH_VARARGS,
     "count_outside(data, low, high) -> int\n\nCount values below low or above high."},
    {"normalize", fs_normalize, METH_VARARGS,
     "normalize(data) -> list\n\nScale values to [0, 1] using min and max."},
    {NULL, NULL, 0, NULL}   /* sentinel: marks the end of the table */
};

static struct PyModuleDef faststats_module = {
    PyModuleDef_HEAD_INIT,
    "faststats",                              /* module name */
    "Fast statistics implemented in C.",      /* docstring */
    -1,                                       /* no per-interpreter state */
    FastStatsMethods,
    NULL, NULL, NULL, NULL                    /* m_slots, m_traverse, m_clear, m_free */
};

PyMODINIT_FUNC PyInit_faststats(void)
{
    return PyModule_Create(&faststats_module);
}