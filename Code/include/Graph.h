/**
 * @file Graph.h
 * @brief Generic graph data structures: Vertex, Edge, and Graph templates.
 */
#ifndef GRAPH_H_
#define GRAPH_H_

#include <cstddef>
#include <vector>
#include <queue>
#include <fstream>
#include <iostream>
#include <limits>
#include <algorithm>


using namespace std;

template <class T> class Edge;
template <class T> class Graph;
template <class T> class Vertex;


/****************** Provided structures  ********************/

/**
 * @brief Represents a vertex in a generic graph.
 * @tparam T Type of the information stored in the vertex.
 */
template <class T>
class Vertex {
    T info;                   /**< @brief Contents stored in the vertex. */
    vector<Edge<T> > adj;     /**< @brief List of outgoing edges. */
    bool visited;             /**< @brief Auxiliary field for graph traversal. */
    bool processing;          /**< @brief Auxiliary field for cycle detection. */
    int indegree;             /**< @brief Auxiliary field for topological sorting. */
    int num;                  /**< @brief Auxiliary field for articulation points/SCC. */
    int low;                  /**< @brief Auxiliary field for articulation points/SCC. */

    /**
     * @brief Adds an outgoing edge to this vertex.
     * @param dest Destination vertex.
     * @param w Edge weight.
     */
    void addEdge(Vertex<T> *dest, double w);

    /**
     * @brief Removes an outgoing edge to a given destination.
     * @param d Destination vertex.
     * @return true if the edge was found and removed; false otherwise.
     */
    bool removeEdgeTo(Vertex<T> *d);

public:
    /**
     * @brief Constructs a vertex with the given info.
     * @param in The info to store.
     */
    Vertex(T in);

    /** @return The stored info. */
    T getInfo() const;

    /** @brief Sets the stored info.
     *  @param in The new info value. */
    void setInfo(T in);

    /** @return true if the vertex has been visited. */
    bool isVisited() const;

    /** @brief Marks the vertex as visited or not.
     *  @param v The visited state. */
    void setVisited(bool v);

    /** @return true if the vertex is being processed. */
    bool isProcessing() const;

    /** @brief Marks the vertex as processing or not.
     *  @param p The processing state. */
    void setProcessing(bool p);

    /** @return The indegree of the vertex. */
    int getIndegree() const;

    /** @brief Sets the indegree of the vertex.
     *  @param indegree The new indegree value. */
    void setIndegree(int indegree);

    /** @brief Sets the low value (used in SCC/articulation algorithms).
     *  @param low The new low value. */
    void setLow(int low);

    /** @brief Sets the num value (used in SCC/articulation algorithms).
     *  @param num The new num value. */
    void setNum(int num);

    /** @return The num value. */
    int getNum() const;

    /** @return The low value. */
    int getLow() const;

    /** @return Const reference to the adjacency list. */
    const vector<Edge<T>> &getAdj() const;

    /** @brief Sets the adjacency list.
     *  @param adj The new adjacency list. */
    void setAdj(const vector<Edge<T>> &adj);

    friend class Graph<T>;
};

/**
 * @brief Represents a directed edge in a generic graph.
 * @tparam T Type of the information stored in the vertices.
 */
template <class T>
class Edge {
    Vertex<T> * dest;   /**< @brief Destination vertex. */
    double weight;       /**< @brief Edge weight. */
public:
    /**
     * @brief Constructs an edge to a destination with a given weight.
     * @param d Destination vertex.
     * @param w Edge weight.
     */
    Edge(Vertex<T> *d, double w);

    /** @return Pointer to the destination vertex. */
    Vertex<T> *getDest() const;

    /** @brief Sets the destination vertex.
     *  @param dest The new destination vertex. */
    void setDest(Vertex<T> *dest);

    /** @return The edge weight. */
    double getWeight() const;

    /** @brief Sets the edge weight.
     *  @param weight The new weight value. */
    void setWeight(double weight);

    friend class Graph<T>;
    friend class Vertex<T>;
};

/**
 * @brief Represents a generic directed or undirected graph.
 * @tparam T Type of the information stored in vertices.
 */
template <class T>
class Graph {
    vector<Vertex<T> *> vertexSet; /**< @brief Set of vertices in the graph. */

    /**
     * @brief DFS visit helper used by the public dfs() methods.
     * @param v Current vertex being visited.
     * @param res Vector to accumulate the traversal order.
     */
    void dfsVisit(Vertex<T> *v, vector<T> & res) const;

public:
    /**
     * @brief Finds a vertex by its stored info.
     * @param in The info to search for.
     * @return Pointer to the vertex, or nullptr if not found.
     */
    Vertex<T> *findVertex(const T &in) const;

    /** @return The number of vertices in the graph. */
    int getNumVertex() const;

    /**
     * @brief Adds a vertex with the given info.
     * @param in The info to store in the new vertex.
     * @return true if the vertex was added; false if a vertex with that info already exists.
     */
    bool addVertex(const T &in);

    /**
     * @brief Removes a vertex and all its incident edges.
     * @param in The info of the vertex to remove.
     * @return true if the vertex was found and removed; false otherwise.
     */
    bool removeVertex(const T &in);

    /**
     * @brief Adds a directed edge between two vertices.
     * @param sourc Info of the source vertex.
     * @param dest Info of the destination vertex.
     * @param w Edge weight.
     * @return true if the edge was added; false if either vertex does not exist.
     */
    bool addEdge(const T &sourc, const T &dest, double w);

    /**
     * @brief Adds an undirected edge between two vertices.
     * @param sourc Info of the first vertex.
     * @param dest Info of the second vertex.
     * @param w Edge weight.
     * @return true if the edge was added; false if either vertex does not exist.
     */
    bool addUndirectedEdge(const T &sourc, const T &dest, double w);

    /**
     * @brief Removes a directed edge between two vertices.
     * @param sourc Info of the source vertex.
     * @param dest Info of the destination vertex.
     * @return true if the edge was removed; false if either vertex or the edge does not exist.
     */
    bool removeEdge(const T &sourc, const T &dest);

    /** @return The vector of all vertices in the graph. */
    vector<Vertex<T> * > getVertexSet() const;

    /** @return DFS traversal from an arbitrary start vertex. */
    vector<T> dfs() const;

    /**
     * @brief DFS traversal from a given source vertex.
     * @param source The starting vertex info.
     * @return Vector of visited vertex infos in DFS order.
     */
    vector<T> dfs(const T & source) const;

    /**
     * @brief BFS traversal from a given source vertex.
     * @param source The starting vertex info.
     * @return Vector of visited vertex infos in BFS order.
     */
    vector<T> bfs(const T & source) const;

    /**
     * @brief Emits a DOT file representation of the graph.
     * @param gname Base filename (without extension); ".gv" is appended automatically.
     */
    void emitDOTFile(string gname);

};

/****************** Provided constructors and functions ********************/

template <class T>
Vertex<T>::Vertex(T in): info(in) {}

template <class T>
Edge<T>::Edge(Vertex<T> *d, double w): dest(d), weight(w) {}


template <class T>
int Graph<T>::getNumVertex() const {
	return vertexSet.size();
}

template <class T>
vector<Vertex<T> * > Graph<T>::getVertexSet() const {
    return vertexSet;
}

template<class T>
T Vertex<T>::getInfo() const {
    return info;
}

template<class T>
void Vertex<T>::setInfo(T in) {
    Vertex::info = in;
}

template<class T>
bool Vertex<T>::isProcessing() const {
    return processing;
}
template<class T>
void Vertex<T>::setNum(int num) {
    Vertex::num = num;
}

template<class T>
void Vertex<T>::setLow(int low) {
    Vertex::low = low;
}

template<class T>
int Vertex<T>::getNum() const {
    return num;
}

template<class T>
int Vertex<T>::getLow() const {
    return low;
}

template<class T>
void Vertex<T>::setProcessing(bool p) {
    Vertex::processing = p;
}

template<class T>
Vertex<T> *Edge<T>::getDest() const {
    return dest;
}

template<class T>
void Edge<T>::setDest(Vertex<T> *d) {
    Edge::dest = d;
}

template<class T>
double Edge<T>::getWeight() const {
    return weight;
}

template<class T>
void Edge<T>::setWeight(double weight) {
    Edge::weight = weight;
}

/*
 * Auxiliary function to find a vertex with a given content.
 */
template <class T>
Vertex<T> * Graph<T>::findVertex(const T &in) const {
	for (auto v : vertexSet)
		if (v->info == in)
			return v;
	return NULL;
}

template <class T>
bool Vertex<T>::isVisited() const {
    return visited;
}

template <class T>
void Vertex<T>::setVisited(bool v) {
    Vertex::visited = v;
}
template<class T>
int Vertex<T>::getIndegree() const {
    return indegree;
}

template<class T>
void Vertex<T>::setIndegree(int indegree) {
    Vertex::indegree = indegree;
}

/*
template<class T>
vector<Edge<T> > Vertex<T>::getIncoming() const {
    return this->incoming;
}
 */

template<class T>
const vector<Edge<T>> &Vertex<T>::getAdj() const {
    return adj;
}

template <class T>
void Vertex<T>::setAdj(const vector<Edge<T>> &adj) {
    Vertex::adj = adj;
}


/*
 * Adds a vertex with a given content or info (in) to a graph (this).
 * Returns true if successful, and false if a vertex with that content already exists.
 */
template <class T>
bool Graph<T>::addVertex(const T &in) {
	if ( findVertex(in) != NULL)
		return false;
	vertexSet.push_back(new Vertex<T>(in));
	return true;
}


/*
 * Adds an edge to a graph (this), given the contents of the source and
 * destination vertices and the edge weight (w).
 * Returns true if successful, and false if the source or destination vertex does not exist.
 */
template <class T>
bool Graph<T>::addEdge(const T &sourc, const T &dest, double w) {
	auto v1 = findVertex(sourc);
	auto v2 = findVertex(dest);
	if (v1 == NULL || v2 == NULL)
		return false;
	v1->addEdge(v2,w);
    //v2->incoming.push_back(Edge<T>(v1,w)); // incoming edges
	return true;
}

/*
 * Adds an undirected edge to a graph (this).
 * Returns true if successful, and false if the source or destination vertex does not exist.
 */
template <class T>
bool Graph<T>::addUndirectedEdge(const T& sourc, const T& dest, double w) {
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (v1 == nullptr || v2 == nullptr) return false;
    v1->addEdge(v2, w);
    v2->addEdge(v1, w);
    return true;
}

/*
 * Auxiliary function to add an outgoing edge to a vertex (this),
 * with a given destination vertex (d) and edge weight (w).
 */
template <class T>
void Vertex<T>::addEdge(Vertex<T> *d, double w) {
	adj.push_back(Edge<T>(d, w));
}


/*
 * Removes an edge from a graph (this).
 * Returns true if successful, and false if such edge does not exist.
 */
template <class T>
bool Graph<T>::removeEdge(const T &sourc, const T &dest) {
	auto v1 = findVertex(sourc);
	auto v2 = findVertex(dest);
	if (v1 == NULL || v2 == NULL)
		return false;

	return v1->removeEdgeTo(v2); //&& v2->removeEdgeFrom(v1);
}

/*
 * Auxiliary function to remove an incoming edge (with a given source (s))
 * from a vertex (this).
 * Returns true if successful, and false if such edge does not exist.
 */
/*
template <class T>
bool Vertex<T>::removeEdgeFrom(Vertex<T> *d) {
    auto inc = getIncoming();
    for (auto it = inc.begin(); it != inc.end(); it++)
        if (it->dest  == d) {
            inc.erase(it);
            return true;
        }
    return false;
}
*/


/*
 * Auxiliary function to remove an outgoing edge from a vertex.
 * Returns true if successful, and false if such edge does not exist.
 */
template <class T>
bool Vertex<T>::removeEdgeTo(Vertex<T> *d) {
	for (auto it = adj.begin(); it != adj.end(); it++)
		if (it->dest  == d) {
			adj.erase(it);
			return true;
		}
	return false;
}

/*
 * Removes a vertex with a given content (in) from a graph, and all incident edges.
 * Returns true if successful, and false if such vertex does not exist.
 */
template <class T>
bool Graph<T>::removeVertex(const T &in) {
	for (auto it = vertexSet.begin(); it != vertexSet.end(); it++)
		if ((*it)->info  == in) {
			auto v = *it;
			vertexSet.erase(it);
			for (auto u : vertexSet)
				u->removeEdgeTo(v);
			delete v;
			return true;
		}
	return false;
}

template <class T>
inline void  Graph<T>::emitDOTFile(string gname) {
    ofstream g_dot_file;

    g_dot_file.open (gname+".gv");
    g_dot_file << "graph { \n";

    g_dot_file << "labelloc  = top;\n";
    g_dot_file << "labeljust = left;\n";
    g_dot_file << "fontname  = calibri;\n";
    g_dot_file << "fontsize  = 16;\n";
    g_dot_file << "rankdir = LR;\n";
    g_dot_file << "  node [shape=circle, style=filled];\n";
    g_dot_file << "\n";

    for (auto v : vertexSet) {
        g_dot_file << "  " << v->getInfo() << ";\n";  // Print isolated node
        for (auto & e : v->getAdj()) {
            auto w = e.getDest();
            // Emit undirected edge only once by ordering pointers
            if (v < w) {
                g_dot_file << "  " << v->getInfo() << " -- " << w->getInfo() << ";\n";
            }
        }
    }

    g_dot_file << "}\n";
    g_dot_file.close();
}

#endif /* GRAPH_H_ */
