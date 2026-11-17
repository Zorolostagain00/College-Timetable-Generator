# DeptFlow V2 – Smart Department Timetable & Faculty Management System
**C++ DSA Mini-Project | Full CRUD | Winsock HTTP Server | Academic Scheduling**

DeptFlow V2 is a full-stack academic timetable and faculty workload management system designed for a **Data Structures and Algorithms (DSA)** course. It automates conflict-free weekly scheduling for multi-year academic departments, enforces 7 simultaneous resource constraints, provides full Create-Read-Update-Delete (CRUD) capabilities across all academic entities with dependency safety checks, and runs an interactive browser dashboard connected to a native C++ HTTP backend engine.

---

## 1. System Architecture (4-Layer Model)

```
┌─────────────────────────────────────────────────────────────┐
│                 Layer 1: Presentation Layer                 │
│         HTML5  •  Matte Emerald CSS3  •  Vanilla JS         │
│  (Interactive Timetable, CRUD Modals, Dynamic Granularity,  │
│   Division Pills, Print/PDF Export, Analytics Progress Bars)│
└──────────────────────────────┬──────────────────────────────┘
                               │ HTTP REST / JSON API
┌──────────────────────────────▼──────────────────────────────┐
│                Layer 2: Communication Layer                 │
│      Native Winsock2 C++ HTTP Server (localhost:8080)       │
│  (GET, POST, PUT, DELETE, OPTIONS, Static Asset Delivery)   │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                   Layer 3: C++ Core Engine                  │
│  • Max-Heap Priority Queue (Practicals 100/110, Outside 55) │
│  • Division Dependency Graph + BFS Traversal                │
│    (Distinguishes Direct vs Indirect transitive ripple)     │
│  • Faculty Hash Table with Separate Chaining (17 buckets)   │
│  • Singly Linked List for Outside Commitments               │
│  • Undo History Stack & Interactive Session Modification    │
│  • Multi-Resource 7-Constraint Validation Engine            │
│  • Fixed & Flexible Breaks Window Placement Engine          │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                   Layer 4: Storage Layer                    │
│                 C++ File I/O Persistence                    │
│ (department.txt, faculty.txt, subjects.txt, rooms.txt, ...)  │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Complete CRUD & Safety Operations

Every academic resource supports full Create, Read, Update, and Delete operations with dependency safeguards:

1. **Faculty Management**:
   - **Create / Edit**: ID, Name, Department, Category (Internal vs Outside), Max Weekly Hours, Subject Qualifications, Available Working Days (Mon–Sat bitmask), Outside Commitments.
   - **Delete (with Safety Check)**: Verifies that faculty is not assigned to any subjects or active scheduled sessions. Rejects deletion with explicit feedback if dependencies exist.
2. **Subject Management**:
   - **Create / Edit**: Subject Code, Title, Academic Year, Theory Faculty, Practical Faculty, Weekly Lectures, Lecture Duration (mins), Practical Blocks, Practical Duration (mins), Facility Requirement (Classroom vs Laboratory).
   - **Delete (with Safety Check)**: Blocks deletion if subject is currently scheduled in timetable sessions.
3. **Academic Divisions**:
   - **Create / Rename / Delete**: Allows adding, renaming, and removing divisions.
4. **Classrooms & Laboratories**:
   - **Create / Edit**: Code, Name, Facility Type (Theory Classroom vs Computing Lab), Capacity, Installed Equipment (Projector, GPUs, AV), Operational Availability toggle.
   - **Delete (with Safety Check)**: Blocks deletion if room is currently hosting scheduled classes.
5. **Break Intervals**:
   - **Fixed Breaks**: Specific day and slot (e.g. Lunch Break at 12:15–01:15).
   - **Flexible Breaks**: Dynamic placement within an allowed scheduling window (e.g. Tea Break between slot 1 and 3) without colliding with lectures or other breaks.
   - **Scope**: Department-wide, Year-specific, or Division-specific.
6. **Timetable Interactive Editing & Undo**:
   - Click any session card in the timetable grid to open the **Interactive Session Modal**.
   - **Edit Session**: Change assigned faculty or room for that slot.
   - **Delete Session**: Clear the scheduled session from the slot.
   - **Reschedule / Move**: Move session to another day and time slot with conflict validation.
   - **Global Undo**: Powered by the custom C++ `UndoStack`, reverts the last manual timetable edit.

---

## 3. Dynamic Timing & Single Source of Truth

- **Single Source of Truth**: Department configuration controls the master timing across the entire system.
- **Granularity Support**: Supports **15-minute**, **30-minute**, and **60-minute** intervals.
- **Dynamic Slot Generation**: Automatically computes daily slot counts and formats time intervals (e.g. `08:30-09:30`, `09:30-10:30`) based on `startTime`, `endTime`, and `interval`.
- **Configurable Working Days**: Custom working days (Monday through Saturday) dynamically update the timetable columns without hardcoding.

---

## 4. Manual DSA Implementations

All data structures and algorithms are implemented manually from scratch without STL containers or algorithms:

| Data Structure / Algorithm | File Location | Purpose & Practical Role | Time Complexity | Space Complexity |
| :--- | :--- | :--- | :--- | :--- |
| **Max-Heap (Priority Queue)** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L52-L80) | Prioritizes heavily constrained sessions: Outside Practicals (110) > Dept Practicals (100) > Outside Theory (55) > Dept Theory (30). | Push: $\mathcal{O}(\log N)$<br>Pop: $\mathcal{O}(\log N)$ | $\mathcal{O}(N)$ |
| **Adjacency Matrix Graph + BFS** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L82-L107) | Models division dependencies via shared faculty. BFS distinguishes **direct dependencies** (shared faculty edge) from **indirect dependencies** (transitive ripple path). | BFS: $\mathcal{O}(V^2)$ | $\mathcal{O}(V^2)$ |
| **Hash Table (Separate Chaining)** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L11-L47) | 17-bucket hash table with polynomial rolling hash for constant-time faculty ID lookup, insertion, and deletion. | Search: $\mathcal{O}(1)$ avg<br>Insert: $\mathcal{O}(1)$ | $\mathcal{O}(B + N)$ |
| **Singly Linked List** | [backend/models.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/models.h#L14-L18) | Dynamically records external commitment blocks (day, slot) for visiting outside faculty. | Search: $\mathcal{O}(K)$<br>Insert: $\mathcal{O}(1)$ | $\mathcal{O}(K)$ |
| **Undo Stack** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L109-L117) | LIFO stack storing session placements to allow interactive undo and rollback of manual timetable edits. | Push: $\mathcal{O}(1)$<br>Pop: $\mathcal{O}(1)$ | $\mathcal{O}(S)$ |
| **Insertion Sort** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L119-L127) | Chronologically orders session entries by day and time slot before sending to UI. | $\mathcal{O}(N^2)$ worst<br>$\mathcal{O}(N)$ best | $\mathcal{O}(1)$ aux |
| **Binary Search** | [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h#L134-L142) | Rapidly searches sorted subject records by subject ID code. | $\mathcal{O}(\log M)$ | $\mathcal{O}(1)$ |
| **3D Array / Occupancy Grid** | [backend/models.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/models.h#L111-L125) | Instant constant-time cell access: `grid[division][day][slot]`. | Access: $\mathcal{O}(1)$ | $\mathcal{O}(D \cdot W \cdot S)$ |

---

## 5. C++ Backend Source Line Counts

In accordance with requirement 13 ("prioritize core requirements and report the actual line count without unreadable compression"):

| Backend File | Lines of Code | Description |
| :--- | :---: | :--- |
| [backend/models.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/models.h) | 130 | Data entities, dynamic slot calculator, struct constructors |
| [backend/dsa.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/dsa.h) | 139 | Manual DSA (MaxHeap, Graph+BFS direct/indirect, FacultyHash, UndoStack, InsertionSort) |
| [backend/scheduler.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/scheduler.h) | 241 | 7-constraint validation engine, flexible break allocator, temporary memory generation |
| [backend/storage.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/storage.h) | 290 | Robust line-by-line file I/O persistence and clean CSDS seed data |
| [backend/api.h](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/api.h) | 575 | Full REST API (GET, POST, PUT, DELETE for all entities and timetable actions) |
| [backend/main.cpp](file:///c:/Zoro/C++/timetable%20generator/College-Timetable-Generator/backend/main.cpp) | 126 | Winsock HTTP server on port 8080 and static asset router |
| **Total Modular Backend** | **1,501** | **Complete Full-Stack C++ Backend Engine** |

---

## 6. QA Test Suite Results (18 / 18 Tests Passed)

The automated QA test suite (`test_suite.py`) verifies all 18 requirements from the user specification:

```
=== DEPTFLOW V2 QA TEST SUITE ===
[PASS] Test 1: Add and edit faculty
[PASS] Test 2: Delete unused faculty
[PASS] Test 3: Attempt to delete faculty with dependencies rejected correctly: 'Cannot delete: Faculty is assigned to subject 'Engg Math'. Reassign subject first.'
[PASS] Test 4: Add and edit subjects
[PASS] Test 5: Assign subjects to multiple divisions & verify list/cleanup
[PASS] Test 6: Add and delete rooms
[PASS] Test 7: Configure fixed breaks
[PASS] Test 8: Configure flexible breaks
[PASS] Test 9: Change department timings (Slots recalculated: 8)
[PASS] Test 10: Change working days (Now 6 days: ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'])
[PASS] Test 11: Generate selected divisions (Scheduled 16 sessions in FE-A & FE-B)
[PASS] Test 12: Preserve unselected divisions (Total active sessions: 50)
[PASS] Test 13: Test faculty conflicts (Zero overlapping faculty assignments)
[PASS] Test 14: Test room conflicts (Zero room collisions)
[PASS] Test 15: Test workload limits (All 6 faculty within capacity)
[PASS] Test 16: Test practical duration & laboratory facility mapping
[PASS] Test 17: Test data persistence on disk (Structured data files verified)
[PASS] Test 18: Test all dashboard reports (Direct dependencies: ['FE-B', 'SE-A', 'SE-B', 'BE-A'], Indirect: ['TE-A'])
---------------------------------
RESULTS: 18 / 18 TESTS PASSED.
ALL 18 AUDIT AND REFINEMENT TESTS PASSED PERFECTLY!
```

---

## 7. How to Compile & Run

### 1. Compile the Backend
```powershell
g++ -std=c++11 -Wall -Wextra backend/main.cpp -lws2_32 -o DeptFlow.exe
```

### 2. Run the Server
```powershell
.\DeptFlow.exe
```

### 3. Open in Browser
Visit **`http://localhost:8080`**.
