#ifndef STORAGE_H
#define STORAGE_H

#include <fstream>
#include <sstream>
#include "models.h"
#include "dsa.h"

inline void trimCr(string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\n'))
        s.pop_back();
}

inline void initDefaultData() {
    dept = Department();
    numDivs = 0;
    numFac = 0;
    facHash = FacultyHash();
    numRooms = 0;
    numSubs = 0;
    numBreaks = 0;
    for (int d = 0; d < MAX_DIVS; d++)
        for (int day = 0; day < MAX_DAYS; day++)
            for (int s = 0; s < MAX_SLOTS; s++) grid[d][day][s] = Cell();
}

inline void saveData() {
    ofstream fd("data/department.txt");
    if (fd) {
        fd << dept.name << "\n" << dept.startTime << "\n" << dept.endTime << "\n" << dept.interval << "\n"
           << dept.lastGenerated << "\n" << dept.numDays << "\n" << dept.numSlots << "\n";
        for (int i = 0; i < dept.numDays; i++) fd << dept.days[i] << (i + 1 < dept.numDays ? "," : "");
        fd << "\n";
        for (int i = 0; i < dept.numSlots; i++) fd << dept.times[i] << (i + 1 < dept.numSlots ? "," : "");
        fd << "\n";
        fd << numDivs << "\n";
        for (int i = 0; i < numDivs; i++) fd << divisions[i].name << "|" << divisions[i].yearId << "\n";
    }

    ofstream ff("data/faculty.txt");
    if (ff) {
        ff << numFac << "\n";
        for (int i = 0; i < numFac; i++) {
            ff << faculties[i].id << "|" << faculties[i].name << "|" << faculties[i].dept << "|"
               << (faculties[i].isOutside ? 1 : 0) << "|" << faculties[i].maxHours << "|"
               << faculties[i].qualifications << "|";
            for (int d = 0; d < MAX_DAYS; d++) ff << (faculties[i].availDays[d] ? '1' : '0');
            ff << "|";
            for (CommitNode* c = faculties[i].commitments; c; c = c->next) ff << c->day << ":" << c->slot << ";";
            ff << "\n";
        }
    }

    ofstream fs("data/subjects.txt");
    if (fs) {
        fs << numSubs << "\n";
        for (int i = 0; i < numSubs; i++) {
            fs << subjects[i].id << "|" << subjects[i].name << "|" << subjects[i].yearId << "|"
               << subjects[i].facIdx << "|" << subjects[i].practicalFacIdx << "|"
               << subjects[i].weeklyLec << "|" << subjects[i].lecDuration << "|"
               << subjects[i].practicalCount << "|" << subjects[i].practicalDuration << "|"
               << subjects[i].roomTypeReq << "|" << subjects[i].numAssignedDivs << "|";
            for (int k = 0; k < subjects[i].numAssignedDivs; k++) fs << subjects[i].assignedDivs[k] << ",";
            fs << "\n";
        }
    }

    ofstream fr("data/rooms.txt");
    if (fr) {
        fr << numRooms << "\n";
        for (int i = 0; i < numRooms; i++) {
            fr << rooms[i].id << "|" << rooms[i].name << "|" << rooms[i].type << "|"
               << rooms[i].equipment << "|" << rooms[i].capacity << "|" << rooms[i].isLab << "|"
               << (rooms[i].isAvailable ? 1 : 0) << "\n";
        }
    }

    ofstream fb("data/breaks.txt");
    if (fb) {
        fb << numBreaks << "\n";
        for (int i = 0; i < numBreaks; i++) {
            fb << breaks[i].name << "|" << breaks[i].day << "|" << breaks[i].slot << "|"
               << breaks[i].scopeType << "|" << breaks[i].scopeVal << "|"
               << (breaks[i].isFlexible ? 1 : 0) << "|" << breaks[i].durationSlots << "|"
               << breaks[i].windowStartSlot << "|" << breaks[i].windowEndSlot << "\n";
        }
    }

    ofstream ft("data/timetables.txt");
    if (ft) {
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    ft << d << "|" << day << "|" << s << "|" << grid[d][day][s].subIdx << "|"
                       << grid[d][day][s].facIdx << "|" << grid[d][day][s].roomIdx << "|"
                       << grid[d][day][s].isBreak << "|" << grid[d][day][s].isManual << "|"
                       << grid[d][day][s].label << "\n";
                }
            }
        }
    }
}

inline void loadData() {
    ifstream fd("data/department.txt");
    if (!fd) { initDefaultData(); saveData(); return; }

    string line;
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.name = line; }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.startTime = line; }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.endTime = line; }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.interval = line; }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.lastGenerated = line; }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.numDays = atoi(line.c_str()); }
    if (getline(fd, line)) { trimCr(line); if (!line.empty()) dept.numSlots = atoi(line.c_str()); }

    if (getline(fd, line)) {
        trimCr(line);
        stringstream ss(line); string t; int idx = 0;
        while (getline(ss, t, ',') && idx < MAX_DAYS) {
            trimCr(t);
            if (!t.empty()) dept.days[idx++] = t;
        }
        if (idx > 0) dept.numDays = idx;
    }

    if (getline(fd, line)) {
        trimCr(line);
        stringstream ss(line); string t; int idx = 0;
        while (getline(ss, t, ',') && idx < MAX_SLOTS) {
            trimCr(t);
            if (!t.empty()) dept.times[idx++] = t;
        }
    }
    dept.recalculateSlots();

    if (getline(fd, line)) {
        trimCr(line);
        numDivs = atoi(line.c_str());
        for (int i = 0; i < numDivs && getline(fd, line); i++) {
            trimCr(line);
            stringstream sdiv(line); getline(sdiv, divisions[i].name, '|');
            string y; getline(sdiv, y); divisions[i].yearId = atoi(y.c_str());
        }
    }

    ifstream ff("data/faculty.txt");
    if (ff) {
        ff >> numFac; string dummy; getline(ff, dummy);
        for (int i = 0; i < numFac && getline(ff, line); i++) {
            trimCr(line);
            stringstream sf(line); string id, name, dep, out, mh, qual, daysStr, comms;
            getline(sf, id, '|'); getline(sf, name, '|'); getline(sf, dep, '|');
            getline(sf, out, '|'); getline(sf, mh, '|'); getline(sf, qual, '|');
            getline(sf, daysStr, '|'); getline(sf, comms, '|');

            faculties[i] = Faculty(id, name, dep, out == "1", atoi(mh.c_str()), 0, qual);
            for (int k = 0; k < MAX_DAYS && k < (int)daysStr.size(); k++)
                faculties[i].availDays[k] = (daysStr[k] == '1');

            facHash.insert(id, i);
            stringstream sc(comms); string cm;
            while (getline(sc, cm, ';')) {
                size_t col = cm.find(':');
                if (col != string::npos) {
                    faculties[i].commitments = new CommitNode(atoi(cm.substr(0, col).c_str()),
                                                              atoi(cm.substr(col + 1).c_str()),
                                                              faculties[i].commitments);
                }
            }
        }
    }

    ifstream fs("data/subjects.txt");
    if (fs) {
        fs >> numSubs; string dummy; getline(fs, dummy);
        for (int i = 0; i < numSubs && getline(fs, line); i++) {
            trimCr(line);
            stringstream ssub(line); string id, name, yr, fi, pfi, wl, ld, pc, pd, rtr, adCnt, ads;
            getline(ssub, id, '|'); getline(ssub, name, '|'); getline(ssub, yr, '|');
            getline(ssub, fi, '|'); getline(ssub, pfi, '|');
            getline(ssub, wl, '|'); getline(ssub, ld, '|');
            getline(ssub, pc, '|'); getline(ssub, pd, '|');
            getline(ssub, rtr, '|'); getline(ssub, adCnt, '|'); getline(ssub, ads, '|');

            int yrI = atoi(yr.c_str()), fiI = atoi(fi.c_str());
            int pfiI = pfi.empty() ? fiI : atoi(pfi.c_str());
            int wlI = wl.empty() ? 3 : atoi(wl.c_str());
            int ldI = ld.empty() ? 60 : atoi(ld.c_str());
            int pcI = pc.empty() ? 0 : atoi(pc.c_str());
            int pdI = pd.empty() ? 120 : atoi(pd.c_str());
            string rt = rtr.empty() ? (pcI > 0 ? "Laboratory" : "Classroom") : rtr;

            subjects[i] = Subject(id, name, yrI, fiI, wlI, ldI, pcI, pdI, rt);
            subjects[i].practicalFacIdx = pfiI;
            subjects[i].numAssignedDivs = atoi(adCnt.c_str());
            stringstream sad(ads); string ad; int idx = 0;
            while (getline(sad, ad, ',') && idx < MAX_DIVS) {
                if (!ad.empty()) subjects[i].assignedDivs[idx++] = atoi(ad.c_str());
            }
        }
    }

    ifstream fr("data/rooms.txt");
    if (fr) {
        fr >> numRooms; string dummy; getline(fr, dummy);
        for (int i = 0; i < numRooms && getline(fr, line); i++) {
            trimCr(line);
            stringstream srm(line); string id, name, type, eq, cap, isL, isAv;
            getline(srm, id, '|'); getline(srm, name, '|'); getline(srm, type, '|');
            getline(srm, eq, '|'); getline(srm, cap, '|'); getline(srm, isL, '|'); getline(srm, isAv, '|');
            rooms[i] = Room(id, name, type, atoi(cap.c_str()), atoi(isL.c_str()), eq.empty() ? "Standard" : eq, isAv != "0");
        }
    }

    ifstream fb("data/breaks.txt");
    if (fb) {
        fb >> numBreaks; string dummy; getline(fb, dummy);
        for (int i = 0; i < numBreaks && getline(fb, line); i++) {
            trimCr(line);
            stringstream sbr(line); string name, day, slot, st, sv, flex, dur, ws, we;
            getline(sbr, name, '|'); getline(sbr, day, '|'); getline(sbr, slot, '|');
            getline(sbr, st, '|'); getline(sbr, sv, '|');
            getline(sbr, flex, '|'); getline(sbr, dur, '|'); getline(sbr, ws, '|'); getline(sbr, we, '|');
            breaks[i] = Break(name, atoi(day.c_str()), atoi(slot.c_str()), atoi(st.c_str()), atoi(sv.c_str()),
                              flex == "1", dur.empty() ? 1 : atoi(dur.c_str()),
                              ws.empty() ? 0 : atoi(ws.c_str()), we.empty() ? 5 : atoi(we.c_str()));
        }
    }

    ifstream ft("data/timetables.txt");
    if (ft) {
        string d, dy, s, sub, fac, rm, brk, man, lbl;
        while (getline(ft, line)) {
            trimCr(line);
            stringstream stt(line);
            getline(stt, d, '|'); getline(stt, dy, '|'); getline(stt, s, '|');
            getline(stt, sub, '|'); getline(stt, fac, '|'); getline(stt, rm, '|');
            getline(stt, brk, '|'); getline(stt, man, '|'); getline(stt, lbl, '|');
            int di = atoi(d.c_str()), dyi = atoi(dy.c_str()), si = atoi(s.c_str());
            if (di < MAX_DIVS && dyi < MAX_DAYS && si < MAX_SLOTS) {
                grid[di][dyi][si] = Cell(atoi(sub.c_str()), atoi(fac.c_str()), atoi(rm.c_str()),
                                         atoi(brk.c_str()), lbl, atoi(man.c_str()));
            }
        }
    }
}

#endif
