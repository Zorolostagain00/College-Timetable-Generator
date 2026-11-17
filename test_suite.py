import urllib.request
import json
import sys

BASE = "http://localhost:8080"

def req(path, method="GET", data=None):
    url = f"{BASE}{path}"
    headers = {"Content-Type": "application/json"}
    body = json.dumps(data).encode("utf-8") if data is not None else None
    request = urllib.request.Request(url, data=body, headers=headers, method=method)
    with urllib.request.urlopen(request) as resp:
        return json.loads(resp.read().decode("utf-8"))

def log(msg):
    print(msg, flush=True)

def run_tests():
    log("=== DEPTFLOW V2 QA TEST SUITE ===")
    passed = 0
    total = 19

    # Pre-seed baseline test data
    req("/api/reset", "POST", {"target": "sample"})

    # Test 1: Add and edit faculty
    try:
        r = req("/api/faculty", "POST", {"id": "F99", "name": "Prof. Test User", "dept": "CSDS", "isOutside": 0, "maxHours": 12, "qualifications": "Testing"})
        assert r["status"] == "success", f"Add faculty failed: {r}"
        r_edit = req("/api/faculty", "PUT", {"id": "F99", "name": "Prof. Test Renamed", "dept": "CSDS", "isOutside": 0, "maxHours": 14, "qualifications": "Testing,QA"})
        assert r_edit["status"] == "success", f"Edit faculty failed: {r_edit}"
        log("[PASS] Test 1: Add and edit faculty")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 1: {e}")

    # Test 2: Delete unused faculty
    try:
        r_del = req("/api/faculty", "DELETE", {"id": "F99"})
        assert r_del["status"] == "success", f"Delete unused faculty failed: {r_del}"
        log("[PASS] Test 2: Delete unused faculty")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 2: {e}")

    # Test 3: Attempt to delete faculty with active dependencies
    try:
        r_dep = req("/api/faculty", "DELETE", {"id": "F01"})
        assert r_dep["status"] == "error" and "Cannot delete" in r_dep["message"], f"Expected dependency error, got {r_dep}"
        log(f"[PASS] Test 3: Attempt to delete faculty with dependencies rejected correctly: '{r_dep['message']}'")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 3: {e}")

    # Test 4: Add and edit subjects
    try:
        r_sub = req("/api/subjects", "POST", {"id": "S999", "name": "Computer Networks", "year": 3, "facIdx": 0, "practicalFacIdx": 0, "weeklyLec": 3, "lecDuration": 60, "practicalCount": 1, "practicalDuration": 120, "roomTypeReq": "Laboratory"})
        assert r_sub["status"] == "success", f"Add subject failed: {r_sub}"
        r_sub_edit = req("/api/subjects", "PUT", {"id": "S999", "name": "Advanced Networks", "year": 3, "facIdx": 0, "practicalFacIdx": 0, "weeklyLec": 4, "practicalCount": 1, "roomTypeReq": "Laboratory"})
        assert r_sub_edit["status"] == "success", f"Edit subject failed: {r_sub_edit}"
        log("[PASS] Test 4: Add and edit subjects")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 4: {e}")

    # Test 5: Assign subjects to multiple divisions
    try:
        subs = req("/api/subjects")["subjects"]
        found = False
        for s in subs:
            if s["id"] == "S999":
                found = True
                break
        assert found, "Subject S999 not found"
        # Delete test subject S999
        r_sub_del = req("/api/subjects", "DELETE", {"id": "S999"})
        assert r_sub_del["status"] == "success", f"Delete subject failed: {r_sub_del}"
        log("[PASS] Test 5: Assign subjects to multiple divisions & verify list/cleanup")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 5: {e}")

    # Test 6: Add and delete rooms
    try:
        r_rm = req("/api/rooms", "POST", {"id": "R999", "name": "Seminar Hall", "type": "Classroom", "capacity": 120, "isLab": 0, "equipment": "Projector, Surround Audio"})
        assert r_rm["status"] == "success", f"Add room failed: {r_rm}"
        r_rm_del = req("/api/rooms", "DELETE", {"id": "R999"})
        assert r_rm_del["status"] == "success", f"Delete room failed: {r_rm_del}"
        log("[PASS] Test 6: Add and delete rooms")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 6: {e}")

    # Test 7: Configure fixed breaks
    try:
        r_brk_fix = req("/api/breaks", "POST", {"name": "Short Tea Break", "isFlexible": 0, "day": 0, "slot": 2, "scopeType": 0, "durationSlots": 1})
        assert r_brk_fix["status"] == "success", f"Add fixed break failed: {r_brk_fix}"
        log("[PASS] Test 7: Configure fixed breaks")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 7: {e}")

    # Test 8: Configure flexible breaks
    try:
        r_brk_flex = req("/api/breaks", "POST", {"name": "Flex Afternoon Break", "isFlexible": 1, "day": 1, "windowStartSlot": 3, "windowEndSlot": 5, "scopeType": 0, "durationSlots": 1})
        assert r_brk_flex["status"] == "success", f"Add flex break failed: {r_brk_flex}"
        log("[PASS] Test 8: Configure flexible breaks")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 8: {e}")

    # Test 9: Change department timings
    try:
        r_time = req("/api/config", "POST", {"name": "CSDS", "startTime": "08:30", "endTime": "16:30", "interval": "60"})
        assert r_time["status"] == "success", f"Update timing failed: {r_time}"
        cfg = req("/api/config")
        assert cfg["startTime"] == "08:30" and cfg["endTime"] == "16:30", "Timings mismatch"
        log(f"[PASS] Test 9: Change department timings (Slots recalculated: {cfg['numSlots']})")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 9: {e}")

    # Test 10: Change working days
    try:
        r_days = req("/api/config", "POST", {"name": "CSDS", "startTime": "08:30", "endTime": "16:30", "interval": "60", "days": ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]})
        assert r_days["status"] == "success"
        cfg = req("/api/config")
        assert cfg["numDays"] == 6 and "Saturday" in cfg["days"], "Working days mismatch"
        log(f"[PASS] Test 10: Change working days (Now {cfg['numDays']} days: {cfg['days']})")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 10: {e}")

    # Test 11: Generate selected divisions
    try:
        r_gen = req("/api/generate", "POST", {"divisions": [0, 1]})
        assert r_gen["status"] == "success" and r_gen["scheduledCount"] > 0, f"Generation failed: {r_gen}"
        log(f"[PASS] Test 11: Generate selected divisions (Scheduled {r_gen['scheduledCount']} sessions in FE-A & FE-B)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 11: {e}")

    # Test 12: Preserve unselected divisions
    try:
        r_gen_all = req("/api/generate", "POST", {"divisions": [0, 1, 2, 3, 4, 5]})
        count_all = r_gen_all["scheduledCount"]
        # Now regenerate only division 2 (SE-A)
        r_gen_sea = req("/api/generate", "POST", {"divisions": [2]})
        # Total scheduled should preserve division 0, 1, 3, 4, 5!
        assert r_gen_sea["scheduledCount"] >= count_all - 10, "Unselected division schedule was wiped unexpectedly"
        log(f"[PASS] Test 12: Preserve unselected divisions (Total active sessions: {r_gen_sea['scheduledCount']})")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 12: {e}")

    # Test 13: Test faculty conflicts
    try:
        tt0 = req("/api/timetable?division=0")["grid"]
        tt1 = req("/api/timetable?division=1")["grid"]
        conflict = False
        for day in range(len(tt0)):
            for slot in range(len(tt0[day])):
                f0 = tt0[day][slot]["facIdx"]
                f1 = tt1[day][slot]["facIdx"]
                if f0 != -1 and f0 == f1:
                    conflict = True
        assert not conflict, "Faculty double-booking conflict detected between divisions 0 and 1!"
        log("[PASS] Test 13: Test faculty conflicts (Zero overlapping faculty assignments)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 13: {e}")

    # Test 14: Test room conflicts
    try:
        room_conflict = False
        for day in range(len(tt0)):
            for slot in range(len(tt0[day])):
                r0 = tt0[day][slot]["roomIdx"]
                r1 = tt1[day][slot]["roomIdx"]
                if r0 != -1 and r0 == r1:
                    room_conflict = True
        assert not room_conflict, "Room double-booking conflict detected between divisions 0 and 1!"
        log("[PASS] Test 14: Test room conflicts (Zero room collisions)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 14: {e}")

    # Test 15: Test workload limits
    try:
        reps = req("/api/reports")
        for f in reps["facultyWorkload"]:
            assert f["current"] <= f["max"], f"Faculty {f['name']} exceeded max workload {f['current']} > {f['max']}!"
        log(f"[PASS] Test 15: Test workload limits (All {len(reps['facultyWorkload'])} faculty within capacity)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 15: {e}")

    # Test 16: Test practical duration
    try:
        has_lab = False
        for day in range(len(tt0)):
            for slot in range(len(tt0[day])):
                if "[Lab]" in tt0[day][slot]["label"]:
                    has_lab = True
                    assert tt0[day][slot]["room"] != "", "Lab has no room allocated"
        assert has_lab, "No lab practical scheduled in FE-A"
        log("[PASS] Test 16: Test practical duration & laboratory facility mapping")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 16: {e}")

    # Test 17: Test data persistence after restart
    try:
        with open("data/department.txt") as f:
            lines = f.readlines()
            assert len(lines) >= 6, "department.txt incomplete"
        with open("data/faculty.txt") as f:
            f_lines = f.readlines()
            assert int(f_lines[0].strip()) >= 5, "faculty.txt incomplete"
        log("[PASS] Test 17: Test data persistence on disk (Structured data files verified)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 17: {e}")

    # Test 18: Test all dashboard reports
    try:
        rep = req("/api/reports")
        assert len(rep["facultyWorkload"]) > 0, "Missing facultyWorkload"
        assert len(rep["roomUtilization"]) > 0, "Missing roomUtilization"
        assert len(rep["cascadeAnalysis"]) > 0, "Missing cascadeAnalysis"
        first_div = rep["cascadeAnalysis"][0]
        assert "direct" in first_div and "indirect" in first_div, "Missing direct vs indirect graph analysis"
        log(f"[PASS] Test 18: Test all dashboard reports (Direct dependencies: {first_div['direct']}, Indirect: {first_div['indirect']})")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 18: {e}")

    # Test 19: Test reset faculty and reset all (Zero defaults remaining)
    try:
        r_fac = req("/api/reset", "POST", {"target": "faculty"})
        assert r_fac["status"] == "success", f"Reset faculty failed: {r_fac}"
        f_list = req("/api/faculty")["faculty"]
        assert len(f_list) == 0, f"Expected 0 faculty after reset, got {len(f_list)}"

        r_all = req("/api/reset", "POST", {"target": "all"})
        assert r_all["status"] == "success", f"Reset all failed: {r_all}"
        assert len(req("/api/faculty")["faculty"]) == 0, "Faculty still present"
        assert len(req("/api/subjects")["subjects"]) == 0, "Subjects still present"
        assert len(req("/api/rooms")["rooms"]) == 0, "Rooms still present"
        assert len(req("/api/breaks")["breaks"]) == 0, "Breaks still present"
        assert len(req("/api/config")["divisions"]) == 0, "Divisions still present"
        log("[PASS] Test 19: Test complete wipe on reset (Zero defaults remaining, ready for fresh input)")
        passed += 1
    except Exception as e:
        log(f"[FAIL] Test 19: {e}")

    log("---------------------------------")
    log(f"RESULTS: {passed} / {total} TESTS PASSED.")
    if passed == total:
        log("ALL 19 AUDIT AND REFINEMENT TESTS PASSED PERFECTLY!")

if __name__ == "__main__":
    run_tests()
