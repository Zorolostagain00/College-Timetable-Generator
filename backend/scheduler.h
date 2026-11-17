#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <ctime>
#include <algorithm>
#include "models.h"
#include "dsa.h"

extern Department dept;
extern Division divisions[MAX_DIVS]; extern int numDivs;
extern Faculty faculties[MAX_FAC]; extern int numFac;
extern Subject subjects[MAX_SUBS]; extern int numSubs;
extern Room rooms[MAX_ROOMS]; extern int numRooms;
extern Break breaks[MAX_BREAKS]; extern int numBreaks;
extern Cell grid[MAX_DIVS][MAX_DAYS][MAX_SLOTS];
extern UnscheduledInfo unscheduled[MAX_SESS]; extern int numUnsched;
extern Graph divGraph;
extern UndoStack undoStack;
extern FacultyHash facHash;

inline string getCurrentTimestamp() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M:%S", localtime(&now));
    return string(buf);
}

inline void buildFacultyGraph() {
    divGraph = Graph();
    for (int i = 0; i < numDivs; i++) {
        for (int j = i + 1; j < numDivs; j++) {
            bool shared = false;
            for (int s1 = 0; s1 < numSubs && !shared; s1++) {
                bool iAssigned = false;
                if (subjects[s1].numAssignedDivs > 0) {
                    for (int k = 0; k < subjects[s1].numAssignedDivs; k++)
                        if (subjects[s1].assignedDivs[k] == i) iAssigned = true;
                } else if (subjects[s1].yearId == divisions[i].yearId) iAssigned = true;

                if (!iAssigned) continue;

                for (int s2 = 0; s2 < numSubs && !shared; s2++) {
                    bool jAssigned = false;
                    if (subjects[s2].numAssignedDivs > 0) {
                        for (int k = 0; k < subjects[s2].numAssignedDivs; k++)
                            if (subjects[s2].assignedDivs[k] == j) jAssigned = true;
                    } else if (subjects[s2].yearId == divisions[j].yearId) jAssigned = true;

                    if (jAssigned && (subjects[s1].facIdx == subjects[s2].facIdx ||
                                      subjects[s1].practicalFacIdx == subjects[s2].practicalFacIdx)) {
                        shared = true;
                    }
                }
            }
            if (shared) divGraph.addEdge(i, j);
        }
    }
}

inline bool checkSlotFreeInGrid(const Cell targetGrid[MAX_DIVS][MAX_DAYS][MAX_SLOTS],
                               int divIdx, int day, int slot, int dur, int facIdx,
                               bool needLab, int& assignedRoom, string& reason) {
    if (slot + dur > dept.numSlots) { reason = "Session exceeds daily working hours"; return false; }
    if (facIdx >= 0) {
        if (!faculties[facIdx].availDays[day]) { reason = "Faculty unavailable on this working day"; return false; }
        int intM = atoi(dept.interval.c_str()); if (intM <= 0) intM = 60;
        int hoursToAdd = max(1, (dur * intM) / 60);
        if (faculties[facIdx].currHours + hoursToAdd > faculties[facIdx].maxHours) {
            reason = "Faculty weekly workload capacity exceeded"; return false;
        }
    }
    for (int s = slot; s < slot + dur; s++) {
        if (targetGrid[divIdx][day][s].subIdx != -1 || targetGrid[divIdx][day][s].isBreak) {
            reason = "Division slot or scheduled break collision"; return false;
        }
        if (facIdx >= 0) {
            for (int d = 0; d < numDivs; d++) {
                if (targetGrid[d][day][s].facIdx == facIdx) {
                    reason = "Faculty already lecturing in another division"; return false;
                }
            }
            for (CommitNode* c = faculties[facIdx].commitments; c; c = c->next) {
                if (c->day == day && c->slot == s) {
                    reason = "Outside faculty home department commitment clash"; return false;
                }
            }
        }
    }
    assignedRoom = -1;
    for (int r = 0; r < numRooms; r++) {
        if (!rooms[r].isAvailable) continue;
        if (rooms[r].isLab != (needLab ? 1 : 0)) continue;
        bool free = true;
        for (int s = slot; s < slot + dur; s++) {
            for (int d = 0; d < numDivs; d++) {
                if (targetGrid[d][day][s].roomIdx == r) { free = false; break; }
            }
            if (!free) break;
        }
        if (free) { assignedRoom = r; break; }
    }
    if (assignedRoom == -1) {
        reason = needLab ? "No suitable laboratory facility available" : "No classroom available";
        return false;
    }
    return true;
}

inline void generateTimetable(const bool selectedDivs[MAX_DIVS]) {
    numUnsched = 0;
    int intM = atoi(dept.interval.c_str()); if (intM <= 0) intM = 60;

    Cell tempGrid[MAX_DIVS][MAX_DAYS][MAX_SLOTS];
    for (int d = 0; d < numDivs; d++) {
        for (int day = 0; day < dept.numDays; day++) {
            for (int s = 0; s < dept.numSlots; s++) {
                tempGrid[d][day][s] = selectedDivs[d] ? Cell() : grid[d][day][s];
            }
        }
    }

    // Recalculate faculty hours for unselected divisions
    for (int f = 0; f < numFac; f++) {
        faculties[f].currHours = 0;
        for (int d = 0; d < numDivs; d++) {
            if (selectedDivs[d]) continue;
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (tempGrid[d][day][s].facIdx == f) {
                        faculties[f].currHours += max(1, intM / 60);
                    }
                }
            }
        }
    }

    // Place fixed breaks first
    for (int b = 0; b < numBreaks; b++) {
        if (breaks[b].isFlexible) continue;
        for (int d = 0; d < numDivs; d++) {
            if (!selectedDivs[d]) continue;
            bool apply = (breaks[b].scopeType == 0) ||
                         (breaks[b].scopeType == 1 && divisions[d].yearId == breaks[b].scopeVal) ||
                         (breaks[b].scopeType == 2 && d == breaks[b].scopeVal);
            if (apply && breaks[b].day < dept.numDays && breaks[b].slot < dept.numSlots) {
                tempGrid[d][breaks[b].day][breaks[b].slot] = Cell(-1, -1, -1, 1, breaks[b].name);
            }
        }
    }

    // Build Heap of teaching sessions
    MaxHeap heap;
    for (int d = 0; d < numDivs; d++) {
        if (!selectedDivs[d]) continue;
        for (int s = 0; s < numSubs; s++) {
            bool applies = false;
            if (subjects[s].numAssignedDivs > 0) {
                for (int k = 0; k < subjects[s].numAssignedDivs; k++)
                    if (subjects[s].assignedDivs[k] == d) applies = true;
            } else if (subjects[s].yearId == divisions[d].yearId) applies = true;

            if (!applies) continue;

            int pFac = subjects[s].practicalFacIdx >= 0 ? subjects[s].practicalFacIdx : subjects[s].facIdx;
            int lFac = subjects[s].facIdx;
            bool pOut = (pFac >= 0 && faculties[pFac].isOutside);
            bool lOut = (lFac >= 0 && faculties[lFac].isOutside);

            int pracDurSlots = max(1, subjects[s].practicalDuration / intM);
            int lecDurSlots = max(1, subjects[s].lecDuration / intM);

            for (int p = 0; p < subjects[s].practicalCount; p++)
                heap.push(Session(s, d, pFac, 1, pracDurSlots, pOut ? 110 : 100));

            for (int l = 0; l < subjects[s].weeklyLec; l++)
                heap.push(Session(s, d, lFac, 0, lecDurSlots, lOut ? 55 : 30));
        }
    }

    // Greedy scheduling using Max-Heap
    while (!heap.empty()) {
        Session sess = heap.pop();
        bool scheduled = false; string lastFail = "No available slot matching constraints";
        int assignedRoom = -1;
        for (int day = 0; day < dept.numDays && !scheduled; day++) {
            for (int slot = 0; slot <= dept.numSlots - sess.duration && !scheduled; slot++) {
                if (checkSlotFreeInGrid(tempGrid, sess.divIdx, day, slot, sess.duration, sess.facIdx, sess.type == 1, assignedRoom, lastFail)) {
                    for (int dur = 0; dur < sess.duration; dur++) {
                        string lbl = subjects[sess.subIdx].name + (sess.type == 1 ? " [Lab]" : "");
                        tempGrid[sess.divIdx][day][slot + dur] = Cell(sess.subIdx, sess.facIdx, assignedRoom, 0, lbl);
                    }
                    if (sess.facIdx >= 0) {
                        int hoursToAdd = max(1, (sess.duration * intM) / 60);
                        faculties[sess.facIdx].currHours += hoursToAdd;
                    }
                    scheduled = true;
                }
            }
        }
        if (!scheduled && numUnsched < MAX_SESS) {
            string tag = sess.type == 1 ? " [Practical]" : " [Theory]";
            unscheduled[numUnsched++] = UnscheduledInfo{subjects[sess.subIdx].name + tag, divisions[sess.divIdx].name, lastFail};
        }
    }

    // Place flexible breaks in available windows
    for (int b = 0; b < numBreaks; b++) {
        if (!breaks[b].isFlexible) continue;
        for (int d = 0; d < numDivs; d++) {
            if (!selectedDivs[d]) continue;
            bool apply = (breaks[b].scopeType == 0) ||
                         (breaks[b].scopeType == 1 && divisions[d].yearId == breaks[b].scopeVal) ||
                         (breaks[b].scopeType == 2 && d == breaks[b].scopeVal);
            if (!apply) continue;

            int targetDay = breaks[b].day < dept.numDays ? breaks[b].day : 0;
            int startSlot = max(0, breaks[b].windowStartSlot);
            int endSlot = min(dept.numSlots - 1, breaks[b].windowEndSlot);
            bool placed = false;
            for (int s = startSlot; s <= endSlot && !placed; s++) {
                if (tempGrid[d][targetDay][s].subIdx == -1 && !tempGrid[d][targetDay][s].isBreak) {
                    tempGrid[d][targetDay][s] = Cell(-1, -1, -1, 1, breaks[b].name + " (Flex)");
                    placed = true;
                }
            }
        }
    }

    // Validation passed: commit temporary memory into primary grid
    for (int d = 0; d < numDivs; d++) {
        for (int day = 0; day < dept.numDays; day++) {
            for (int s = 0; s < dept.numSlots; s++) {
                grid[d][day][s] = tempGrid[d][day][s];
            }
        }
    }
    dept.lastGenerated = getCurrentTimestamp();
    buildFacultyGraph();
}

#endif
