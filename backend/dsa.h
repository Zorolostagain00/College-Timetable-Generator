#ifndef DSA_H //if not defined DSA_H define dsa_H
#define DSA_H

#include "models.h"

struct HashNode {
    string key; int val; HashNode* next;
    HashNode(string k, int v, HashNode* n = nullptr) : key(k), val(v), next(n) {}
};

class FacultyHash {
    static const int BUCKETS = 17;
    HashNode* table[BUCKETS];
    int hash(const string& s) {
        unsigned long h = 0;
        for (char c : s) h = (h * 31 + c) % BUCKETS;
        return (int)h;
    }
public:
    FacultyHash() { for (int i = 0; i < BUCKETS; i++) table[i] = nullptr; }
    void insert(const string& k, int v) {
        int idx = hash(k);
        table[idx] = new HashNode(k, v, table[idx]);
    }
    int search(const string& k) {
        int idx = hash(k);
        HashNode* curr = table[idx];
        while (curr) {
            if (curr->key == k) return curr->val;
            curr = curr->next;
        }
        return -1;
    }
    void remove(const string& k) {
        int idx = hash(k);
        HashNode* curr = table[idx]; HashNode* prev = nullptr;
        while (curr) {
            if (curr->key == k) {
                if (prev) prev->next = curr->next;
                else table[idx] = curr->next;
                delete curr; return;
            }
            prev = curr; curr = curr->next;
        }
    }
};

struct Session {
    int subIdx, divIdx, facIdx, type, duration, priority;
    Session(int s = -1, int di = -1, int f = -1, int t = 0, int dur = 1, int p = 30)
        : subIdx(s), divIdx(di), facIdx(f), type(t), duration(dur), priority(p) {}
};

class MaxHeap {
    Session heap[MAX_SESS];
    int count;
    void swapS(int a, int b) { Session t = heap[a]; heap[a] = heap[b]; heap[b] = t; }
    void siftUp(int i) {
        while (i > 0 && heap[i].priority > heap[(i - 1) / 2].priority) {
            swapS(i, (i - 1) / 2); i = (i - 1) / 2;
        }
    }
    void siftDown(int i) {
        int maxI = i, l = 2 * i + 1, r = 2 * i + 2;
        if (l < count && heap[l].priority > heap[maxI].priority) maxI = l;
        if (r < count && heap[r].priority > heap[maxI].priority) maxI = r;
        if (maxI != i) { swapS(i, maxI); siftDown(maxI); }
    }
public:
    MaxHeap() : count(0) {}
    void push(const Session& s) { if (count < MAX_SESS) { heap[count] = s; siftUp(count++); } }
    Session pop() { Session top = heap[0]; heap[0] = heap[--count]; siftDown(0); return top; }
    bool empty() const { return count == 0; }
    int size() const { return count; }
};

class Graph {
    int adj[MAX_DIVS][MAX_DIVS];
public:
    Graph() { for (int i = 0; i < MAX_DIVS; i++) for (int j = 0; j < MAX_DIVS; j++) adj[i][j] = 0; }
    void addEdge(int u, int v) { if (u < MAX_DIVS && v < MAX_DIVS) { adj[u][v] = 1; adj[v][u] = 1; } }
    bool isDirect(int u, int v) const { return (u < MAX_DIVS && v < MAX_DIVS) ? (adj[u][v] == 1) : false; }
    void bfs(int start, int n, int* visitedOrder, int& orderCount, bool* directFlags) {
        bool visited[MAX_DIVS] = {false};
        int queue[MAX_DIVS], front = 0, rear = 0;
        visited[start] = true; queue[rear++] = start;
        orderCount = 0;
        while (front < rear) {
            int u = queue[front++];
            if (u != start) {
                visitedOrder[orderCount] = u;
                directFlags[orderCount] = (adj[start][u] == 1);
                orderCount++;
            }
            for (int v = 0; v < n; v++) {
                if (adj[u][v] && !visited[v]) {
                    visited[v] = true; queue[rear++] = v;
                }
            }
        }
    }
};

class UndoStack {
    TimetableEntry data[MAX_SESS]; int topIdx;
public:
    UndoStack() : topIdx(-1) {}
    void push(const TimetableEntry& e) { if (topIdx < MAX_SESS - 1) data[++topIdx] = e; }
    TimetableEntry pop() { return (topIdx >= 0) ? data[topIdx--] : TimetableEntry{-1,-1,-1,-1,-1,-1,0,0,""}; }
    bool empty() const { return topIdx == -1; }
    int size() const { return topIdx + 1; }
};

inline void insertionSortSessions(TimetableEntry arr[], int n) {
    for (int i = 1; i < n; i++) {
        TimetableEntry key = arr[i]; int j = i - 1;
        while (j >= 0 && (arr[j].day > key.day || (arr[j].day == key.day && arr[j].slot > key.slot))) {
            arr[j + 1] = arr[j]; j--;
        }
        arr[j + 1] = key;
    }
}

inline int findFaculty(const Faculty facs[], int n, const string& id) {
    for (int i = 0; i < n; i++) if (facs[i].id == id) return i;
    return -1;
}

inline int binarySearchSubject(const Subject subs[], int n, const string& id) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (subs[mid].id == id) return mid;
        if (subs[mid].id < id) low = mid + 1; else high = mid - 1;
    }
    return -1;
}

#endif
