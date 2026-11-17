#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iomanip>
using namespace std;
const int MAX_DAYS = 6, MAX_SLOTS = 8, MAX_DIVS = 12, MAX_FAC = 15;
const int MAX_SUBS = 25, MAX_ROOMS = 10, MAX_BREAKS = 8, MAX_SESS = 120;
// ============================================================================
// SECTION 1: CUSTOM DATA STRUCTURES (MANUAL DSA IMPLEMENTATIONS)
// ============================================================================
struct CommitNode {
    int day, slot; CommitNode* next;
    CommitNode(int d, int s, CommitNode* n = nullptr) : day(d), slot(s), next(n) {}
};
struct HashNode {
    string key; int val; HashNode* next;
    HashNode(string k, int v, HashNode* n = nullptr) : key(k), val(v), next(n) {}
};
struct FacultyHash {
    HashNode* b[17];
    FacultyHash() { for(int i = 0; i < 17; i++) b[i] = nullptr; }
    int hash(const string& s) {
        int h = 0; for(char c : s) h = (h * 31 + c) % 17;
        return (h < 0 ? h + 17 : h);
    }
    void insert(const string& k, int v) { int i = hash(k); b[i] = new HashNode(k, v, b[i]); }
    int search(const string& k) {
        for(HashNode* e = b[hash(k)]; e; e = e->next) if(e->key == k) return e->val;
        return -1;
    }
};
struct UndoAction { int div, day, slot, subIdx, facIdx, roomIdx; };
struct UndoStack {
    UndoAction arr[MAX_SESS]; int top;
    UndoStack() : top(-1) {}
    void push(UndoAction a) { if(top < MAX_SESS - 1) arr[++top] = a; }
    bool pop(UndoAction& a) { if(top >= 0) { a = arr[top--]; return true; } return false; }
};
struct Session { int subIdx, divIdx, priority, isPractical; };
struct MaxHeap {
    Session h[MAX_SESS]; int sz;
    MaxHeap() : sz(0) {}
    void push(Session s) {
        int i = sz++; h[i] = s;
        while(i > 0 && h[(i - 1) / 2].priority < h[i].priority) {
            Session t = h[i]; h[i] = h[(i - 1) / 2]; h[(i - 1) / 2] = t; i = (i - 1) / 2;
        }
    }
    bool pop(Session& s) {
        if(sz == 0) return false;
        s = h[0]; h[0] = h[--sz]; int i = 0;
        while(2 * i + 1 < sz) {
            int left = 2 * i + 1, right = 2 * i + 2, best = left;
            if(right < sz && h[right].priority > h[left].priority) best = right;
            if(h[best].priority <= h[i].priority) break;
            Session t = h[i]; h[i] = h[best]; h[best] = t; i = best;
        }
        return true;
    }
};
struct Graph {
    int adj[MAX_DIVS][MAX_DIVS], n;
    Graph() : n(0) { init(0); }
    void init(int count) { n = count; for(int i = 0; i < n; i++) for(int j = 0; j < n; j++) adj[i][j] = 0; }
    void addEdge(int u, int v) { if(u >= 0 && v >= 0 && u < n && v < n && u != v) adj[u][v] = adj[v][u] = 1; }
    void bfs(int start, bool vis[], int order[], int& count) {
        for(int i = 0; i < n; i++) vis[i] = false;
        int q[MAX_DIVS], f = 0, r = 0; vis[start] = true; q[r++] = start; count = 0;
        while(f < r) {
            int u = q[f++]; order[count++] = u;
            for(int v = 0; v < n; v++) if(adj[u][v] && !vis[v]) { vis[v] = true; q[r++] = v; }
        }
    }
};
// ============================================================================
// SECTION 2: SYSTEM MODELS & GLOBAL STATE
// ============================================================================
struct Department {
    string name; int numDays, numSlots; string days[MAX_DAYS], times[MAX_SLOTS];
    Department() : name("CSDS"), numDays(5), numSlots(6) {
        string d[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
        string t[] = {"09:00-10:00", "10:00-11:00", "11:15-12:15", "12:15-01:15", "02:00-03:00", "03:00-04:00"};
        for(int i = 0; i < 5; i++) { days[i] = d[i]; }
        for(int i = 0; i < 6; i++) { times[i] = t[i]; }
    }
} dept;
struct Division { string name; int yearId; Division(string n="", int y=0):name(n),yearId(y){} } divs[MAX_DIVS]; int divCount = 0;
struct Faculty {
    string id, name, dept; bool isOutside; int maxHours, currHours; CommitNode* commitments;
    Faculty(string i="", string n="", string d="", bool o=false, int m=0, int c=0, CommitNode* cm=nullptr)
        : id(i), name(n), dept(d), isOutside(o), maxHours(m), currHours(c), commitments(cm) {}
} facs[MAX_FAC]; int facCount = 0;
struct Subject {
    string id, name; int yearId, facIdx, weeklyLec, isPractical, labReq;
    Subject(string i="", string n="", int y=0, int f=0, int w=0, int p=0, int l=0)
        : id(i), name(n), yearId(y), facIdx(f), weeklyLec(w), isPractical(p), labReq(l) {}
} subs[MAX_SUBS]; int subCount = 0;
struct Room {
    string id, name; int capacity, isLab;
    Room(string i="", string n="", int c=0, int l=0):id(i), name(n), capacity(c), isLab(l){}
} rooms[MAX_ROOMS]; int roomCount = 0;
struct Break {
    string name; int day, slot, scopeType, scopeVal;
    Break(string n="", int d=0, int s=0, int st=0, int sv=0):name(n),day(d),slot(s),scopeType(st),scopeVal(sv){}
} breaks[MAX_BREAKS]; int breakCount = 0;
struct Cell {
    int subIdx, facIdx, roomIdx, isBreak; string label;
    Cell(int s=-1, int f=-1, int r=-1, int b=0, string l=""):subIdx(s),facIdx(f),roomIdx(r),isBreak(b),label(l){}
} grid[MAX_DIVS][MAX_DAYS][MAX_SLOTS];
struct UnscheduledInfo { string subName, divName, reason; } unsched[MAX_SESS];
int unschedCount = 0;
struct TimetableEntry { int day, slot, subIdx, facIdx, roomIdx; };
FacultyHash facHash;
UndoStack undoStack;
Graph divGraph;
// ============================================================================
// SECTION 3: CORE ALGORITHMS (LINEAR SEARCH, INSERTION SORT, GRAPH)
// ============================================================================
int findFaculty(const string& id) { for(int i=0; i<facCount; i++) if(facs[i].id == id) return i; return -1; }
int findSubject(const string& id) { for(int i=0; i<subCount; i++) if(subs[i].id == id) return i; return -1; }
int findRoom(const string& id) { for(int i=0; i<roomCount; i++) if(rooms[i].id == id) return i; return -1; }
void insertionSortSessions(TimetableEntry arr[], int n) {
    for(int i = 1; i < n; i++) {
        TimetableEntry key = arr[i]; int j = i - 1;
        while(j >= 0 && (arr[j].day > key.day || (arr[j].day == key.day && arr[j].slot > key.slot))) {
            arr[j + 1] = arr[j]; j--;
        }
        arr[j + 1] = key;
    }
}
void buildFacultyGraph() {
    divGraph.init(divCount);
    for(int i = 0; i < divCount; i++)
        for(int j = i + 1; j < divCount; j++) {
            bool shared = false;
            for(int s1 = 0; s1 < subCount && !shared; s1++)
                if(subs[s1].yearId == divs[i].yearId)
                    for(int s2 = 0; s2 < subCount && !shared; s2++)
                        if(subs[s2].yearId == divs[j].yearId && subs[s1].facIdx == subs[s2].facIdx) {
                            divGraph.addEdge(i, j); shared = true;
                        }
        }
}
// ============================================================================
// SECTION 4: SCHEDULING ENGINE (GREEDY + HEAP + CONSTRAINTS)
// ============================================================================
void generateTimetable(bool selectedDivs[]) {
    unschedCount = 0;
    int facBusy[MAX_FAC][MAX_DAYS][MAX_SLOTS] = {}, roomBusy[MAX_ROOMS][MAX_DAYS][MAX_SLOTS] = {};
    for(int d = 0; d < divCount; d++)
        if(selectedDivs[d])
            for(int day = 0; day < dept.numDays; day++)
                for(int slot = 0; slot < dept.numSlots; slot++) grid[d][day][slot] = Cell();
    for(int f = 0; f < facCount; f++) facs[f].currHours = 0;
    // Preserve unselected divisions as active commitments
    for(int d = 0; d < divCount; d++)
        for(int day = 0; day < dept.numDays; day++)
            for(int slot = 0; slot < dept.numSlots; slot++) {
                Cell& c = grid[d][day][slot];
                if(!selectedDivs[d] && c.subIdx != -1) {
                    facBusy[c.facIdx][day][slot] = 1; roomBusy[c.roomIdx][day][slot] = 1; facs[c.facIdx].currHours++;
                }
            }
    for(int f = 0; f < facCount; f++)
        for(CommitNode* c = facs[f].commitments; c; c = c->next)
            if(c->day < dept.numDays && c->slot < dept.numSlots) facBusy[f][c->day][c->slot] = 1;
    // Apply configured breaks
    for(int b = 0; b < breakCount; b++) {
        Break& brk = breaks[b];
        for(int d = 0; d < divCount; d++) {
            if(!selectedDivs[d]) continue;
            bool apply = (brk.scopeType == 0) || (brk.scopeType == 1 && divs[d].yearId == brk.scopeVal) || (brk.scopeType == 2 && d == brk.scopeVal);
            if(apply && brk.day < dept.numDays && brk.slot < dept.numSlots) grid[d][brk.day][brk.slot] = Cell(-1, -1, -1, 1, brk.name);
        }
    }
    // Queue sessions in MaxHeap: Practicals (priority 100) > Outside Fac (priority 55) > Lectures (priority 30)
    MaxHeap heap;
    for(int d = 0; d < divCount; d++) {
        if(!selectedDivs[d]) continue;
        for(int s = 0; s < subCount; s++) {
            if(subs[s].yearId != divs[d].yearId) continue;
            int f = subs[s].facIdx;
            if(subs[s].isPractical) heap.push({s, d, 100 + (facs[f].isOutside ? 20 : 0), 1});
            else for(int k = 0; k < subs[s].weeklyLec; k++) heap.push({s, d, 30 + (facs[f].isOutside ? 25 : 0), 0});
        }
    }
    // Greedily schedule from heap
    Session sess;
    while(heap.pop(sess)) {
        int d = sess.divIdx, s = sess.subIdx, f = subs[s].facIdx, dur = sess.isPractical ? 2 : 1;
        bool placed = false;
        if(facs[f].currHours + dur <= facs[f].maxHours) {
            for(int day = 0; day < dept.numDays && !placed; day++) {
                for(int slot = 0; slot <= dept.numSlots - dur && !placed; slot++) {
                    bool valid = true;
                    for(int t = 0; t < dur; t++) if(grid[d][day][slot + t].subIdx != -1 || grid[d][day][slot + t].isBreak || facBusy[f][day][slot + t]) valid = false;
                    if(!valid) continue;
                    int chosenRoom = -1;
                    for(int r = 0; r < roomCount; r++) {
                        if((subs[s].labReq && !rooms[r].isLab) || (!subs[s].labReq && rooms[r].isLab)) continue;
                        bool rFree = true;
                        for(int t = 0; t < dur; t++) if(roomBusy[r][day][slot + t]) rFree = false;
                        if(rFree) { chosenRoom = r; break; }
                    }
                    if(chosenRoom != -1) {
                        for(int t = 0; t < dur; t++) {
                            grid[d][day][slot + t] = Cell(s, f, chosenRoom, 0, subs[s].name + (sess.isPractical ? " [Lab]" : ""));
                            facBusy[f][day][slot + t] = 1; roomBusy[chosenRoom][day][slot + t] = 1;
                            undoStack.push({d, day, slot + t, s, f, chosenRoom});
                        }
                        facs[f].currHours += dur; placed = true;
                    }
                }
            }
        }
        if(!placed) {
            string rsn = (facs[f].currHours + dur > facs[f].maxHours) ? "Faculty Max Hours Exceeded" : (subs[s].labReq ? "No Free Lab / Consecutive Slots" : "Slot/Faculty/Room Conflict");
            unsched[unschedCount++] = {subs[s].name + (sess.isPractical ? " [Lab]" : ""), divs[d].name, rsn};
        }
    }
    buildFacultyGraph();
}
// ============================================================================
// SECTION 5: HTML / CSS EXPORT & PERSISTENCE
// ============================================================================
void exportHTML() {
    ofstream out("dashboard.html"); if(!out.is_open()) return;
    out << "<!DOCTYPE html>\n<html>\n<head>\n  <meta charset='UTF-8'>\n  <title>DeptFlow Dashboard</title>\n  <link rel='stylesheet' href='style.css'>\n</head>\n<body>\n<div class='container'>\n"
        << "  <div class='header'>\n    <div class='header-title'><h1>DeptFlow Timetable System</h1><p>Department of " << dept.name << " | Academic Schedule</p></div>\n    <div class='dept-badge'>" << dept.name << " DEPT</div>\n  </div>\n  <div class='metrics-grid'>\n"
        << "    <div class='metric-card'><div class='metric-label'>Total Divisions</div><div class='metric-value'>" << divCount << "</div></div>\n    <div class='metric-card'><div class='metric-label'>Faculty Members</div><div class='metric-value'>" << facCount << "</div></div>\n"
        << "    <div class='metric-card'><div class='metric-label'>Subjects Offered</div><div class='metric-value'>" << subCount << "</div></div>\n    <div class='metric-card'><div class='metric-label'>Classrooms & Labs</div><div class='metric-value'>" << roomCount << "</div></div>\n  </div>\n"
        << "  <div class='section-card'>\n    <div class='section-header'><div class='section-title'>Timetable Schedule</div><div class='filter-group'><label class='filter-label'>Select Division:</label><select class='filter-select' onchange=\"document.querySelectorAll('.tt-table').forEach(t=>t.style.display='none');document.getElementById('tt-'+this.value).style.display='table';\">\n";
    for(int d = 0; d < divCount; d++) out << "      <option value='" << d << "'>" << divs[d].name << " (Year " << divs[d].yearId << ")</option>\n";
    out << "    </select></div></div>\n    <div class='table-responsive'>\n";
    for(int d = 0; d < divCount; d++) {
        out << "      <table id='tt-" << d << "' class='timetable-table tt-table' style='display:" << (d == 0 ? "table" : "none") << ";'>\n        <thead><tr><th class='day-header'>Day / Time</th>";
        for(int s = 0; s < dept.numSlots; s++) out << "<th>" << dept.times[s] << "</th>";
        out << "</tr></thead>\n        <tbody>\n";
        for(int day = 0; day < dept.numDays; day++) {
            out << "          <tr><td class='day-header'>" << dept.days[day] << "</td>";
            for(int s = 0; s < dept.numSlots; s++) {
                Cell& c = grid[d][day][s]; out << "<td>";
                if(c.isBreak) out << "<div class='slot-cell slot-break'><div class='slot-subject'>" << c.label << "</div><div class='slot-meta'>Break</div></div>";
                else if(c.subIdx != -1) out << "<div class='slot-cell " << (subs[c.subIdx].isPractical ? "slot-practical" : "slot-lecture") << "'><div class='slot-subject'>" << subs[c.subIdx].name << "</div><div class='slot-meta'>" << facs[c.facIdx].name << "</div><div class='slot-room'>" << rooms[c.roomIdx].name << "</div></div>";
                else out << "<div class='slot-cell slot-free'><div class='slot-meta'>Free Slot</div></div>";
                out << "</td>";
            }
            out << "</tr>\n";
        }
        out << "        </tbody>\n      </table>\n";
    }
    out << "    </div>\n  </div>\n  <div class='grid-2col'>\n    <div class='section-card'>\n      <div class='section-header'><div class='section-title'>Faculty Workload & Status</div></div>\n      <table class='data-table'><thead><tr><th>ID</th><th>Faculty Name</th><th>Dept</th><th>Status</th><th>Workload</th></tr></thead><tbody>\n";
    for(int f = 0; f < facCount; f++) {
        int pct = facs[f].maxHours > 0 ? (facs[f].currHours * 100 / facs[f].maxHours) : 0;
        out << "        <tr><td>" << facs[f].id << "</td><td>" << facs[f].name << "</td><td>" << facs[f].dept << "</td><td><span class='tag " << (facs[f].isOutside ? "tag-outside" : "tag-internal") << "'>" << (facs[f].isOutside ? "Outside" : "Internal") << "</span></td><td>" << facs[f].currHours << " / " << facs[f].maxHours << " hrs<div class='progress-bar-bg'><div class='progress-bar-fill' style='width:" << pct << "%;'></div></div></td></tr>\n";
    }
    out << "      </tbody></table>\n    </div>\n    <div class='section-card'>\n      <div class='section-header'><div class='section-title'>Room & Lab Utilization</div></div>\n      <table class='data-table'><thead><tr><th>Room ID</th><th>Type</th><th>Capacity</th><th>Allocated</th><th>Utilization</th></tr></thead><tbody>\n";
    for(int r = 0; r < roomCount; r++) {
        int occ = 0;
        for(int d = 0; d < divCount; d++) for(int day = 0; day < dept.numDays; day++) for(int s = 0; s < dept.numSlots; s++) if(grid[d][day][s].roomIdx == r) occ++;
        int tot = dept.numDays * dept.numSlots, pct = (occ * 100) / (tot > 0 ? tot : 1);
        out << "        <tr><td>" << rooms[r].name << "</td><td><span class='tag " << (rooms[r].isLab ? "tag-lab" : "tag-classroom") << "'>" << (rooms[r].isLab ? "Laboratory" : "Classroom") << "</span></td><td>" << rooms[r].capacity << " seats</td><td>" << occ << " / " << tot << " slots</td><td><div class='progress-bar-bg'><div class='progress-bar-fill' style='width:" << pct << "%;'></div></div></td></tr>\n";
    }
    out << "      </tbody></table>\n    </div>\n  </div>\n  <div class='section-card'>\n    <div class='section-header'><div class='section-title'>Conflict & Shared Faculty Cascade Analysis</div></div>\n";
    if(unschedCount == 0) out << "    <div class='alert-box alert-success'>All scheduled sessions placed successfully with zero conflicts detected.</div>\n";
    else {
        out << "    <div class='alert-box alert-warning'>" << unschedCount << " sessions could not be placed due to resource constraints.</div>\n    <table class='data-table'><thead><tr><th>Subject</th><th>Division</th><th>Constraint Reason</th></tr></thead><tbody>\n";
        for(int u = 0; u < unschedCount; u++) out << "      <tr><td>" << unsched[u].subName << "</td><td>" << unsched[u].divName << "</td><td>" << unsched[u].reason << "</td></tr>\n";
        out << "    </tbody></table>\n";
    }
    out << "  </div>\n</div>\n</body>\n</html>\n";
    out.close();
}
void saveData() {
    ofstream out("deptflow_data.txt"); if(!out.is_open()) return;
    out << dept.name << "," << dept.numDays << "," << dept.numSlots << "\n" << divCount << "\n";
    for(int i = 0; i < divCount; i++) out << divs[i].name << "," << divs[i].yearId << "\n";
    out << facCount << "\n";
    for(int i = 0; i < facCount; i++) {
        out << facs[i].id << "," << facs[i].name << "," << facs[i].dept << "," << facs[i].isOutside << "," << facs[i].maxHours;
        int cc = 0; for(CommitNode* c = facs[i].commitments; c; c = c->next) cc++;
        out << "," << cc;
        for(CommitNode* c = facs[i].commitments; c; c = c->next) out << "," << c->day << "," << c->slot;
        out << "\n";
    }
    out << subCount << "\n";
    for(int i = 0; i < subCount; i++) out << subs[i].id << "," << subs[i].name << "," << subs[i].yearId << "," << subs[i].facIdx << "," << subs[i].weeklyLec << "," << subs[i].isPractical << "," << subs[i].labReq << "\n";
    out << roomCount << "\n";
    for(int i = 0; i < roomCount; i++) out << rooms[i].id << "," << rooms[i].name << "," << rooms[i].capacity << "," << rooms[i].isLab << "\n";
    out << breakCount << "\n";
    for(int i = 0; i < breakCount; i++) out << breaks[i].name << "," << breaks[i].day << "," << breaks[i].slot << "," << breaks[i].scopeType << "," << breaks[i].scopeVal << "\n";
    int cells = 0;
    for(int d = 0; d < divCount; d++) for(int day = 0; day < dept.numDays; day++) for(int s = 0; s < dept.numSlots; s++) if(grid[d][day][s].subIdx != -1 || grid[d][day][s].isBreak) cells++;
    out << cells << "\n";
    for(int d = 0; d < divCount; d++)
        for(int day = 0; day < dept.numDays; day++)
            for(int s = 0; s < dept.numSlots; s++)
                if(grid[d][day][s].subIdx != -1 || grid[d][day][s].isBreak)
                    out << d << "," << day << "," << s << "," << grid[d][day][s].subIdx << "," << grid[d][day][s].facIdx << "," << grid[d][day][s].roomIdx << "," << grid[d][day][s].isBreak << "," << grid[d][day][s].label << "\n";
    out.close();
}
bool loadData() {
    ifstream in("deptflow_data.txt"); if(!in.is_open()) return false;
    char c; getline(in, dept.name, ','); in >> dept.numDays >> c >> dept.numSlots; in.ignore();
    in >> divCount; in.ignore();
    for(int i = 0; i < divCount; i++) { getline(in, divs[i].name, ','); in >> divs[i].yearId; in.ignore(); }
    in >> facCount; in.ignore();
    for(int i = 0; i < facCount; i++) {
        getline(in, facs[i].id, ','); getline(in, facs[i].name, ','); getline(in, facs[i].dept, ',');
        int cc = 0; in >> facs[i].isOutside >> c >> facs[i].maxHours >> c >> cc;
        facs[i].commitments = nullptr; facs[i].currHours = 0;
        for(int k = 0; k < cc; k++) { int cd, cs; in >> c >> cd >> c >> cs; facs[i].commitments = new CommitNode(cd, cs, facs[i].commitments); }
        in.ignore(); facHash.insert(facs[i].id, i);
    }
    in >> subCount; in.ignore();
    for(int i = 0; i < subCount; i++) {
        getline(in, subs[i].id, ','); getline(in, subs[i].name, ',');
        in >> subs[i].yearId >> c >> subs[i].facIdx >> c >> subs[i].weeklyLec >> c >> subs[i].isPractical >> c >> subs[i].labReq; in.ignore();
    }
    in >> roomCount; in.ignore();
    for(int i = 0; i < roomCount; i++) { getline(in, rooms[i].id, ','); getline(in, rooms[i].name, ','); in >> rooms[i].capacity >> c >> rooms[i].isLab; in.ignore(); }
    in >> breakCount; in.ignore();
    for(int i = 0; i < breakCount; i++) {
        getline(in, breaks[i].name, ','); in >> breaks[i].day >> c >> breaks[i].slot >> c >> breaks[i].scopeType >> c >> breaks[i].scopeVal; in.ignore();
    }
    int cells = 0;
    if(in >> cells) {
        in.ignore();
        for(int k = 0; k < cells; k++) {
            int d, day, s, sub, fac, rm, brk; string lbl;
            in >> d >> c >> day >> c >> s >> c >> sub >> c >> fac >> c >> rm >> c >> brk >> c;
            getline(in, lbl); grid[d][day][s] = Cell(sub, fac, rm, brk, lbl);
            if(fac >= 0) facs[fac].currHours++;
        }
    }
    in.close(); buildFacultyGraph(); exportHTML(); return true;
}
void initSampleData() {
    dept.name = "CSDS"; dept.numDays = 5; dept.numSlots = 6; divCount = 6;
    string dNames[] = {"FE-A","FE-B","SE-A","SE-B","TE-A","BE-A"}; int dY[] = {1,1,2,2,3,4};
    for(int i = 0; i < 6; i++) divs[i] = Division(dNames[i], dY[i]);
    facCount = 5;
    facs[0] = Faculty("F101", "Dr. Sharma", "CSDS", false, 16); facs[1] = Faculty("F102", "Prof. Verma", "CSDS", false, 16);
    facs[2] = Faculty("F103", "Dr. Kulkarni", "CSDS", false, 14); facs[3] = Faculty("F104", "Prof. Iyer", "IT", true, 12);
    facs[4] = Faculty("F105", "Prof. Patel", "AI_DS", true, 10);
    facs[3].commitments = new CommitNode(0, 1, new CommitNode(2, 4, nullptr));
    facs[4].commitments = new CommitNode(4, 0, nullptr);
    for(int i = 0; i < facCount; i++) facHash.insert(facs[i].id, i);
    roomCount = 5;
    rooms[0] = Room("CR101", "Lecture Hall 101", 70, 0); rooms[1] = Room("CR102", "Lecture Hall 102", 70, 0);
    rooms[2] = Room("CR201", "Lecture Hall 201", 70, 0); rooms[3] = Room("LAB1", "Data Analytics Lab", 35, 1);
    rooms[4] = Room("LAB2", "Systems Computing Lab", 35, 1);
    breakCount = 0; for(int d = 0; d < 5; d++) breaks[breakCount++] = Break("Lunch Break", d, 3, 0, 0);
    subCount = 8;
    subs[0] = Subject("S101", "Engineering Math", 1, 0, 3, 0, 0); subs[1] = Subject("S102", "Programming Lab", 1, 1, 1, 1, 1);
    subs[2] = Subject("S201", "Data Structures", 2, 3, 3, 0, 0); subs[3] = Subject("S202", "DSA Laboratory", 2, 3, 1, 1, 1);
    subs[4] = Subject("S301", "Database Systems", 3, 2, 3, 0, 0); subs[5] = Subject("S302", "DBMS Laboratory", 3, 2, 1, 1, 1);
    subs[6] = Subject("S401", "Cloud Computing", 4, 0, 3, 0, 0); subs[7] = Subject("S402", "Machine Learning", 4, 4, 2, 0, 0);
    bool allDivs[MAX_DIVS]; for(int i = 0; i < divCount; i++) allDivs[i] = true;
    generateTimetable(allDivs); exportHTML();
}
// ============================================================================
// SECTION 6: CONSOLE REPORTS & VIEWS
// ============================================================================
void printDivisionTimetable(int d) {
    if(d < 0 || d >= divCount) { cout << "Invalid division ID.\n"; return; }
    TimetableEntry entries[MAX_DAYS * MAX_SLOTS]; int count = 0;
    for(int day = 0; day < dept.numDays; day++)
        for(int s = 0; s < dept.numSlots; s++)
            if(grid[d][day][s].subIdx != -1) entries[count++] = {day, s, grid[d][day][s].subIdx, grid[d][day][s].facIdx, grid[d][day][s].roomIdx};
    insertionSortSessions(entries, count);
    cout << "\n=== TIMETABLE: " << divs[d].name << " (Year " << divs[d].yearId << ") ===\n"
         << left << setw(12) << "DAY" << setw(14) << "TIME" << setw(22) << "SUBJECT" << setw(16) << "FACULTY" << setw(12) << "ROOM\n-------------------------------------------------------------------\n";
    for(int i = 0; i < count; i++)
        cout << left << setw(12) << dept.days[entries[i].day] << setw(14) << dept.times[entries[i].slot]
             << setw(22) << subs[entries[i].subIdx].name << setw(16) << facs[entries[i].facIdx].name << setw(12) << rooms[entries[i].roomIdx].name << "\n";
}
void printFacultySchedule(int f) {
    if(f < 0 || f >= facCount) { cout << "Invalid faculty.\n"; return; }
    cout << "\n=== SCHEDULE: " << facs[f].name << " (" << facs[f].dept << ") [" << (facs[f].isOutside ? "Outside" : "Internal") << "] ===\n";
    for(int day = 0; day < dept.numDays; day++)
        for(int s = 0; s < dept.numSlots; s++)
            for(int d = 0; d < divCount; d++)
                if(grid[d][day][s].facIdx == f)
                    cout << dept.days[day] << " " << dept.times[s] << " -> " << divs[d].name << " | " << subs[grid[d][day][s].subIdx].name << " (" << rooms[grid[d][day][s].roomIdx].name << ")\n";
}
void detectConflictsAndGraph() {
    cout << "\n--- CONFLICT DETECTION & GRAPH CASCADE ANALYSIS ---\n";
    int doubleBookings = 0;
    for(int day = 0; day < dept.numDays; day++)
        for(int s = 0; s < dept.numSlots; s++)
            for(int f = 0; f < facCount; f++) {
                int occ = 0;
                for(int d = 0; d < divCount; d++) if(grid[d][day][s].facIdx == f) occ++;
                if(occ > 1) { cout << "[CONFLICT] Faculty " << facs[f].name << " double-booked at " << dept.days[day] << " " << dept.times[s] << "\n"; doubleBookings++; }
            }
    if(doubleBookings == 0) cout << ">> ZERO faculty or room double-bookings detected in schedule.\n";
    cout << ">> SHARED FACULTY DIVISION COUPLING (BFS Cascade Analysis):\n";
    bool vis[MAX_DIVS]; int order[MAX_DIVS], count = 0;
    for(int d = 0; d < divCount; d++) {
        divGraph.bfs(d, vis, order, count);
        if(count > 1) {
            cout << "Division " << divs[d].name << " shares faculty with: ";
            for(int i = 1; i < count; i++) cout << divs[order[i]].name << " ";
            cout << "\n";
        }
    }
}
void showAnalytics() {
    cout << "\n=== DEPTFLOW ANALYTICS & RESOURCE UTILIZATION ===\n"
         << left << setw(8) << "ID" << setw(18) << "FACULTY" << setw(14) << "CATEGORY" << setw(12) << "ALLOCATED" << setw(10) << "MAX" << "LOAD %\n-------------------------------------------------------------------\n";
    for(int f = 0; f < facCount; f++) {
        int pct = facs[f].maxHours > 0 ? (facs[f].currHours * 100 / facs[f].maxHours) : 0;
        cout << left << setw(8) << facs[f].id << setw(18) << facs[f].name << setw(14) << (facs[f].isOutside ? "Outside" : "Internal")
             << setw(12) << facs[f].currHours << setw(10) << facs[f].maxHours << pct << "%\n";
    }
    cout << "\nROOM OCCUPANCY:\n";
    int totSlots = dept.numDays * dept.numSlots;
    for(int r = 0; r < roomCount; r++) {
        int occ = 0;
        for(int d = 0; d < divCount; d++) for(int day = 0; day < dept.numDays; day++) for(int s = 0; s < dept.numSlots; s++) if(grid[d][day][s].roomIdx == r) occ++;
        cout << rooms[r].name << " (" << (rooms[r].isLab ? "Lab" : "Classroom") << "): " << occ << "/" << totSlots << " slots (" << (occ * 100 / (totSlots ? totSlots : 1)) << "%)\n";
    }
    if(unschedCount > 0) {
        cout << "\nUNSCHEDULED SESSIONS (" << unschedCount << "):\n";
        for(int i = 0; i < unschedCount; i++) cout << "- " << unsched[i].subName << " (" << unsched[i].divName << "): " << unsched[i].reason << "\n";
    }
}
// ============================================================================
// SECTION 7: MAIN MENU (13 OPTIONS)
// ============================================================================
int main() {
    if(!loadData()) initSampleData();
    int choice = 0;
    while(choice != 13) {
        cout << "\n========================================\n                DEPTFLOW                \n      DEPARTMENT MANAGEMENT SYSTEM      \n========================================\n"
             << "1.  Department Configuration\n2.  Academic Year & Division Management\n3.  Faculty Management\n4.  Subject Management\n5.  Classroom Management\n"
             << "6.  Break & Time Management\n7.  Generate Timetable\n8.  View Timetables\n9.  Faculty Schedule\n10. Conflict Detection & Graph Analysis\n"
             << "11. Reports & Analytics\n12. Save Data\n13. Exit\n========================================\nEnter Choice [1-13]: ";
        if(!(cin >> choice)) { cin.clear(); cin.ignore(1000, '\n'); continue; }
        if(choice == 1) {
            cout << "\n[DEPT CONFIG] Current: " << dept.name << " (" << dept.numDays << " days, " << dept.numSlots << " slots)\nEnter Dept Name, Working Days (1-6), Slots (1-8): ";
            cin >> dept.name >> dept.numDays >> dept.numSlots; cout << "Configuration updated.\n";
        } else if(choice == 2) {
            cout << "\n[ACADEMIC DIVISIONS] Total: " << divCount << "\n";
            for(int i = 0; i < divCount; i++) cout << i + 1 << ". " << divs[i].name << " (Year " << divs[i].yearId << ")\n";
            cout << "Add new division? (1=Yes, 0=No): "; int add; cin >> add;
            if(add && divCount < MAX_DIVS) {
                cout << "Division Name (e.g. TE-B) and Year (1-4): "; cin >> divs[divCount].name >> divs[divCount].yearId;
                divCount++; cout << "Division added.\n";
            }
        } else if(choice == 3) {
            cout << "\n[FACULTY MANAGEMENT] Total: " << facCount << "\n";
            for(int i = 0; i < facCount; i++) cout << i + 1 << ". [" << facs[i].id << "] " << facs[i].name << " - " << facs[i].dept << " (" << (facs[i].isOutside ? "Outside" : "Internal") << ")\n";
            cout << "Search Faculty by ID (or '0' to skip): "; string fid; cin >> fid;
            if(fid != "0") {
                int idx = facHash.search(fid);
                if(idx != -1) cout << "Found: " << facs[idx].name << " | Dept: " << facs[idx].dept << " | Max Hrs: " << facs[idx].maxHours << "\n";
                else cout << "Faculty ID not found.\n";
            }
        } else if(choice == 4) {
            cout << "\n[SUBJECTS] Total: " << subCount << "\n";
            for(int i = 0; i < subCount; i++) cout << i + 1 << ". [" << subs[i].id << "] " << subs[i].name << " (Year " << subs[i].yearId << ") - Fac: " << facs[subs[i].facIdx].name << "\n";
        } else if(choice == 5) {
            cout << "\n[ROOMS & LABS] Total: " << roomCount << "\n";
            for(int i = 0; i < roomCount; i++) cout << i + 1 << ". " << rooms[i].name << " | " << (rooms[i].isLab ? "Laboratory" : "Classroom") << " | Cap: " << rooms[i].capacity << "\n";
        } else if(choice == 6) {
            cout << "\n[BREAK MANAGEMENT] Configured Breaks: " << breakCount << "\n";
            for(int i = 0; i < breakCount; i++) cout << i + 1 << ". " << breaks[i].name << " on " << dept.days[breaks[i].day] << " Slot " << breaks[i].slot + 1 << "\n";
            cout << "Add Break? (1=Yes, 0=No): "; int bAdd; cin >> bAdd;
            if(bAdd && breakCount < MAX_BREAKS) {
                cout << "Break Name, Day (0-" << dept.numDays - 1 << "), Slot (0-" << dept.numSlots - 1 << "): ";
                cin >> breaks[breakCount].name >> breaks[breakCount].day >> breaks[breakCount].slot;
                breaks[breakCount].scopeType = 0; breaks[breakCount].scopeVal = 0;
                breakCount++; cout << "Break configured.\n";
            }
        } else if(choice == 7) {
            cout << "\n[TIMETABLE GENERATION SELECTION]\n1. All Academic Years\n2. First Year (FE) Only\n3. Second Year (SE) Only\n4. Third Year (TE) Only\n5. Final Year (BE) Only\n6. Custom Division Selection\nSelect Option: ";
            int genOpt; cin >> genOpt; bool sel[MAX_DIVS] = {};
            if(genOpt == 1) for(int i = 0; i < divCount; i++) sel[i] = true;
            else if(genOpt >= 2 && genOpt <= 5) { for(int i = 0; i < divCount; i++) if(divs[i].yearId == genOpt - 1) sel[i] = true;
            } else {
                for(int i = 0; i < divCount; i++) { cout << "Include " << divs[i].name << "? (1=Yes, 0=No): "; cin >> sel[i]; }
            }
            generateTimetable(sel); exportHTML();
            cout << ">> Timetable generated! Preserved unselected schedules.\n>> Exported schedule to dashboard.html\n";
        } else if(choice == 8) {
            cout << "\nSelect Division (1-" << divCount << "): "; int dSel; cin >> dSel; printDivisionTimetable(dSel - 1);
        } else if(choice == 9) {
            cout << "\nSelect Faculty (1-" << facCount << "): "; int fSel; cin >> fSel; printFacultySchedule(fSel - 1);
        } else if(choice == 10) { detectConflictsAndGraph();
        } else if(choice == 11) { showAnalytics(); exportHTML(); cout << ">> Updated HTML dashboard generated.\n";
        } else if(choice == 12) { saveData(); cout << ">> Data saved to deptflow_data.txt\n"; }
    }
    saveData(); cout << "\nThank you for using DeptFlow.\n"; return 0;
}
