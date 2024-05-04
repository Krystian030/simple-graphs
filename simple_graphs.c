#include <Python.h>
#include "structmember.h"

typedef struct Node {
    int vertex;
    struct Node* next;
} Node;

typedef struct
{
    PyObject_HEAD
    Node* adj_list[16];
} AdjacencyList;


Node* createNode(int v) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    newNode->vertex = v;
    newNode->next = NULL;
    return newNode;
}

void addEdge(AdjacencyList* self, int src, int dest) {
    Node* newNode = createNode(dest);
    newNode->next = self->adj_list[src];
    self->adj_list[src] = newNode;

    newNode = createNode(src);
    newNode->next = self->adj_list[dest];
    self->adj_list[dest] = newNode;
}

static void AdjacencyList_dealloc(AdjacencyList* self) {
    for (int i = 0; i < 16; i++) {
        Node* current = self->adj_list[i];
        while (current != NULL) {
            Node* next = current->next;
            free(current);
            current = next;
        }
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* AdjacencyList_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    AdjacencyList* self;
    self = (AdjacencyList*)type->tp_alloc(type, 0);
    if (self != NULL) {
        for (int i = 0; i < 16; i++) {
            self->adj_list[i] = NULL;
        }
    }
    return (PyObject*)self;
}


static int AdjacencyList_init(AdjacencyList* self, PyObject* args, PyObject* kwds) {
    static char* kwlist[] = { "text", NULL };
    char* text = "?";

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &text)) {
        return -1;
    }

    if (text[0] != '?') {
        int num_vertices = text[0] - 63; 

        if (num_vertices <= 0) {
            PyErr_SetString(PyExc_ValueError, "Niepoprawny format g6. Liczba wierzcho³ków powinna byæ > 0");
            return -1;
        }

        for (int i = 0; i < num_vertices; i++) {
            Node* newNode = createNode(i);
            if (newNode == NULL) {
                PyErr_SetString(PyExc_RuntimeError, "Wyst¹pi³ nieoczekiwany problem podczas tworzenia wierzcho³ka");
                return -1;
            }
            self->adj_list[i] = newNode;
        }

        int c = 0;
        int i = 1;
        int k = 0;

        for (int v = 1; v < num_vertices; v++) {
            for (int u = 0; u < v; u++) {
                if (k == 0) {
                    c = text[i++] - 63;
                    k = 6;
                }
                k--;

                if ((c & (1 << k)) != 0) {
                    addEdge(self, u, v);
                }
            }
        }
    }

    return 0;
}


static PyObject *number_of_vertices(AdjacencyList *self) {
    int counter = 0;

    for (int i = 0; i < 16; i++) {
        if (self->adj_list[i] != NULL) {
            counter++;
        }
    }

    return PyLong_FromLong(counter);
}

static PyObject* vertices(AdjacencyList* self) {
    PyObject* vertex_set = PySet_New(NULL);

    if (vertex_set == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "Wyst¹pi³ nieoczekiwany problem podczas tworzenia zbioru dla wierzcho³ków");
        return NULL;
    }

    for (int i = 0; i < 16; i++)
    {
        if (self->adj_list[i] != NULL) {
            PyObject* vertex = PyLong_FromLong(i);

            if (vertex == NULL)
            {
                Py_DECREF(vertex_set);
                PyErr_SetString(PyExc_RuntimeError, "Wyst¹pi³ nieoczekiwany problem podczas tworzenia wierzcho³ka");
                return NULL;
            }
            PySet_Add(vertex_set, vertex);
            Py_DECREF(vertex);
        }
    }

    return vertex_set;
}

static PyObject* vertex_degree(AdjacencyList* self, PyObject* args) {
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex)) {
        return NULL;
    }

    if (vertex < 0 || vertex >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }


    Node* current = self->adj_list[vertex];
    int degree = 0;

    while (current != NULL && current->next != NULL) {
        degree++;
        current = current->next;
    }

    return PyLong_FromLong(degree);
}



static PyObject* vertex_neighbors(AdjacencyList* self, PyObject* args) {
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex)) {
        return NULL;
    }

    if (vertex < 0 || vertex >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    PyObject* neighbors_set = PySet_New(NULL);

    if (neighbors_set == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Nie uda³o siê utworzyæ zbioru s¹siadów");
        return NULL;
    }

    Node* current = self->adj_list[vertex];

    while (current != NULL) {
        int neighbor = current->vertex;
        if (neighbor == vertex) {
            current = current->next;
            continue;
        }
        
        PyObject* neighbor_py = PyLong_FromLong(neighbor);

        if (neighbor_py == NULL) {
            Py_DECREF(neighbors_set);
            PyErr_SetString(PyExc_RuntimeError, "Nie uda³o siê utworzyæ obiektu s¹siada");
            return NULL;
        }

        int result = PySet_Add(neighbors_set, neighbor_py);
        Py_DECREF(neighbor_py);

        if (result < 0) {
            Py_DECREF(neighbors_set);
            PyErr_SetString(PyExc_RuntimeError, "Nie uda³o siê dodaæ s¹siada do zbioru s¹siadów");
            return NULL;
        }

        current = current->next;
    }

    return neighbors_set;
}

static PyObject* add_vertex(AdjacencyList* self, PyObject* args) {
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex)) {
        return NULL;
    }

    if (vertex < 0 || vertex >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    if (self->adj_list[vertex] != NULL) {
        PyErr_SetString(PyExc_ValueError, "Wierzcho³ek ju¿ istnieje w grafie");
        return NULL;
    }

    Node* newNode = createNode(vertex);
    if (newNode == NULL) {
        return NULL;  
    }
    self->adj_list[vertex] = newNode;

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject* delete_vertex(AdjacencyList* self, PyObject* args) {
    int vertex;

    if (!PyArg_ParseTuple(args, "i", &vertex)) {
        return NULL;
    }

    if (vertex < 0 || vertex >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    if (self->adj_list[vertex] == NULL) {
        PyErr_SetString(PyExc_ValueError, "Wierzcho³ek nie istnieje");
        return NULL;
    }

    Node* current = self->adj_list[vertex];
    while (current != NULL) {
        int neighbor = current->vertex;
        Node* next = current->next;
        free(current);
        self->adj_list[vertex] = NULL;
        current = next;

        Node* neighborNode = self->adj_list[neighbor];
        Node* prev = NULL;
        while (neighborNode != NULL) {
            if (neighborNode->vertex == vertex) {
                if (prev == NULL) {
                    self->adj_list[neighbor] = neighborNode->next;
                }
                else {
                    prev->next = neighborNode->next;
                }
                free(neighborNode);
                break;
            }
            prev = neighborNode;
            neighborNode = neighborNode->next;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}



static PyObject* number_of_edges(AdjacencyList* self) {
    int edgeCount = 0;

    for (int i = 0; i < 16; i++) {
        Node* current = self->adj_list[i];

        while (current != NULL) {
            if (current->vertex > i) {
                edgeCount++;
            }
            current = current->next;
        }
    }

    return PyLong_FromLong(edgeCount);
}

static PyObject* edges(AdjacencyList* self) {
    PyObject* edges_set = PySet_New(NULL);

    if (edges_set == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Nie mo¿na zaalokowaæ pamiêci dla zbioru (set)");
        return NULL;
    }

    for (int u = 0; u < 16; u++) {
        Node* current = self->adj_list[u];

        while (current != NULL) {
            int v = current->vertex;
            if (u < v) {
                PyObject* edge_tuple = Py_BuildValue("(ii)", u, v);
                if (edge_tuple == NULL) {
                    Py_DECREF(edges_set);
                    PyErr_SetString(PyExc_RuntimeError, "Wyst¹pi³ nieoczekiwany problem podczas tworzenia krotki (tuple)");
                    return NULL;
                }

                int result = PySet_Add(edges_set, edge_tuple);
                Py_DECREF(edge_tuple);

                if (result < 0) {
                    Py_DECREF(edges_set);
                    PyErr_SetString(PyExc_RuntimeError, "Wyst¹pi³ nieoczekiwany problem podczas dodawania krotki do zbioru");
                    return NULL;
                }
            }

            current = current->next;
        }
    }

    return edges_set;
}


static PyObject* is_edge(AdjacencyList* self, PyObject* args) {
    int u, v;

    if (!PyArg_ParseTuple(args, "ii", &u, &v)) {
        return NULL;
    }

    if (u < 0 || u >= 16 || v < 0 || v >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    Node* current = self->adj_list[u];
    while (current != NULL) {
        if (current->vertex == v) {
            return PyBool_FromLong(1);
        }
        current = current->next;
    }

    PyBool_FromLong(0);
}


static PyObject* add_edge(AdjacencyList* self, PyObject* args) {
    int src, dest;

    if (!PyArg_ParseTuple(args, "ii", &src, &dest)) {
        return NULL;
    }

    if (src < 0 || src >= 16 || dest < 0 || dest >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    addEdge(self, src, dest);

    return PyBool_FromLong(1);
}

int deleteNode(Node** head, int key) {
    Node* temp = *head;
    Node* prev = NULL;

    while (temp != NULL && temp->vertex != key) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        return 0;
    }

    if (prev == NULL) {
        *head = temp->next;
    }
    else {
        prev->next = temp->next;
    }

    free(temp);
    return 1;
}

static PyObject* delete_edge(AdjacencyList* self, PyObject* args) {
    int u, v;

    if (!PyArg_ParseTuple(args, "ii", &u, &v)) {
        return NULL;
    }

    if (u < 0 || u >= 16 || v < 0 || v >= 16) {
        PyErr_SetString(PyExc_ValueError, "Indeks wierzcho³ka poza zakresem");
        return NULL;
    }

    if (self->adj_list[u] == NULL || self->adj_list[v] == NULL) {
        PyErr_SetString(PyExc_ValueError, "Wierzcho³ek nie istnieje w grafie");
        return NULL;
    }

    int deleted = deleteNode(&(self->adj_list[u]), v);
    if (!deleted) {
        return NULL;
    }

    deleted = deleteNode(&(self->adj_list[v]), u);
    if (!deleted) {
        return NULL; 
    }

    return PyBool_FromLong(1);
}

int isBipartiteUtil(AdjacencyList* self, int v, int col[]) {
    col[v] = 0; // Kolorujemy wierzcho³ek v na kolor 0

    // kolejka dla BFS
    int queue[16];
    int front = 0, rear = 0;
    queue[rear++] = v;

    while (front < rear) {
        int u = queue[front++];

        // Przechodzimy przez wszystkich s¹siadów wierzcho³ka u
        Node* current = self->adj_list[u];
        while (current != NULL) {
            int neighbor = current->vertex;

            if (neighbor != u) {

                // Jeœli s¹siad nie zosta³ jeszcze pokolorowany
                if (col[neighbor] == -1) {
                    // Kolorujemy s¹siada na przeciwny kolor
                    col[neighbor] = 1 - col[u];
                    queue[rear++] = neighbor;
                }
                // Jeœli s¹siad ma ten sam kolor co u, to graf nie jest dwudzielny
                else if (col[neighbor] == col[u]) {
                    return 0;
                }
            }
            current = current->next;
        }
    }

    return 1; 
}

static PyObject* is_bipartite(AdjacencyList* self) {
    int col[16];

    // Inicjalizujemy tablicê kolorów na -1
    for (int i = 0; i < 16; i++) {
        col[i] = -1;
    }

    // Sprawdzamy wszystkie sk³adowe grafu
    for (int i = 0; i < 16; i++) {
        if (col[i] == -1) {
            if (!isBipartiteUtil(self, i, col)) {
                Py_RETURN_FALSE;
                return PyBool_FromLong(0);
            }
        }
    }

    return PyBool_FromLong(1);
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
    {NULL, NULL}
};

static PyTypeObject AdjacencyListType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "simple_graphs.AdjacencyList",    /* tp_name */
    sizeof(AdjacencyList),             /* tp_basicsize */
    0,                                 /* tp_name */
    (destructor)AdjacencyList_dealloc, /* tp_dealloc */
    0,                                 /* tp_vectorcall_offset */
    0,                                 /* tp_getattr */
    0,                                 /* tp_setattr */
    0,                                 /* tp_as_async */
    0,                                 /* tp_repr */
    0,                                 /* tp_as_number */
    0,                                 /* tp_as_sequence */
    0,                                 /* tp_as_mapping */
    0,                                 /* tp_hash */
    0,                                 /* tp_call */
    0,                                 /* tp_str */
    0,                                 /* tp_getattro */
    0,                                 /* tp_setattro */
    0,                                 /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                /* tp_flags */
    0,                                 /* tp_doc */
    0,                                 /* tp_traverse */
    0,                                 /* tp_clear */
    0,                                 /* tp_richcompare */
    0,                                 /* tp_weaklistoffset */
    0,                                 /* tp_iter */
    0,                                 /* tp_iternext */
    AdjacencyList_methods,             /* tp_methods */
    0,                                 /* tp_members */
    0,                                 /* tp_getset */
    0,                                 /* tp_base */
    0,                                 /* tp_dict */
    0,                                 /* tp_descr_get */
    0,                                 /* tp_descr_set */
    0,                                 /* tp_dictoffset */
    (initproc)AdjacencyList_init,      /* tp_init */
    0,                                 /* tp_alloc */
    AdjacencyList_new,                 /* tp_new */
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