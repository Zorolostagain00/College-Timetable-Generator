#ifndef API_H
#define API_H

#include <string>
#include <sstream>
#include <vector>
#include "models.h"
#include "dsa.h"
#include "scheduler.h"
#include "storage.h"

inline string getJsonStr(const string& j, const string& k) {
    size_t pos = j.find(k);
    if (pos == string::npos) return "";
    pos = j.find(':', pos); if (pos == string::npos) return "";
    pos = j.find('\"', pos); if (pos == string::npos) return "";
    size_t end = j.find('\"', pos + 1);
    return (end != string::npos) ? j.substr(pos + 1, end - pos - 1) : "";
}

inline int getJsonInt(const string& j, const string& k, int def = 0) {
    size_t pos = j.find(k);
    if (pos == string::npos) return def;
    pos = j.find(':', pos); if (pos == string::npos) return def;
    while (pos < j.size() && (j[pos] == ':' || j[pos] == ' ' || j[pos] == '\"')) pos++;
    size_t end = pos;
    while (end < j.size() && (isdigit(j[end]) || j[end] == '-')) end++;
    return (pos < end) ? atoi(j.substr(pos, end - pos).c_str()) : def;
}

inline string handleConfig(const string& method, const string& body) {
    if (method == "POST" || method == "PUT") {
        string name = getJsonStr(body, "name");
        string st = getJsonStr(body, "startTime");
        string et = getJsonStr(body, "endTime");
        string inter = getJsonStr(body, "interval");
        if (!name.empty()) dept.name = name;
        if (!st.empty()) dept.startTime = st;
        if (!et.empty()) dept.endTime = et;
        if (!inter.empty()) dept.interval = inter;

        // Custom working days parsing if provided
        size_t dPos = body.find("days");
        if (dPos != string::npos) {
            size_t bStart = body.find('[', dPos);
            size_t bEnd = body.find(']', bStart);
            if (bStart != string::npos && bEnd != string::npos) {
                string dArr = body.substr(bStart + 1, bEnd - (bStart + 1));
                stringstream ss(dArr); string dayItem; int cnt = 0;
                while (getline(ss, dayItem, ',') && cnt < MAX_DAYS) {
                    size_t q1 = dayItem.find('\"'), q2 = dayItem.rfind('\"');
                    if (q1 != string::npos && q2 != string::npos && q2 > q1) {
                        string val = dayItem.substr(q1 + 1, q2 - q1 - 1);
                        trimCr(val);
                        if (!val.empty()) dept.days[cnt++] = val;
                    }
                }
                if (cnt > 0) dept.numDays = cnt;
            }
        }

        dept.recalculateSlots();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Department configuration saved and slots recalculated\"}";
    }

    stringstream ss;
    ss << "{\"name\":\"" << dept.name << "\",\"startTime\":\"" << dept.startTime << "\",\"endTime\":\"" << dept.endTime
       << "\",\"interval\":\"" << dept.interval << "\",\"lastGenerated\":\"" << dept.lastGenerated
       << "\",\"numDays\":" << dept.numDays << ",\"numSlots\":" << dept.numSlots << ",\"days\":[";
    for (int i = 0; i < dept.numDays; i++) ss << (i ? ",\"" : "\"") << dept.days[i] << "\"";
    ss << "],\"times\":[";
    for (int i = 0; i < dept.numSlots; i++) ss << (i ? ",\"" : "\"") << dept.times[i] << "\"";
    ss << "],\"divisions\":[";
    for (int i = 0; i < numDivs; i++) ss << (i ? ",{\"" : "{\"") << "id\":" << i << ",\"name\":\"" << divisions[i].name << "\",\"year\":" << divisions[i].yearId << "}";
    ss << "]}";
    return ss.str();
}

inline string handleFaculty(const string& method, const string& body) {
    if (method == "POST") {
        string id = getJsonStr(body, "id"), name = getJsonStr(body, "name"), dep = getJsonStr(body, "dept");
        string qual = getJsonStr(body, "qualifications");
        int out = getJsonInt(body, "isOutside", 0), mh = getJsonInt(body, "maxHours", 16);
        if (!id.empty() && !name.empty() && numFac < MAX_FAC) {
            for (int i = 0; i < numFac; i++) {
                if (faculties[i].id == id) return "{\"status\":\"error\",\"message\":\"Faculty ID '" + id + "' is already registered.\"}";
            }
            faculties[numFac] = Faculty(id, name, dep, out == 1, mh, 0, qual);
            size_t dPos = body.find("days");
            if (dPos != string::npos) {
                size_t bStart = body.find('[', dPos), bEnd = body.find(']', bStart);
                if (bStart != string::npos && bEnd != string::npos) {
                    for (int d = 0; d < MAX_DAYS; d++) faculties[numFac].availDays[d] = false;
                    string arr = body.substr(bStart + 1, bEnd - (bStart + 1));
                    stringstream ss(arr); string dVal;
                    while (getline(ss, dVal, ',')) {
                        int dayNum = atoi(dVal.c_str());
                        if (dayNum >= 0 && dayNum < MAX_DAYS) faculties[numFac].availDays[dayNum] = true;
                    }
                }
            }
            numFac++;
            facHash = FacultyHash();
            for (int i = 0; i < numFac; i++) facHash.insert(faculties[i].id, i);
            buildFacultyGraph();
            saveData();
            return "{\"status\":\"success\",\"message\":\"Faculty member registered\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Unable to register faculty: Name and ID required\"}";
    }
    if (method == "PUT") {
        string origId = getJsonStr(body, "origId");
        string id = getJsonStr(body, "id");
        if (origId.empty()) origId = id;
        int idx = -1;
        for (int i = 0; i < numFac; i++) if (faculties[i].id == origId) { idx = i; break; }
        if (idx == -1 && !id.empty()) {
            for (int i = 0; i < numFac; i++) if (faculties[i].id == id) { idx = i; break; }
        }
        if (idx >= 0 && idx < numFac) {
            if (!id.empty() && id != faculties[idx].id) {
                for (int i = 0; i < numFac; i++) {
                    if (i != idx && faculties[i].id == id) {
                        return "{\"status\":\"error\",\"message\":\"Faculty ID '" + id + "' is already in use.\"}";
                    }
                }
                faculties[idx].id = id;
            }
            string name = getJsonStr(body, "name"), dep = getJsonStr(body, "dept"), qual = getJsonStr(body, "qualifications");
            int out = getJsonInt(body, "isOutside", faculties[idx].isOutside ? 1 : 0);
            int mh = getJsonInt(body, "maxHours", faculties[idx].maxHours);
            if (!name.empty()) faculties[idx].name = name;
            if (!dep.empty()) faculties[idx].dept = dep;
            faculties[idx].qualifications = qual;
            faculties[idx].isOutside = (out == 1);
            faculties[idx].maxHours = mh;

            size_t dPos = body.find("days");
            if (dPos != string::npos) {
                size_t bStart = body.find('[', dPos), bEnd = body.find(']', bStart);
                if (bStart != string::npos && bEnd != string::npos) {
                    for (int d = 0; d < MAX_DAYS; d++) faculties[idx].availDays[d] = false;
                    string arr = body.substr(bStart + 1, bEnd - (bStart + 1));
                    stringstream ss(arr); string dVal;
                    while (getline(ss, dVal, ',')) {
                        int dayNum = atoi(dVal.c_str());
                        if (dayNum >= 0 && dayNum < MAX_DAYS) faculties[idx].availDays[dayNum] = true;
                    }
                }
            }
            facHash = FacultyHash();
            for (int i = 0; i < numFac; i++) facHash.insert(faculties[i].id, i);
            buildFacultyGraph();
            saveData();
            return "{\"status\":\"success\",\"message\":\"Faculty updated successfully\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Faculty not found for update\"}";
    }
    if (method == "DELETE") {
        string id = getJsonStr(body, "id");
        int idx = -1;
        for (int i = 0; i < numFac; i++) if (faculties[i].id == id) { idx = i; break; }
        if (idx < 0 || idx >= numFac) return "{\"status\":\"error\",\"message\":\"Faculty not found\"}";

        // Dependency check 1: Assigned to subjects
        for (int s = 0; s < numSubs; s++) {
            if (subjects[s].facIdx == idx || subjects[s].practicalFacIdx == idx) {
                return "{\"status\":\"error\",\"message\":\"Cannot delete: Faculty is assigned to subject '" + subjects[s].name + "'. Reassign subject first.\"}";
            }
        }
        // Dependency check 2: Active sessions in timetable
        int sessCount = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].facIdx == idx) sessCount++;
                }
            }
        }
        if (sessCount > 0) {
            return "{\"status\":\"error\",\"message\":\"Cannot delete: Faculty has " + to_string(sessCount) + " active scheduled timetable sessions.\"}";
        }

        // Safe delete: shift array and update indices
        for (int i = idx; i < numFac - 1; i++) faculties[i] = faculties[i + 1];
        numFac--;
        for (int s = 0; s < numSubs; s++) {
            if (subjects[s].facIdx > idx) subjects[s].facIdx--;
            if (subjects[s].practicalFacIdx > idx) subjects[s].practicalFacIdx--;
        }
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].facIdx > idx) grid[d][day][s].facIdx--;
                }
            }
        }
        facHash = FacultyHash();
        for (int i = 0; i < numFac; i++) facHash.insert(faculties[i].id, i);
        buildFacultyGraph();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Faculty deleted successfully\"}";
    }

    stringstream ss; ss << "{\"faculty\":[";
    for (int i = 0; i < numFac; i++) {
        ss << (i ? ",{\"" : "{\"") << "id\":\"" << faculties[i].id << "\",\"name\":\"" << faculties[i].name
           << "\",\"dept\":\"" << faculties[i].dept << "\",\"qualifications\":\"" << faculties[i].qualifications
           << "\",\"isOutside\":" << (faculties[i].isOutside ? "true" : "false")
           << ",\"maxHours\":" << faculties[i].maxHours << ",\"currHours\":" << faculties[i].currHours
           << ",\"availDays\":[";
        for (int d = 0; d < dept.numDays; d++) ss << (d ? "," : "") << (faculties[i].availDays[d] ? "true" : "false");
        ss << "],\"commitments\":[";
        CommitNode* c = faculties[i].commitments; bool firstC = true;
        while (c) {
            if (!firstC) ss << ",";
            ss << "{\"day\":" << c->day << ",\"slot\":" << c->slot << "}";
            firstC = false; c = c->next;
        }
        ss << "]}";
    }
    ss << "]}"; return ss.str();
}

inline string handleSubjects(const string& method, const string& body) {
    if (method == "POST") {
        string id = getJsonStr(body, "id"), name = getJsonStr(body, "name"), rt = getJsonStr(body, "roomTypeReq");
        int yr = getJsonInt(body, "year", 1), fi = getJsonInt(body, "facIdx", 0);
        int pfi = getJsonInt(body, "practicalFacIdx", fi);
        int wl = getJsonInt(body, "weeklyLec", 3), ld = getJsonInt(body, "lecDuration", 60);
        int pc = getJsonInt(body, "practicalCount", 0), pd = getJsonInt(body, "practicalDuration", 120);

        if (!id.empty() && !name.empty() && numSubs < MAX_SUBS) {
            subjects[numSubs] = Subject(id, name, yr, fi, wl, ld, pc, pd, rt.empty() ? (pc > 0 ? "Laboratory" : "Classroom") : rt);
            subjects[numSubs].practicalFacIdx = pfi;
            numSubs++; saveData();
            return "{\"status\":\"success\",\"message\":\"Subject added\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Unable to add subject\"}";
    }
    if (method == "PUT") {
        string origId = getJsonStr(body, "origId");
        string id = getJsonStr(body, "id");
        if (origId.empty()) origId = id;
        int idx = -1;
        for (int i = 0; i < numSubs; i++) if (subjects[i].id == origId) { idx = i; break; }
        if (idx == -1 && !id.empty()) {
            for (int i = 0; i < numSubs; i++) if (subjects[i].id == id) { idx = i; break; }
        }
        if (idx >= 0 && idx < numSubs) {
            if (!id.empty() && id != subjects[idx].id) {
                for (int i = 0; i < numSubs; i++) {
                    if (i != idx && subjects[i].id == id) {
                        return "{\"status\":\"error\",\"message\":\"Subject ID '" + id + "' is already in use.\"}";
                    }
                }
                subjects[idx].id = id;
            }
            string name = getJsonStr(body, "name"), rt = getJsonStr(body, "roomTypeReq");
            int yr = getJsonInt(body, "year", subjects[idx].yearId);
            int fi = getJsonInt(body, "facIdx", subjects[idx].facIdx);
            int pfi = getJsonInt(body, "practicalFacIdx", subjects[idx].practicalFacIdx);
            int wl = getJsonInt(body, "weeklyLec", subjects[idx].weeklyLec);
            int ld = getJsonInt(body, "lecDuration", subjects[idx].lecDuration);
            int pc = getJsonInt(body, "practicalCount", subjects[idx].practicalCount);
            int pd = getJsonInt(body, "practicalDuration", subjects[idx].practicalDuration);

            if (!name.empty()) subjects[idx].name = name;
            subjects[idx].yearId = yr;
            subjects[idx].facIdx = fi;
            subjects[idx].practicalFacIdx = pfi;
            subjects[idx].weeklyLec = wl;
            subjects[idx].lecDuration = ld;
            subjects[idx].practicalCount = pc;
            subjects[idx].practicalDuration = pd;
            if (!rt.empty()) subjects[idx].roomTypeReq = rt;

            buildFacultyGraph();
            saveData();
            return "{\"status\":\"success\",\"message\":\"Subject updated successfully\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Subject not found for update\"}";
    }
    if (method == "DELETE") {
        string id = getJsonStr(body, "id");
        int idx = -1;
        for (int i = 0; i < numSubs; i++) if (subjects[i].id == id) { idx = i; break; }
        if (idx < 0) return "{\"status\":\"error\",\"message\":\"Subject not found\"}";

        int activeSess = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].subIdx == idx) activeSess++;
                }
            }
        }
        if (activeSess > 0) {
            return "{\"status\":\"error\",\"message\":\"Cannot delete: Subject exists in " + to_string(activeSess) + " scheduled sessions.\"}";
        }

        for (int i = idx; i < numSubs - 1; i++) subjects[i] = subjects[i + 1];
        numSubs--;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].subIdx > idx) grid[d][day][s].subIdx--;
                }
            }
        }
        saveData();
        return "{\"status\":\"success\",\"message\":\"Subject deleted\"}";
    }

    stringstream ss; ss << "{\"subjects\":[";
    for (int i = 0; i < numSubs; i++) {
        ss << (i ? ",{\"" : "{\"") << "id\":\"" << subjects[i].id << "\",\"name\":\"" << subjects[i].name
           << "\",\"year\":" << subjects[i].yearId << ",\"facIdx\":" << subjects[i].facIdx
           << ",\"facName\":\"" << (subjects[i].facIdx >= 0 && subjects[i].facIdx < numFac ? faculties[subjects[i].facIdx].name : "Unassigned")
           << "\",\"practicalFacIdx\":" << subjects[i].practicalFacIdx
           << ",\"practicalFacName\":\"" << (subjects[i].practicalFacIdx >= 0 && subjects[i].practicalFacIdx < numFac ? faculties[subjects[i].practicalFacIdx].name : "Unassigned")
           << "\",\"weeklyLec\":" << subjects[i].weeklyLec << ",\"lecDuration\":" << subjects[i].lecDuration
           << ",\"practicalCount\":" << subjects[i].practicalCount << ",\"practicalDuration\":" << subjects[i].practicalDuration
           << ",\"roomTypeReq\":\"" << subjects[i].roomTypeReq << "\"}";
    }
    ss << "]}"; return ss.str();
}

inline string handleDivisions(const string& method, const string& body) {
    if (method == "POST") {
        string name = getJsonStr(body, "name");
        int yr = getJsonInt(body, "year", 1);
        if (!name.empty() && numDivs < MAX_DIVS) {
            divisions[numDivs++] = Division(name, yr);
            saveData();
            return "{\"status\":\"success\",\"message\":\"Academic division added\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Unable to add division\"}";
    }
    if (method == "PUT") {
        int id = getJsonInt(body, "id", -1);
        string name = getJsonStr(body, "name");
        int yr = getJsonInt(body, "year", 0);
        if (id >= 0 && id < numDivs) {
            if (!name.empty()) divisions[id].name = name;
            if (yr > 0) divisions[id].yearId = yr;
            saveData();
            return "{\"status\":\"success\",\"message\":\"Division updated\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Division not found\"}";
    }
    if (method == "DELETE") {
        int id = getJsonInt(body, "id", -1);
        if (id >= 0 && id < numDivs) {
            for (int i = id; i < numDivs - 1; i++) {
                divisions[i] = divisions[i + 1];
                for (int day = 0; day < dept.numDays; day++) {
                    for (int s = 0; s < dept.numSlots; s++) grid[i][day][s] = grid[i + 1][day][s];
                }
            }
            numDivs--;
            saveData();
            return "{\"status\":\"success\",\"message\":\"Division deleted\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Division not found\"}";
    }
    return "{\"status\":\"error\",\"message\":\"Unsupported method\"}";
}

inline string handleRooms(const string& method, const string& body) {
    if (method == "POST") {
        string id = getJsonStr(body, "id"), name = getJsonStr(body, "name"), typ = getJsonStr(body, "type"), eq = getJsonStr(body, "equipment");
        int cap = getJsonInt(body, "capacity", 60), isL = getJsonInt(body, "isLab", 0);
        if (!id.empty() && numRooms < MAX_ROOMS) {
            rooms[numRooms++] = Room(id, name, typ.empty() ? (isL ? "Laboratory" : "Classroom") : typ, cap, isL, eq.empty() ? "Standard" : eq, true);
            saveData(); return "{\"status\":\"success\",\"message\":\"Room added\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Failed to add room\"}";
    }
    if (method == "PUT") {
        string origId = getJsonStr(body, "origId");
        string id = getJsonStr(body, "id");
        if (origId.empty()) origId = id;
        int idx = -1;
        for (int i = 0; i < numRooms; i++) if (rooms[i].id == origId) { idx = i; break; }
        if (idx == -1 && !id.empty()) {
            for (int i = 0; i < numRooms; i++) if (rooms[i].id == id) { idx = i; break; }
        }
        if (idx >= 0 && idx < numRooms) {
            if (!id.empty() && id != rooms[idx].id) {
                for (int i = 0; i < numRooms; i++) {
                    if (i != idx && rooms[i].id == id) {
                        return "{\"status\":\"error\",\"message\":\"Facility ID '" + id + "' is already in use.\"}";
                    }
                }
                rooms[idx].id = id;
            }
            string name = getJsonStr(body, "name"), typ = getJsonStr(body, "type"), eq = getJsonStr(body, "equipment");
            int cap = getJsonInt(body, "capacity", rooms[idx].capacity);
            int isL = getJsonInt(body, "isLab", rooms[idx].isLab);
            int isAv = getJsonInt(body, "isAvailable", rooms[idx].isAvailable ? 1 : 0);
            if (!name.empty()) rooms[idx].name = name;
            if (!typ.empty()) rooms[idx].type = typ;
            if (!eq.empty()) rooms[idx].equipment = eq;
            rooms[idx].capacity = cap; rooms[idx].isLab = isL; rooms[idx].isAvailable = (isAv == 1);
            saveData();
            return "{\"status\":\"success\",\"message\":\"Facility updated successfully\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Room not found for update\"}";
    }
    if (method == "DELETE") {
        string id = getJsonStr(body, "id");
        int idx = -1;
        for (int i = 0; i < numRooms; i++) if (rooms[i].id == id) { idx = i; break; }
        if (idx < 0) return "{\"status\":\"error\",\"message\":\"Room not found\"}";

        int usedSess = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].roomIdx == idx) usedSess++;
                }
            }
        }
        if (usedSess > 0) {
            return "{\"status\":\"error\",\"message\":\"Cannot delete: Room is allocated in " + to_string(usedSess) + " active timetable sessions.\"}";
        }

        for (int i = idx; i < numRooms - 1; i++) rooms[i] = rooms[i + 1];
        numRooms--;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].roomIdx > idx) grid[d][day][s].roomIdx--;
                }
            }
        }
        saveData();
        return "{\"status\":\"success\",\"message\":\"Room deleted\"}";
    }

    stringstream ss; ss << "{\"rooms\":[";
    for (int i = 0; i < numRooms; i++) {
        ss << (i ? ",{\"" : "{\"") << "id\":\"" << rooms[i].id << "\",\"name\":\"" << rooms[i].name
           << "\",\"type\":\"" << rooms[i].type << "\",\"equipment\":\"" << rooms[i].equipment
           << "\",\"capacity\":" << rooms[i].capacity << ",\"isLab\":" << (rooms[i].isLab ? "true" : "false")
           << ",\"isAvailable\":" << (rooms[i].isAvailable ? "true" : "false") << "}";
    }
    ss << "]}"; return ss.str();
}

inline string handleBreaks(const string& method, const string& body) {
    if (method == "POST") {
        string name = getJsonStr(body, "name");
        int day = getJsonInt(body, "day", 0), slot = getJsonInt(body, "slot", 3);
        int st = getJsonInt(body, "scopeType", 0), sv = getJsonInt(body, "scopeVal", 0);
        int flex = getJsonInt(body, "isFlexible", 0), dur = getJsonInt(body, "durationSlots", 1);
        int ws = getJsonInt(body, "windowStartSlot", 0), we = getJsonInt(body, "windowEndSlot", 5);

        if (numBreaks < MAX_BREAKS) {
            breaks[numBreaks++] = Break(name.empty() ? "Break" : name, day, slot, st, sv, flex == 1, dur, ws, we);
            saveData(); return "{\"status\":\"success\",\"message\":\"Break scheduled\"}";
        }
    }
    if (method == "DELETE") {
        int idx = getJsonInt(body, "index", -1);
        if (idx >= 0 && idx < numBreaks) {
            for (int i = idx; i < numBreaks - 1; i++) breaks[i] = breaks[i + 1];
            numBreaks--; saveData();
            return "{\"status\":\"success\",\"message\":\"Break removed\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Break not found\"}";
    }

    stringstream ss; ss << "{\"breaks\":[";
    for (int i = 0; i < numBreaks; i++) {
        ss << (i ? ",{\"" : "{\"") << "name\":\"" << breaks[i].name << "\",\"day\":" << breaks[i].day
           << ",\"slot\":" << breaks[i].slot << ",\"scopeType\":" << breaks[i].scopeType
           << ",\"scopeVal\":" << breaks[i].scopeVal << ",\"isFlexible\":" << (breaks[i].isFlexible ? "true" : "false")
           << ",\"durationSlots\":" << breaks[i].durationSlots << ",\"windowStartSlot\":" << breaks[i].windowStartSlot
           << ",\"windowEndSlot\":" << breaks[i].windowEndSlot << "}";
    }
    ss << "]}"; return ss.str();
}

inline void recalculateFacultyWorkload() {
    int intM = atoi(dept.interval.c_str()); if (intM <= 0) intM = 60;
    for (int f = 0; f < numFac; f++) {
        faculties[f].currHours = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].facIdx == f) faculties[f].currHours += max(1, intM / 60);
                }
            }
        }
    }
}

inline string handleSessionEdit(const string& body) {
    string action = getJsonStr(body, "action");
    int divIdx = getJsonInt(body, "divIdx", 0);
    int day = getJsonInt(body, "day", 0);
    int slot = getJsonInt(body, "slot", 0);

    if (divIdx < 0 || divIdx >= numDivs || day < 0 || day >= dept.numDays || slot < 0 || slot >= dept.numSlots)
        return "{\"status\":\"error\",\"message\":\"Invalid coordinates\"}";

    if (action == "delete") {
        Cell prev = grid[divIdx][day][slot];
        undoStack.push(TimetableEntry{divIdx, day, slot, prev.subIdx, prev.facIdx, prev.roomIdx, prev.isBreak, prev.isManual, prev.label});
        grid[divIdx][day][slot] = Cell();
        recalculateFacultyWorkload();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Session removed from slot\"}";
    }
    if (action == "edit") {
        int facIdx = getJsonInt(body, "facIdx", grid[divIdx][day][slot].facIdx);
        int roomIdx = getJsonInt(body, "roomIdx", grid[divIdx][day][slot].roomIdx);
        Cell prev = grid[divIdx][day][slot];
        undoStack.push(TimetableEntry{divIdx, day, slot, prev.subIdx, prev.facIdx, prev.roomIdx, prev.isBreak, prev.isManual, prev.label});
        grid[divIdx][day][slot].facIdx = facIdx;
        grid[divIdx][day][slot].roomIdx = roomIdx;
        grid[divIdx][day][slot].isManual = 1;
        recalculateFacultyWorkload();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Session updated successfully\"}";
    }
    if (action == "reschedule") {
        int newDay = getJsonInt(body, "newDay", 0);
        int newSlot = getJsonInt(body, "newSlot", 0);
        if (newDay < 0 || newDay >= dept.numDays || newSlot < 0 || newSlot >= dept.numSlots)
            return "{\"status\":\"error\",\"message\":\"Invalid target slot\"}";

        if (grid[divIdx][newDay][newSlot].subIdx != -1 || grid[divIdx][newDay][newSlot].isBreak)
            return "{\"status\":\"error\",\"message\":\"Target slot is already occupied or blocked by a break\"}";

        Cell current = grid[divIdx][day][slot];
        undoStack.push(TimetableEntry{divIdx, day, slot, current.subIdx, current.facIdx, current.roomIdx, current.isBreak, current.isManual, current.label});
        undoStack.push(TimetableEntry{divIdx, newDay, newSlot, -1, -1, -1, 0, 0, ""});

        grid[divIdx][newDay][newSlot] = current;
        grid[divIdx][newDay][newSlot].isManual = 1;
        grid[divIdx][day][slot] = Cell();
        recalculateFacultyWorkload();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Session rescheduled successfully\"}";
    }
    if (action == "undo") {
        if (undoStack.empty()) return "{\"status\":\"error\",\"message\":\"No manual modifications to undo\"}";
        TimetableEntry e = undoStack.pop();
        if (e.divIdx >= 0 && e.divIdx < numDivs && e.day >= 0 && e.day < dept.numDays && e.slot >= 0 && e.slot < dept.numSlots) {
            grid[e.divIdx][e.day][e.slot] = Cell(e.subIdx, e.facIdx, e.roomIdx, e.isBreak, e.label, e.isManual);
            recalculateFacultyWorkload();
            saveData();
            return "{\"status\":\"success\",\"message\":\"Undo operation applied successfully\"}";
        }
        return "{\"status\":\"error\",\"message\":\"Unable to undo modification\"}";
    }
    return "{\"status\":\"error\",\"message\":\"Unknown action\"}";
}

inline string handleResetTimetable(const string& body) {
    int divIdx = getJsonInt(body, "division", -1);
    if (divIdx >= 0 && divIdx < numDivs) {
        for (int day = 0; day < dept.numDays; day++) {
            for (int s = 0; s < dept.numSlots; s++) grid[divIdx][day][s] = Cell();
        }
        for (int b = 0; b < numBreaks; b++) {
            if (breaks[b].isFlexible) continue;
            bool apply = (breaks[b].scopeType == 0) ||
                         (breaks[b].scopeType == 1 && divisions[divIdx].yearId == breaks[b].scopeVal) ||
                         (breaks[b].scopeType == 2 && divIdx == breaks[b].scopeVal);
            if (apply && breaks[b].day < dept.numDays && breaks[b].slot < dept.numSlots) {
                grid[divIdx][breaks[b].day][breaks[b].slot] = Cell(-1, -1, -1, 1, breaks[b].name);
            }
        }
    } else {
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) grid[d][day][s] = Cell();
            }
            for (int b = 0; b < numBreaks; b++) {
                if (breaks[b].isFlexible) continue;
                bool apply = (breaks[b].scopeType == 0) ||
                             (breaks[b].scopeType == 1 && divisions[d].yearId == breaks[b].scopeVal) ||
                             (breaks[b].scopeType == 2 && d == breaks[b].scopeVal);
                if (apply && breaks[b].day < dept.numDays && breaks[b].slot < dept.numSlots) {
                    grid[d][breaks[b].day][breaks[b].slot] = Cell(-1, -1, -1, 1, breaks[b].name);
                }
            }
        }
    }

    int intM = atoi(dept.interval.c_str()); if (intM <= 0) intM = 60;
    for (int f = 0; f < numFac; f++) {
        faculties[f].currHours = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int day = 0; day < dept.numDays; day++) {
                for (int s = 0; s < dept.numSlots; s++) {
                    if (grid[d][day][s].facIdx == f) faculties[f].currHours += max(1, intM / 60);
                }
            }
        }
    }

    dept.lastGenerated = "Timetable Reset (" + getCurrentTimestamp() + ")";
    buildFacultyGraph();
    saveData();
    return "{\"status\":\"success\",\"message\":\"Timetable reset successfully\",\"lastGenerated\":\"" + dept.lastGenerated + "\"}";
}

inline string handleGenerate(const string& body) {
    bool selected[MAX_DIVS] = {false};
    bool selectAll = (body.find("\"divisions\"") == string::npos);
    for (int i = 0; i < numDivs; i++) {
        stringstream sid; sid << i;
        if (selectAll || body.find(sid.str()) != string::npos) selected[i] = true;
    }
    generateTimetable(selected);
    saveData();
    int scheduledCount = 0;
    for (int d = 0; d < numDivs; d++) {
        for (int day = 0; day < dept.numDays; day++) {
            for (int s = 0; s < dept.numSlots; s++) {
                if (grid[d][day][s].subIdx != -1) scheduledCount++;
            }
        }
    }
    stringstream ss;
    ss << "{\"status\":\"success\",\"scheduledCount\":" << scheduledCount
       << ",\"unscheduledCount\":" << numUnsched << ",\"lastGenerated\":\"" << dept.lastGenerated
       << "\",\"unscheduled\":[";
    for (int i = 0; i < numUnsched; i++) {
        ss << (i ? ",{\"" : "{\"") << "subject\":\"" << unscheduled[i].subName
           << "\",\"division\":\"" << unscheduled[i].divName
           << "\",\"reason\":\"" << unscheduled[i].reason << "\"}";
    }
    ss << "]}";
    return ss.str();
}

inline string handleTimetable(int divIdx) {
    if (divIdx < 0 || divIdx >= numDivs) divIdx = 0;
    stringstream ss;
    ss << "{\"division\":{\"id\":" << divIdx << ",\"name\":\"" << divisions[divIdx].name << "\",\"year\":" << divisions[divIdx].yearId
       << "},\"lastGenerated\":\"" << dept.lastGenerated << "\",\"grid\":[";
    for (int day = 0; day < dept.numDays; day++) {
        ss << (day ? ",[" : "[");
        for (int s = 0; s < dept.numSlots; s++) {
            Cell& c = grid[divIdx][day][s];
            ss << (s ? ",{\"" : "{\"") << "subIdx\":" << c.subIdx
               << ",\"subject\":\"" << (c.subIdx >= 0 && c.subIdx < numSubs ? subjects[c.subIdx].name : "")
               << "\",\"facIdx\":" << c.facIdx
               << ",\"faculty\":\"" << (c.facIdx >= 0 && c.facIdx < numFac ? faculties[c.facIdx].name : "")
               << "\",\"roomIdx\":" << c.roomIdx
               << ",\"room\":\"" << (c.roomIdx >= 0 && c.roomIdx < numRooms ? rooms[c.roomIdx].name : "")
               << "\",\"isBreak\":" << (c.isBreak ? "true" : "false")
               << ",\"isManual\":" << (c.isManual ? "true" : "false")
               << ",\"label\":\"" << c.label << "\"}";
        }
        ss << "]";
    }
    ss << "]}";
    return ss.str();
}

inline string handleReports() {
    stringstream ss;
    int intM = atoi(dept.interval.c_str()); if (intM <= 0) intM = 60;

    ss << "{\"facultyWorkload\":[";
    for (int i = 0; i < numFac; i++) {
        int slotsUsed = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int dy = 0; dy < dept.numDays; dy++) {
                for (int s = 0; s < dept.numSlots; s++) if (grid[d][dy][s].facIdx == i) slotsUsed++;
            }
        }
        int hours = max(slotsUsed > 0 ? 1 : 0, (slotsUsed * intM) / 60);
        ss << (i ? ",{\"" : "{\"") << "name\":\"" << faculties[i].name << "\",\"dept\":\"" << faculties[i].dept
           << "\",\"current\":" << hours << ",\"max\":" << faculties[i].maxHours
           << ",\"percent\":" << (faculties[i].maxHours > 0 ? (hours * 100 / faculties[i].maxHours) : 0) << "}";
    }
    ss << "],\"roomUtilization\":[";
    for (int i = 0; i < numRooms; i++) {
        int used = 0;
        for (int d = 0; d < numDivs; d++) {
            for (int dy = 0; dy < dept.numDays; dy++) {
                for (int s = 0; s < dept.numSlots; s++) if (grid[d][dy][s].roomIdx == i) used++;
            }
        }
        int total = dept.numDays * dept.numSlots;
        ss << (i ? ",{\"" : "{\"") << "name\":\"" << rooms[i].name << "\",\"type\":\"" << rooms[i].type
           << "\",\"equipment\":\"" << rooms[i].equipment << "\",\"capacity\":" << rooms[i].capacity
           << ",\"used\":" << used << ",\"total\":" << total
           << ",\"percent\":" << (total > 0 ? (used * 100 / total) : 0) << "}";
    }
    ss << "],\"cascadeAnalysis\":[";
    for (int i = 0; i < numDivs; i++) {
        int order[MAX_DIVS], count = 0; bool directFlags[MAX_DIVS];
        divGraph.bfs(i, numDivs, order, count, directFlags);
        ss << (i ? ",{\"" : "{\"") << "division\":\"" << divisions[i].name
           << "\",\"connectedCount\":" << count << ",\"direct\":[";
        bool firstD = true;
        for (int c = 0; c < count; c++) {
            if (directFlags[c]) {
                if (!firstD) ss << ",";
                ss << "\"" << divisions[order[c]].name << "\"";
                firstD = false;
            }
        }
        ss << "],\"indirect\":[";
        bool firstI = true;
        for (int c = 0; c < count; c++) {
            if (!directFlags[c]) {
                if (!firstI) ss << ",";
                ss << "\"" << divisions[order[c]].name << "\"";
                firstI = false;
            }
        }
        ss << "]}";
    }
    ss << "]}";
    return ss.str();
}

inline string handleReset(const string& body) {
    string target = getJsonStr(body, "target");
    if (target == "sample" || target == "seed") {
        initSampleData();
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++) grid[d][day][s] = Cell();
        for (int b = 0; b < numBreaks; b++) {
            if (breaks[b].day < dept.numDays && breaks[b].slot < dept.numSlots)
                for (int d = 0; d < numDivs; d++) grid[d][breaks[b].day][breaks[b].slot] = Cell(-1, -1, -1, 1, breaks[b].name);
        }
        dept.lastGenerated = "Sample Data Seeded (" + getCurrentTimestamp() + ")";
        buildFacultyGraph();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Sample test data seeded.\"}";
    }
    if (target == "all" || target.empty()) {
        numFac = 0;
        facHash = FacultyHash();
        numSubs = 0;
        numRooms = 0;
        numBreaks = 0;
        numDivs = 0;
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++) grid[d][day][s] = Cell();
        dept.lastGenerated = "All Data Cleared (" + getCurrentTimestamp() + ")";
        buildFacultyGraph();
        saveData();
        return "{\"status\":\"success\",\"message\":\"All system data cleared. All faculty, subjects, rooms, breaks, and divisions removed. No defaults remaining.\"}";
    }
    if (target == "faculty") {
        facHash = FacultyHash();
        numFac = 0;
        for (int i = 0; i < numSubs; i++) {
            subjects[i].facIdx = -1;
            subjects[i].practicalFacIdx = -1;
        }
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++)
                    if (grid[d][day][s].facIdx != -1) grid[d][day][s] = Cell();
        buildFacultyGraph();
        saveData();
        return "{\"status\":\"success\",\"message\":\"All faculty members removed. You can now add faculty from scratch.\"}";
    }
    if (target == "subjects") {
        numSubs = 0;
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++)
                    if (grid[d][day][s].subIdx != -1) grid[d][day][s] = Cell();
        buildFacultyGraph();
        saveData();
        return "{\"status\":\"success\",\"message\":\"All curriculum subjects removed. You can now add subjects from scratch.\"}";
    }
    if (target == "rooms") {
        numRooms = 0;
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++)
                    if (grid[d][day][s].roomIdx != -1) grid[d][day][s].roomIdx = -1;
        saveData();
        return "{\"status\":\"success\",\"message\":\"All classrooms and laboratories removed. You can now add facilities from scratch.\"}";
    }
    if (target == "breaks") {
        numBreaks = 0;
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++)
                    if (grid[d][day][s].isBreak) grid[d][day][s] = Cell();
        saveData();
        return "{\"status\":\"success\",\"message\":\"All break schedules removed.\"}";
    }
    if (target == "department") {
        numDivs = 0;
        dept.recalculateSlots();
        for (int d = 0; d < MAX_DIVS; d++)
            for (int day = 0; day < MAX_DAYS; day++)
                for (int s = 0; s < MAX_SLOTS; s++) grid[d][day][s] = Cell();
        saveData();
        return "{\"status\":\"success\",\"message\":\"Department divisions and timetable reset. You can now configure divisions from scratch.\"}";
    }
    return "{\"status\":\"error\",\"message\":\"Unknown reset target specified.\"}";
}

#endif
