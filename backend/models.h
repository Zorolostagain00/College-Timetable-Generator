#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>

using namespace std;

// system capacity limits
const int MAX_DAYS = 7, MAX_SLOTS = 48, MAX_DIVS = 16, MAX_FAC = 25;
const int MAX_SUBS = 40, MAX_ROOMS = 20, MAX_BREAKS = 15, MAX_SESS = 400;

// linked list node storing external commitments of visiting faculty
struct CommitNode {
    int day, slot;
    CommitNode* next;
    CommitNode(int d = 0, int s = 0, CommitNode* n = nullptr) : day(d), slot(s), next(n) {}
};

// convert minutes from midnight to HH:MM format
inline string formatTime(int totalMinutes) {
    int h = (totalMinutes / 60) % 24;
    int m = totalMinutes % 60;
    ostringstream oss;
    oss << setfill('0') << setw(2) << h << ":" << setw(2) << m;
    return oss.str();
}

// parse HH:MM string to total minutes
inline int parseMinutes(const string& t) {
    size_t col = t.find(':');
    if (col == string::npos) return 0;
    int h = atoi(t.substr(0, col).c_str());
    int m = atoi(t.substr(col + 1).c_str());
    return h * 60 + m;
}

// department configuration and working hours
struct Department {
    string name, startTime, endTime, interval, lastGenerated;
    int numDays, numSlots;
    string days[MAX_DAYS], times[MAX_SLOTS];

    Department() : name("CSDS"), startTime("09:00"), endTime("16:00"), interval("60"),
                   lastGenerated("Not yet generated"), numDays(5), numSlots(6) {
        string d[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
        for (int i = 0; i < 5; i++) days[i] = d[i];
        recalculateSlots();
    }

    // recalculate slots when timing or interval changes
    void recalculateSlots() {
        int startM = parseMinutes(startTime);
        int endM = parseMinutes(endTime);
        int intM = atoi(interval.c_str());
        if (intM <= 0) intM = 60;
        if (endM <= startM) endM = startM + 360; // fallback 6 hrs
        numSlots = (endM - startM) / intM;
        if (numSlots > MAX_SLOTS) numSlots = MAX_SLOTS;
        if (numSlots < 1) numSlots = 1;
        for (int i = 0; i < numSlots; i++) {
            int slotStart = startM + i * intM;
            int slotEnd = slotStart + intM;
            times[i] = formatTime(slotStart) + "-" + formatTime(slotEnd);
        }
    }
};

// academic division (e.g. FE-A, SE-B)
struct Division {
    string name; int yearId;
    Division(string n = "", int y = 0) : name(n), yearId(y) {}
};

// faculty details, availability and commitments
struct Faculty {
    string id, name, dept, qualifications;
    bool isOutside;
    int maxHours, currHours;
    bool availDays[MAX_DAYS];
    CommitNode* commitments; // head of linked list for outside commitments

    Faculty(string i = "", string n = "", string d = "", bool o = false, int m = 16, int c = 0, string q = "")
        : id(i), name(n), dept(d), qualifications(q), isOutside(o), maxHours(m), currHours(c), commitments(nullptr) {
        for (int k = 0; k < MAX_DAYS; k++) availDays[k] = true;
    }
};

// academic subject with lecture & practical requirements
struct Subject {
    string id, name, roomTypeReq;
    int yearId, facIdx, practicalFacIdx;
    int weeklyLec, lecDuration, practicalCount, practicalDuration;
    int assignedDivs[MAX_DIVS]; int numAssignedDivs;

    Subject(string i = "", string n = "", int y = 0, int f = 0, int w = 3, int ld = 60, int p = 0, int pd = 120, string rt = "Classroom")
        : id(i), name(n), roomTypeReq(rt), yearId(y), facIdx(f), practicalFacIdx(f),
          weeklyLec(w), lecDuration(ld), practicalCount(p), practicalDuration(pd), numAssignedDivs(0) {}
};

// classroom or lab facility
struct Room {
    string id, name, type, equipment;
    int capacity, isLab;
    bool isAvailable;

    Room(string i = "", string n = "", string t = "Classroom", int c = 60, int l = 0, string eq = "Standard AV", bool av = true)
        : id(i), name(n), type(t), equipment(eq), capacity(c), isLab(l), isAvailable(av) {}
};

// fixed or flexible break
struct Break {
    string name;
    int day, slot, scopeType, scopeVal; // scope: 0=Dept, 1=Year, 2=Div
    bool isFlexible;
    int durationSlots, windowStartSlot, windowEndSlot;

    Break(string n = "", int d = 0, int s = 0, int st = 0, int sv = 0, bool flex = false, int dur = 1, int ws = 0, int we = 5)
        : name(n), day(d), slot(s), scopeType(st), scopeVal(sv),
          isFlexible(flex), durationSlots(dur), windowStartSlot(ws), windowEndSlot(we) {}
};

// single cell in timetable grid [div][day][slot]
struct Cell {
    int subIdx, facIdx, roomIdx, isBreak, isManual;
    string label;
    Cell(int s = -1, int f = -1, int r = -1, int b = 0, string l = "", int m = 0)
        : subIdx(s), facIdx(f), roomIdx(r), isBreak(b), isManual(m), label(l) {}
};

// stores conflict info for unscheduled sessions
struct UnscheduledInfo {
    string subName, divName, reason;
};

// snapshot of a timetable slot for undo
struct TimetableEntry {
    int divIdx, day, slot, subIdx, facIdx, roomIdx, isBreak, isManual;
    string label;
};

#endif
