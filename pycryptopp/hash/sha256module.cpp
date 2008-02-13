/**
 * sha256module.cpp -- Python wrappers around Crypto++'s SHA-256
 */

#include <Python.h>

#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#endif

/* from Crypto++ */
#ifdef USE_NAME_CRYPTO_PLUS_PLUS
#include "crypto++/sha.h"
#include "crypto++/hex.h"
#include "crypto++/filters.h"
#else
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#endif

static char sha256__doc__[] = "\
sha256 hash function\
";

static PyObject *sha256_error;

typedef struct {
    PyObject_HEAD

    /* internal */
    CryptoPP::SHA256* h;
    PyStringObject* digest;
} SHA256;

PyDoc_STRVAR(SHA256__doc__,
"a SHA256 hash object\n\
Its constructor takes an optional string, which has the same effect as\n\
calling .update() with that string.");

static PyObject *
SHA256_update(SHA256* self, PyObject* msgobj) {
    if (self->digest)
        return PyErr_Format(sha256_error, "Precondition violation: once .digest() has been called you are required to never call .update() again.");

    const char *msg;
    Py_ssize_t msgsize;
    if (PyString_AsStringAndSize(msgobj, const_cast<char**>(&msg), &msgsize))
        return NULL;
    self->h->Update(reinterpret_cast<const byte*>(msg), msgsize);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(SHA256_update__doc__,
"Update the hash object with the string msg. Repeated calls are equivalent to\n\
a single call with the concatenation of all the messages.");

static PyObject *
SHA256_digest(SHA256* self, PyObject* dummy) {
    if (!self->digest) {
        self->digest = reinterpret_cast<PyStringObject*>(PyString_FromStringAndSize(NULL, self->h->DigestSize()));
        if (!self->digest)
            return NULL;
        self->h->Final(reinterpret_cast<byte*>(PyString_AS_STRING(self->digest)));
    }

    Py_INCREF(self->digest);
    return reinterpret_cast<PyObject*>(self->digest);
}

PyDoc_STRVAR(SHA256_digest__doc__,
"Return the binary digest of the messages that were passed to the update()\n\
method (including the initial message if any).");

static PyObject *
SHA256_hexdigest(SHA256* self, PyObject* dummy) {
	PyObject* digest = SHA256_digest(self, NULL);
	if (!digest)
		return NULL;
	Py_ssize_t dsize = PyString_GET_SIZE(digest);
	PyStringObject* hexdigest = reinterpret_cast<PyStringObject*>(PyString_FromStringAndSize(NULL, dsize*2));
	CryptoPP::ArraySink* as = new CryptoPP::ArraySink(reinterpret_cast<byte*>(PyString_AS_STRING(hexdigest)), dsize*2);
	CryptoPP::HexEncoder enc;
	enc.Attach(as);
	enc.Put(reinterpret_cast<const byte*>(PyString_AS_STRING(digest)), static_cast<size_t>(dsize));

    return reinterpret_cast<PyObject*>(hexdigest);
}

PyDoc_STRVAR(SHA256_hexdigest__doc__,
"Return the hex-encoded digest of the messages that were passed to the update()\n\
method (including the initial message if any).");

static PyMethodDef SHA256_methods[] = {
    {"update", reinterpret_cast<PyCFunction>(SHA256_update), METH_O, SHA256_update__doc__},
    {"digest", reinterpret_cast<PyCFunction>(SHA256_digest), METH_NOARGS, SHA256_digest__doc__},
    {"hexdigest", reinterpret_cast<PyCFunction>(SHA256_hexdigest), METH_NOARGS, SHA256_digest__doc__},
    {NULL},
};

static PyObject *
SHA256_new(PyTypeObject* type, PyObject *args, PyObject *kwdict) {
    SHA256* self = reinterpret_cast<SHA256*>(type->tp_alloc(type, 0));
    if (!self)
        return NULL;
    self->h = new CryptoPP::SHA256();
    if (!self->h)
        return PyErr_NoMemory();
    self->digest = NULL;
    return reinterpret_cast<PyObject*>(self);
}

static void
SHA256_dealloc(SHA256* self) {
    Py_XDECREF(self->digest);
    delete self->h;
    self->ob_type->tp_free((PyObject*)self);
}

static int
SHA256_init(PyObject* self, PyObject *args, PyObject *kwdict) {
    static char *kwlist[] = { "msg", NULL };
    const char *msg = NULL;
    size_t msgsize;
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "|t#", const_cast<char**>(kwlist), &msg, &msgsize))
        return -1;

    if (msg)
        reinterpret_cast<SHA256*>(self)->h->Update(reinterpret_cast<const byte*>(msg), msgsize);
    return 0;
}

static PyTypeObject SHA256_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "sha256.SHA256", /*tp_name*/
    sizeof(SHA256),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    reinterpret_cast<destructor>(SHA256_dealloc), /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    SHA256__doc__,           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SHA256_methods,      /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    //reinterpret_cast<initproc>(SHA256_init),               /* tp_init */
    SHA256_init,               /* tp_init */
    0,                         /* tp_alloc */
    SHA256_new,                /* tp_new */
};

static struct PyMethodDef sha256_functions[] = {
    {NULL,     NULL}            /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initsha256(void) {
    PyObject *module;
    PyObject *module_dict;

    if (PyType_Ready(&SHA256_type) < 0)
        return;

    module = Py_InitModule3("sha256", sha256_functions, sha256__doc__);
    if (!module)
      return;

    Py_INCREF(&SHA256_type);

    PyModule_AddObject(module, "SHA256", (PyObject *)&SHA256_type);

    module_dict = PyModule_GetDict(module);
    sha256_error = PyErr_NewException("sha256.Error", NULL, NULL);
    PyDict_SetItemString(module_dict, "Error", sha256_error);
}
