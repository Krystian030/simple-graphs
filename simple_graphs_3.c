#include <Python.h>
#include "structmember.h"

typedef struct Node
{
    int vertex;
    struct Node *next;
} Node;

typedef struct
{
    PyObject_HEAD int num_vertices;
    Node *adj_list[16];
} AdjacencyList;

Node *createNode(int v)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    newNode->vertex = v;
    newNode->next = NULL;
    return newNode;
}

Node **initializeAdjList()
{
    Node **adj_list = (Node **)malloc(16 * sizeof(Node *));
    if (adj_list == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < 16; i++)
    {
        adj_list[i] = NULL;
    }
    return adj_list;
}

void addEdge(AdjacencyList *self, int src, int dest)
{
    if (src > dest)
    {
        int temp = src;
        src = dest;
        dest = temp;
    }

    Node *newNode = createNode(dest);
    newNode->next = self->adj_list[src];
    self->adj_list[src] = newNode;

    newNode = createNode(src);
    newNode->next = self->adj_list[dest];
    self->adj_list[dest] = newNode;
}

static void AdjacencyList_dealloc(AdjacencyList *self)
{
    for (int i = 0; i < 16; i++)
    {
        Node *current = self->adj_list[i];
        while (current != NULL)
        {
            Node *next = current->next;
            free(current);
            current = next;
        }
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *AdjacencyList_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    AdjacencyList *self;
    self = (AdjacencyList *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->num_vertices = 0;
        for (int i = 0; i < 16; i++)
        {
            self->adj_list[i] = NULL;
        }
    }
    return (PyObject *)self;
}

static int AdjacencyList_init(AdjacencyList *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char *text = "?";

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &text))
    {
        return -1;
    }

    if (text[0] != '?')
    {
        int num_vertices = text[0] - 63;

        if (num_vertices <= 0)
        {
            PyErr_SetString(PyExc_ValueError, "Invalid number of vertices in g6 format");
            return -1;
        }

        self->num_vertices = num_vertices;

        int c = 0;
        int i = 1;
        int k = 0;

        for (int v = 1; v < num_vertices; v++)
        {
            for (int u = 0; u < v; u++)
            {
                if (k == 0)
                {
                    c = text[i++] - 63;
                    k = 6;
                }
                k--;

                if ((c & (1 << k)) != 0)
                {
                    addEdge(self, u, v);
                }
            }
        }
    }

    return 0;
}

static PyObject *number_of_vertices(AdjacencyList *self)
{
    return PyLong_FromLong(self->num_vertices);
}

static PyObject *vertices(AdjacencyList *self)
{
    PyObject *vertex_set = PySet_New(NULL);

    if (vertex_set == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create vertex set");
        return NULL;
    }

    for (int i = 0; i < self->num_vertices; i++)
    {
        PyObject *vertex = PyLong_FromLong(i);

        if (vertex == NULL)
        {
            Py_DECREF(vertex_set);
            PyErr_SetString(PyExc_RuntimeError, "Failed to create vertex object");
            return NULL;
        }
        PySet_Add(vertex_set, vertex);
        Py_DECREF(vertex);
    }

    return vertex_set;
}

static PyObject *vertex_degree(AdjacencyList *self, PyObject *args)
{
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex))
    {
        return NULL;
    }

    if (vertex < 0 || vertex >= self->num_vertices)
    {
        PyErr_SetString(PyExc_ValueError, "Vertex index out of range");
        return NULL;
    }

    int degree = 0;

    // Przechodzimy przez listę sąsiadów danego wierzchołka
    Node *neighbors = self->adj_list[vertex];
    while (neighbors != NULL)
    {
        degree++;
        neighbors = neighbors->next;
    }

    return PyLong_FromLong(degree);
}

static PyObject *vertex_neighbors(AdjacencyList *self, PyObject *args)
{
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex))
    {
        return NULL;
    }

    if (vertex < 0 || vertex >= self->num_vertices)
    {
        PyErr_SetString(PyExc_ValueError, "Vertex index out of range");
        return NULL;
    }

    PyObject *neighbors_set = PySet_New(NULL);

    if (neighbors_set == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create neighbors set");
        return NULL;
    }

    for (int u = 0; u < self->num_vertices; u++)
    {
        if (u == vertex)
        {
            continue; // Ignorujemy wierzchołek, dla którego zbieramy sąsiadów
        }

        Node *current = self->adj_list[u];

        while (current != NULL)
        {
            int v = current->vertex;

            if (u == vertex || v == vertex)
            {
                // Dodajemy do zbioru tylko wierzchołki sąsiadujące z 'vertex'
                int neighbor = (u == vertex) ? v : u;
                PyObject *neighbor_py = PyLong_FromLong(neighbor);

                if (neighbor_py == NULL)
                {
                    Py_DECREF(neighbors_set);
                    PyErr_SetString(PyExc_RuntimeError, "Failed to create neighbor object");
                    return NULL;
                }

                int result = PySet_Add(neighbors_set, neighbor_py);
                Py_DECREF(neighbor_py);

                if (result < 0)
                {
                    Py_DECREF(neighbors_set);
                    PyErr_SetString(PyExc_RuntimeError, "Failed to add neighbor to neighbors set");
                    return NULL;
                }
            }

            current = current->next;
        }
    }

    return neighbors_set;
}

static PyObject *add_vertex(AdjacencyList *self, PyObject *args)
{
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex))
    {
        return NULL;
    }

    if (vertex < 0 || vertex >= 16)
    {
        PyErr_SetString(PyExc_ValueError, "Vertex index out of range (must be between 0 and 15)");
        return NULL;
    }

    // Sprawdzamy, czy wierzchołek już istnieje
    if (self->adj_list[vertex] != NULL)
    {
        return NULL;
    }

    Node *newNode = createNode(vertex);
    if (newNode == NULL)
    {
        return NULL;
    }

    self->adj_list[vertex] = newNode;

    self->num_vertices++;

    return PyLong_FromLong(1);
}

static PyObject *delete_vertex(AdjacencyList *self, PyObject *args)
{
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex))
    {
        return NULL;
    }

    if (vertex < 0 || vertex >= self->num_vertices)
    {
        PyErr_SetString(PyExc_ValueError, "Vertex index out of range");
        return NULL;
    }

    // Usuwamy wierzchołek z list sąsiedztwa innych wierzchołków
    for (int i = 0; i < self->num_vertices; i++)
    {
        if (i == vertex)
        {
            // Pomijamy usuwanie sąsiada samego siebie
            continue;
        }

        Node *prev = NULL;
        Node *current = self->adj_list[i];

        // Szukamy wierzchołka do usunięcia z listy sąsiedztwa
        while (current != NULL)
        {
            if (current->vertex == vertex)
            {
                // Usuwamy wierzchołek z listy sąsiedztwa
                if (prev == NULL)
                {
                    self->adj_list[i] = current->next;
                }
                else
                {
                    prev->next = current->next;
                }
                Node *temp = current;
                current = current->next;
                free(temp);
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    }

    // Usuwamy wszystkie krawędzie incydentne z list sąsiedztwa danego wierzchołka
    Node *current = self->adj_list[vertex];
    while (current != NULL)
    {
        Node *next = current->next;
        free(current);
        current = next;
    }

    // Ustawiamy listę sąsiedztwa danego wierzchołka na NULL
    self->adj_list[vertex] = NULL;

    // Zmniejszamy liczbę wierzchołków
    self->num_vertices--;

    Py_RETURN_NONE;
}

static PyObject *number_of_edges(AdjacencyList *self)
{
    int edge_count = 0;

    for (int i = 0; i < self->num_vertices; i++)
    {
        Node *current = self->adj_list[i];

        while (current != NULL)
        {
            edge_count++;
            current = current->next;
        }
    }

    return PyLong_FromLong(edge_count / 2);
}

static PyObject *edges(AdjacencyList *self)
{
    PyObject *edges_set = PySet_New(NULL);

    if (edges_set == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create edges set");
        return NULL;
    }

    for (int u = 0; u < self->num_vertices; u++)
    {
        Node *current = self->adj_list[u];

        while (current != NULL)
        {
            int v = current->vertex;

            // Dodajemy krawędź tylko jeśli u < v, aby uniknąć powtórzeń
            if (u < v)
            {
                PyObject *edge_tuple = Py_BuildValue("(ii)", u, v);
                if (edge_tuple == NULL)
                {
                    Py_DECREF(edges_set);
                    PyErr_SetString(PyExc_RuntimeError, "Failed to create edge tuple");
                    return NULL;
                }

                int result = PySet_Add(edges_set, edge_tuple);
                Py_DECREF(edge_tuple);

                if (result < 0)
                {
                    Py_DECREF(edges_set);
                    PyErr_SetString(PyExc_RuntimeError, "Failed to add edge tuple to edges set");
                    return NULL;
                }
            }

            current = current->next;
        }
    }

    return edges_set;
}

static PyObject *is_edge(AdjacencyList *self, PyObject *args)
{
    int u, v;

    if (!PyArg_ParseTuple(args, "ii", &u, &v))
    {
        return NULL;
    }

    if (u < 0 || u >= self->num_vertices || v < 0 || v >= self->num_vertices)
    {
        return PyBool_FromLong(0);
    }

    int from = (u < v) ? u : v;
    int to = (u < v) ? v : u;

    Node *current = self->adj_list[from];
    while (current != NULL)
    {
        if (current->vertex == to)
        {
            return PyBool_FromLong(1);
        }
        current = current->next;
    }

    return PyBool_FromLong(0);
}

static PyObject *add_edge(AdjacencyList *self, PyObject *args)
{
    return NULL;
}

static PyObject *delete_edge(AdjacencyList *self, PyObject *args)
{
    return NULL;
}

static PyObject *is_bipartite(AdjacencyList *self)
{
    return NULL;
}

static PyMethodDef AdjacencyList_methods[] = {
    {"number_of_vertices", (PyCFunction)number_of_vertices, METH_NOARGS},
    {"vertices", (PyCFunction)vertices, METH_NOARGS},
    {"vertex_degree", (PyCFunction)vertex_degree, METH_VARARGS},
    {"vertex_neighbors", (PyCFunction)vertex_neighbors, METH_VARARGS},
    {"add_vertex", (PyCFunction)add_vertex, METH_VARARGS},
    {"delete_vertex", (PyCFunction)delete_vertex, METH_VARARGS},
    {"number_of_edges", (PyCFunction)number_of_edges, METH_NOARGS},
    {"edges", (PyCFunction)edges, METH_NOARGS},
    {"is_edge", (PyCFunction)is_edge, METH_VARARGS},
    {"add_edge", (PyCFunction)add_edge, METH_VARARGS},
    {"delete_edge", (PyCFunction)delete_edge, METH_VARARGS},
    {"is_bipartite", (PyCFunction)is_bipartite, METH_NOARGS},
    {NULL, NULL}};

static PyTypeObject AdjacencyListType = {
    PyVarObject_HEAD_INIT(NULL, 0) "simple_graphs.AdjacencyList", /* tp_name */
    sizeof(AdjacencyList),                                        /* tp_basicsize */
    0,                                                            /* tp_name */
    (destructor)AdjacencyList_dealloc,                            /* tp_dealloc */
    0,                                                            /* tp_vectorcall_offset */
    0,                                                            /* tp_getattr */
    0,                                                            /* tp_setattr */
    0,                                                            /* tp_as_async */
    0,                                                            /* tp_repr */
    0,                                                            /* tp_as_number */
    0,                                                            /* tp_as_sequence */
    0,                                                            /* tp_as_mapping */
    0,                                                            /* tp_hash */
    0,                                                            /* tp_call */
    0,                                                            /* tp_str */
    0,                                                            /* tp_getattro */
    0,                                                            /* tp_setattro */
    0,                                                            /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                           /* tp_flags */
    0,                                                            /* tp_doc */
    0,                                                            /* tp_traverse */
    0,                                                            /* tp_clear */
    0,                                                            /* tp_richcompare */
    0,                                                            /* tp_weaklistoffset */
    0,                                                            /* tp_iter */
    0,                                                            /* tp_iternext */
    AdjacencyList_methods,                                        /* tp_methods */
    0,                                                            /* tp_members */
    0,                                                            /* tp_getset */
    0,                                                            /* tp_base */
    0,                                                            /* tp_dict */
    0,                                                            /* tp_descr_get */
    0,                                                            /* tp_descr_set */
    0,                                                            /* tp_dictoffset */
    (initproc)AdjacencyList_init,                                 /* tp_init */
    0,                                                            /* tp_alloc */
    AdjacencyList_new,                                            /* tp_new */
};

static struct PyModuleDef graphmodule = {
    PyModuleDef_HEAD_INIT,
    "simple_graphs",
    NULL,
    -1};

PyMODINIT_FUNC PyInit_simple_graphs(void)
{
    PyObject *m;
    if (PyType_Ready(&AdjacencyListType) < 0)
        return NULL;

    m = PyModule_Create(&graphmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&AdjacencyListType);
    if (PyModule_AddObject(m, "AdjacencyList", (PyObject *)&AdjacencyListType) < 0)
    {
        Py_DECREF(&AdjacencyListType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}