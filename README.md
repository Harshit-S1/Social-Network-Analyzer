# High-Performance Social Network Analytics Engine

![C++](https://img.shields.io/badge/C++-17-blue)
![Python](https://img.shields.io/badge/Python-3.8+-yellow)
![PyBind11](https://img.shields.io/badge/PyBind11-Native%20Bindings-orange)
![Streamlit](https://img.shields.io/badge/Streamlit-Dashboard-red)

A production-grade, memory-optimized social network analytics engine built in **C++17** and seamlessly integrated with a **Python/Streamlit** frontend using **PyBind11**.

The engine ingests directed graph data (Follower/Following relationships), computes network resilience and influence metrics, and serves explainable friend recommendations with minimal overhead through native C++ performance.

---

# Core Architecture & Systems Optimization

To maximize performance and scalability, the engine employs several low-level systems optimizations:

### Compressed Sparse Row (CSR) Memory Layout

* Flattened the adjacency list into contiguous 1D arrays.
* Maximizes cache locality and significantly reduces CPU cache misses during graph traversals such as PageRank and BFS.

### Zero-Copy Python Binding

* Integrated the C++ backend directly with Python using **PyBind11**.
* Eliminates disk I/O and JSON serialization by exposing native C++ objects directly to the Streamlit frontend.

### O(L) Radix Tree (Compressed Trie)

* Custom-built Radix Tree powering live username autocomplete.
* Reduces lookup complexity from **O(N)** to **O(L)**, where *L* is the prefix length.

### O(1) LRU Analytics Cache

* Implemented a custom Least Recently Used cache using a doubly-linked list and hash map.
* Caches expensive analytics (Shortest Path, Recommendations) and automatically invalidates after graph mutations.

### O(N log K) Min-Heap Extraction

* Uses bounded priority queues instead of global sorting for scalable top-influencer computation.

### O(1) Edge Deduplication

* Combines 32-bit user IDs into unique 64-bit signatures via bitwise operations.
* Enables constant-time duplicate edge detection during large dataset ingestion.

---

# Graph Algorithms

### Global & Personalized PageRank

* Implemented iterative power iteration for influence scoring.
* Supports Personalized PageRank by biasing the teleportation vector toward selected source nodes.

### Weakly Connected Components

* Uses Union-Find (Disjoint Set Union) with:

  * Path Compression
  * Union by Rank

Achieves near **O(α(N))** time complexity.

### Influence-Aware Friend Recommendations

* Explores second-degree connections using Breadth-First Search (BFS).
* Ranks candidates using PageRank scores of mutual bridging users.

### O(d) Linear Neighbor Intersection

* Leverages sorted CSR arrays.
* Computes shared neighbors using an efficient two-pointer intersection algorithm.

---

# Interactive Dashboard

The frontend is built with **Streamlit** and **PyVis**, providing an interactive interface for exploring and analyzing the social network.

### Features

* **Live Graph Editor**

  * Add/Delete users
  * Follow/Unfollow relationships
  * Automatically rebuilds CSR structures and invalidates cached analytics.

* **Interactive Network Visualization**

  * Barnes-Hut physics simulation using PyVis.
  * Node sizes dynamically scale according to PageRank scores.

* **Explainable Recommendations**

  * Displays not only recommended users but also the shared connections that justify each recommendation.

* **Shortest Path Finder**

  * Interactive BFS visualization showing the shortest follow chain between any two users.

---

# Build & Run

## Prerequisites

* CMake ≥ 3.15
* Python ≥ 3.8
* Modern C++17 compiler (GCC, Clang, or MSVC)

---

## 1. Install Python Dependencies

```bash
pip install streamlit pyvis streamlit-searchbox pybind11
```

---

## 2. Compile the C++ Engine

```bash
mkdir build
cd build

cmake ..
cmake --build . --config Release
```

Move the generated native module (`.so` on Linux/macOS or `.pyd` on Windows) from the `build/` directory into the project root.

---

## 3. Launch the Dashboard

```bash
streamlit run app.py
```

---

# Technologies Used

* C++17
* Python
* Streamlit
* PyBind11
* PyVis
* CMake

