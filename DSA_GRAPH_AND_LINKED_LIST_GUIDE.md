# DeptFlow V2: Student DSA Viva & Implementation Guide
## Assigned Topics:
1. **Adjacency Matrix Graph + BFS (Breadth-First Search)**
2. **Singly Linked List (`CommitNode*`) for Outside Faculty Commitments**

---

# PART 1: Adjacency Matrix Graph + BFS (Breadth-First Search)

---

### 1. Where It Is Located in the Codebase

| Component | File & Exact Lines | What It Does |
| :--- | :--- | :--- |
| **Class Definition & BFS Algorithm** | `backend/dsa.h` (Lines 77–102) | Defines `class Graph`, the 2D `adj[MAX_DIVS][MAX_DIVS]` matrix, `addEdge()`, `isDirect()`, and the queue-based `bfs()` traversal. |
| **Global Graph Instance** | `backend/main.cpp` (Line 24) & `backend/scheduler.h` (Line 17) | Instantiates `Graph divGraph;` in `main.cpp` and declares `extern Graph divGraph;` in `scheduler.h`. |
| **Graph Construction (`buildFacultyGraph`)** | `backend/scheduler.h` (Lines 28–58) | Scans all subjects across all divisions to find shared professors and connects divisions with `divGraph.addEdge(i, j)`. |
| **API Execution & Dependency Analysis** | `backend/api.h` (Lines 700–725) | Runs `divGraph.bfs(...)` on each division to separate **direct** vs **indirect** ripple dependencies for the reports API. |

---

### 2. Simple Explanation: Why We Use It & What It Does

#### The Problem:
In a college department, academic divisions are not independent. For example:
- Division `FE-A` (First Year Division A) and `FE-B` share **Prof. Mandar** for Mathematics.
- Division `FE-B` and `SE-A` (Second Year Division A) share **Prof. Sayali** for Programming.

If an administrator reschedules or modifies a lecture in `FE-A`, which other divisions will feel the ripple effect?
- `FE-B` is directly affected because they share Prof. Mandar (**Direct Dependency**).
- `SE-A` is indirectly affected because modifying `FE-B` might force Prof. Sayali's time in `FE-B` to move, which in turn collides with `SE-A` (**Indirect Transitive Dependency**).

#### How the Graph Models This:
1. **Vertices (Nodes)**: Each academic division (e.g. `FE-A`, `FE-B`, `SE-A`, `TE-A`, `BE-A`) is a node in the graph.
2. **Edges**: If two divisions share at least one faculty member (either for theory or for lab practicals), an undirected edge is created between them: `adj[u][v] = 1; adj[v][u] = 1;`.
3. **BFS (Breadth-First Search)**:
   - Starts at a chosen division `start`.
   - Uses a **FIFO Queue** to visit divisions level by level.
   - **Level 1 (Direct)**: Divisions connected directly by an edge ($adj[\text{start}][u] == 1$).
   - **Level 2+ (Indirect)**: Divisions reachable through a chain of shared professors.

---

### 3. Step-by-Step Code Walkthrough

#### A. The Graph Class (`backend/dsa.h`, Lines 77–102)
```cpp
class Graph {
    int adj[MAX_DIVS][MAX_DIVS];  // 2D Adjacency Matrix (0 = no edge, 1 = shared faculty)
public:
    Graph() { 
        for (int i = 0; i < MAX_DIVS; i++) 
            for (int j = 0; j < MAX_DIVS; j++) adj[i][j] = 0; 
    }
    void addEdge(int u, int v) { 
        if (u < MAX_DIVS && v < MAX_DIVS) { 
            adj[u][v] = 1; 
            adj[v][u] = 1; // Undirected graph
        } 
    }
    bool isDirect(int u, int v) const { 
        return (u < MAX_DIVS && v < MAX_DIVS) ? (adj[u][v] == 1) : false; 
    }
```
- We use an **Adjacency Matrix** of size `MAX_DIVS × MAX_DIVS` because the number of divisions in a college department is small (typically 4 to 10), giving $\mathcal{O}(1)$ edge lookups with minimal memory footprint.

#### B. The BFS Function (`backend/dsa.h`, Lines 83–101)
```cpp
    void bfs(int start, int n, int* visitedOrder, int& orderCount, bool* directFlags) {
        bool visited[MAX_DIVS] = {false};
        int queue[MAX_DIVS], front = 0, rear = 0;
        visited[start] = true; 
        queue[rear++] = start;  // Enqueue start division
        orderCount = 0;

        while (front < rear) {
            int u = queue[front++]; // Dequeue
            if (u != start) {
                visitedOrder[orderCount] = u;
                // If adjacent to start, it's a DIRECT dependency; otherwise INDIRECT
                directFlags[orderCount] = (adj[start][u] == 1);
                orderCount++;
            }
            for (int v = 0; v < n; v++) {
                if (adj[u][v] && !visited[v]) {
                    visited[v] = true; 
                    queue[rear++] = v; // Enqueue neighbor
                }
            }
        }
    }
```
1. It maintains a boolean `visited[]` array to prevent revisiting the same division (avoiding infinite loops).
2. Uses a manual array-based FIFO Queue (`queue[]`, `front`, `rear`).
3. For every visited division, it checks `adj[start][u] == 1`:
   - If `true`: Marked as a **Direct Dependency** (immediate neighbor).
   - If `false`: Marked as an **Indirect Dependency** (multi-hop ripple).

#### C. Building the Graph (`backend/scheduler.h`, Lines 28–58)
- Loops through all pairs of divisions $(i, j)$.
- Compares their assigned subjects. If `subjects[s1].facIdx == subjects[s2].facIdx` (or practical faculty matches), calls `divGraph.addEdge(i, j)`.

---

### 4. Time & Space Complexity
- **Time Complexity of BFS**: $\mathcal{O}(V^2)$ where $V = \text{number of divisions}$. Since we check all entries in row $u$ of the adjacency matrix for each dequeued node.
- **Space Complexity**: $\mathcal{O}(V^2)$ to store the 2D adjacency matrix `adj[MAX_DIVS][MAX_DIVS]`, plus $\mathcal{O}(V)$ for the queue and visited array.

---

### 5. Viva Questions & Ready Answers (Adjacency Matrix + BFS)

> **Q1: Why did you choose an Adjacency Matrix instead of an Adjacency List?**  
> **Answer**: *"In our department system, the number of divisions is small and bounded (usually 4 to 10 divisions). An Adjacency Matrix allows instant $\mathcal{O}(1)$ checking of whether two divisions share faculty (`adj[u][v] == 1`), without needing dynamic pointer allocations."*

> **Q2: What is the practical purpose of running BFS here?**  
> **Answer**: *"BFS performs a level-order traversal starting from any division. It helps us classify cascade impact into two categories: **Direct Dependencies** (divisions at distance 1 that share the exact same teacher) and **Indirect Dependencies** (divisions at distance $\ge 2$ that will be impacted by secondary schedule shifts)."*

> **Q3: How do you prevent infinite loops during BFS traversal?**  
> **Answer**: *"We use a boolean array `visited[MAX_DIVS] = {false}`. When a division is added to the queue, `visited[v]` is set to `true`. This ensures no division is processed more than once even if the graph has cycles."*

---
---

# PART 2: Singly Linked List (`CommitNode*`)

---

### 1. Where It Is Located in the Codebase

| Component | File & Exact Lines | What It Does |
| :--- | :--- | :--- |
| **Struct Definition** | `backend/models.h` (Lines 14–18) | Defines `struct CommitNode { int day, slot; CommitNode* next; };` with a constructor. |
| **Faculty Struct Integration** | `backend/models.h` (Lines 75–78) | Adds `CommitNode* commitments;` as the head pointer inside `struct Faculty` (initialized to `nullptr`). |
| **Constraint Checking in Scheduler** | `backend/scheduler.h` (Lines 82–86) | Traverses the linked list in the 7-constraint validator to prevent scheduling lectures during external commitments. |
| **File Persistence (Save to Disk)** | `backend/storage.h` (Lines 48–50) | Traverses `commitments` and serializes day/slot pairs formatted as `day:slot;` into `data/faculty.txt`. |
| **File Persistence (Load from Disk)** | `backend/storage.h` (Lines 162–170) | Reads the saved pairs and reconstructs the linked list using dynamic $\mathcal{O}(1)$ head insertion (`new CommitNode(...)`). |
| **JSON Serialization (API)** | `backend/api.h` (Lines 214–221) | Traverses the list to serialize commitments into a JSON array `[{"day":0,"slot":1}, ...]` for the web frontend. |

---

### 2. Simple Explanation: Why We Use It & What It Does

#### The Problem:
In colleges, departments have two kinds of teachers:
1. **Internal Department Faculty**: Full-time teachers available all week with **zero outside commitments**.
2. **Visiting / Outside Faculty**: Teachers borrowed from other departments (like Mathematics or Electronics) who have strict prior commitments (e.g. they teach in IT on Monday slot 0 and Wednesday slot 2).

If we used a fixed 2D array for every faculty member to record external commitments:
- 90% of internal faculty would have completely empty arrays, wasting memory.
- A fixed array would also impose an arbitrary maximum limit on how many outside commitments a guest lecturer could have.

#### The Solution:
We use a **Singly Linked List** anchored at `faculties[i].commitments`:
- For an internal professor: `commitments = nullptr`. It consumes **0 bytes** of extra memory beyond the 8-byte pointer!
- For a visiting professor: A new node is dynamically allocated only when a commitment exists. If they have 3 outside commitments, exactly 3 nodes are allocated in heap memory.

---

### 3. Step-by-Step Code Walkthrough

#### A. Node Structure (`backend/models.h`, Lines 14–18)
```cpp
struct CommitNode {
    int day;           // 0 = Monday, 1 = Tuesday, ...
    int slot;          // Slot index (0, 1, 2, ...)
    CommitNode* next;  // Pointer to the next commitment node
    
    CommitNode(int d = 0, int s = 0, CommitNode* n = nullptr) 
        : day(d), slot(s), next(n) {}
};
```
- Each node stores the blocked `(day, slot)` coordinate and a `next` pointer pointing to the next node (or `nullptr` if it's the last node).

#### B. Dynamic Head Insertion (`backend/storage.h`, Lines 166–169)
```cpp
faculties[i].commitments = new CommitNode(dayVal, slotVal, faculties[i].commitments);
```
- **How it works**: When loading data from disk, each new node points its `next` pointer to the current head (`faculties[i].commitments`), and then becomes the new head.
- This is **$\mathcal{O}(1)$ Head Insertion** (fastest possible insertion in a linked list).

#### C. Traversal for Conflict Detection (`backend/scheduler.h`, Lines 82–86)
```cpp
// Check Constraint 6: Outside faculty commitments
for (CommitNode* c = faculties[facIdx].commitments; c; c = c->next) {
    if (c->day == day && c->slot == s) {
        reason = "Outside faculty home department commitment clash";
        return false; // Rejected: Professor is busy in their own department!
    }
}
```
1. Sets a crawler pointer `c` to `faculties[facIdx].commitments`.
2. As long as `c != nullptr`, checks if the candidate timetable slot `(day, s)` matches `(c->day, c->slot)`.
3. If it matches, the scheduler immediately rejects the slot and tries another one.
4. If no clash, advances `c = c->next`.

---

### 4. Time & Space Complexity
- **Insertion**: $\mathcal{O}(1)$ time by prepending new nodes at the head.
- **Search / Traversal**: $\mathcal{O}(K)$ where $K$ is the number of commitments for that specific faculty member (typically $1 \le K \le 4$, so practically instant).
- **Space Complexity**: $\mathcal{O}(K)$ dynamic heap space per teacher. Exactly 0 heap memory for teachers with no commitments.

---

### 5. Viva Questions & Ready Answers (Singly Linked List)

> **Q1: Why did you use a Singly Linked List for faculty commitments instead of an array?**  
> **Answer**: *"A Singly Linked List is dynamic and memory-efficient. Most internal departmental professors have zero outside commitments, so their head pointer is `nullptr` (0 extra memory). For visiting professors, memory is allocated on demand only for the specific slots they are busy. An array would either waste memory with empty slots or impose an artificial limit."*

> **Q2: How does insertion work in your linked list?**  
> **Answer**: *"We use Head Insertion in $\mathcal{O}(1)$ constant time. The new node's `next` pointer is set to the current head, and the head pointer is updated to the new node: `head = new CommitNode(day, slot, head);`."*

> **Q3: Where is this linked list evaluated in the scheduling engine?**  
> **Answer**: *"In `backend/scheduler.h` inside `checkSlotFreeInGrid()` under Constraint 6. Before allocating any slot to a professor, we traverse their `commitments` pointer chain (`c = c->next`). If any node matches the candidate `(day, slot)`, the slot is flagged as a clash and the scheduler tries the next slot."*

---

### Quick Summary Table for Presentation

| My Part | Data Structure | Algorithm | Key Function in Code | Viva One-Liner |
| :--- | :--- | :--- | :--- | :--- |
| **Topic 2** | Adjacency Matrix (`adj[MAX_DIVS][MAX_DIVS]`) | Breadth-First Search (BFS) with FIFO Queue | `divGraph.bfs()` in `backend/dsa.h` | *"Models divisions as nodes and shared professors as edges; BFS finds direct and indirect ripple effects."* |
| **Topic 4** | Singly Linked List (`CommitNode*`) | Dynamic Head Insertion & Pointer Traversal | `checkSlotFreeInGrid()` in `backend/scheduler.h` | *"Saves memory by dynamically allocating blocked slots only for visiting faculty who have external commitments."* |
