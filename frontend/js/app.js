// DeptFlow V2 Presentation Layer Controller
let appState = {
    dept: {},
    faculty: [],
    subjects: [],
    rooms: [],
    breaks: [],
    divisions: [],
    activeDivision: 0,
    activeCell: null
};

// UI Initialization
document.addEventListener('DOMContentLoaded', () => {
    setupNavigation();
    setupEventListeners();
    fetchInitialData();
});

// Navigation Controller
function setupNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            navItems.forEach(n => n.classList.remove('active'));
            item.classList.add('active');

            const tabId = item.getAttribute('data-tab');
            document.querySelectorAll('.tab-pane').forEach(tab => tab.classList.remove('active'));
            const targetTab = document.getElementById(`tab-${tabId}`);
            if (targetTab) targetTab.classList.add('active');

            if (tabId === 'reports') loadReports();
        });
    });
}

// Event Listeners for Forms & Actions
function setupEventListeners() {
    // Division Select Filter
    document.getElementById('divSelect').addEventListener('change', (e) => {
        appState.activeDivision = parseInt(e.target.value, 10);
        updateDivisionHeadings();
        loadTimetable(appState.activeDivision);
    });

    // Quick Gen & Generator Tab
    document.getElementById('btnQuickGen').addEventListener('click', () => executeGenerator());
    document.getElementById('btnExecuteGenerator').addEventListener('click', () => executeGenerator());

    // Division Checkboxes
    document.getElementById('btnSelectAllDivs').addEventListener('click', () => {
        document.querySelectorAll('.div-checkbox').forEach(cb => cb.checked = true);
    });
    document.getElementById('btnDeselectAllDivs').addEventListener('click', () => {
        document.querySelectorAll('.div-checkbox').forEach(cb => cb.checked = false);
    });

    // Department Config Form
    document.getElementById('formDeptConfig').addEventListener('submit', async (e) => {
        e.preventDefault();
        const selectedDays = [];
        document.querySelectorAll('#cfgDaysContainer input:checked').forEach(cb => selectedDays.push(cb.value));
        const payload = {
            name: document.getElementById('cfgDeptName').value,
            startTime: document.getElementById('cfgStartTime').value,
            endTime: document.getElementById('cfgEndTime').value,
            interval: document.getElementById('cfgInterval').value,
            days: selectedDays
        };
        await postData('/api/config', payload, 'POST', 'Department configuration updated & slots recalculated');
        fetchInitialData();
    });

    // Faculty Form (Add / Edit)
    document.getElementById('formAddFaculty').addEventListener('submit', async (e) => {
        e.preventDefault();
        const isEdit = document.getElementById('facEditMode').value === '1';
        const payload = {
            id: document.getElementById('facId').value,
            name: document.getElementById('facName').value,
            dept: document.getElementById('facDept').value,
            qualifications: document.getElementById('facQualifications').value,
            isOutside: parseInt(document.getElementById('facIsOutside').value, 10),
            maxHours: parseInt(document.getElementById('facMaxHours').value, 10)
        };
        const method = isEdit ? 'PUT' : 'POST';
        const msg = isEdit ? 'Faculty member updated' : 'Faculty member registered';
        await postData('/api/faculty', payload, method, msg);
        resetFacultyForm();
        loadFaculty();
    });
    document.getElementById('btnCancelFacultyEdit').addEventListener('click', resetFacultyForm);

    // Subject Form (Add / Edit)
    document.getElementById('formAddSubject').addEventListener('submit', async (e) => {
        e.preventDefault();
        const isEdit = document.getElementById('subEditMode').value === '1';
        const payload = {
            id: document.getElementById('subId').value,
            name: document.getElementById('subName').value,
            year: parseInt(document.getElementById('subYear').value, 10),
            facIdx: parseInt(document.getElementById('subFaculty').value, 10),
            practicalFacIdx: parseInt(document.getElementById('subPracticalFaculty').value, 10),
            weeklyLec: parseInt(document.getElementById('subWeeklyLec').value, 10),
            lecDuration: parseInt(document.getElementById('subLecDuration').value, 10),
            practicalCount: parseInt(document.getElementById('subPracticals').value, 10),
            practicalDuration: parseInt(document.getElementById('subPracticalDuration').value, 10),
            roomTypeReq: document.getElementById('subRoomTypeReq').value
        };
        const method = isEdit ? 'PUT' : 'POST';
        const msg = isEdit ? 'Subject curriculum updated' : 'Academic subject added';
        await postData('/api/subjects', payload, method, msg);
        resetSubjectForm();
        loadSubjects();
    });
    document.getElementById('btnCancelSubEdit').addEventListener('click', resetSubjectForm);

    // Division Add Box
    document.getElementById('btnAddDivision').addEventListener('click', async () => {
        const name = document.getElementById('newDivName').value.trim();
        const yr = parseInt(document.getElementById('newDivYear').value, 10);
        if (!name) { showToast('Please enter a division name (e.g. FE-C)', 'error'); return; }
        await postData('/api/divisions', { name: name, year: yr }, 'POST', 'Academic division added');
        document.getElementById('newDivName').value = '';
        fetchInitialData();
    });

    // Room Form (Add / Edit)
    document.getElementById('formAddRoom').addEventListener('submit', async (e) => {
        e.preventDefault();
        const isEdit = document.getElementById('roomEditMode').value === '1';
        const payload = {
            id: document.getElementById('roomId').value,
            name: document.getElementById('roomName').value,
            type: document.getElementById('roomIsLab').value === '1' ? 'Laboratory' : 'Classroom',
            isLab: parseInt(document.getElementById('roomIsLab').value, 10),
            capacity: parseInt(document.getElementById('roomCapacity').value, 10),
            equipment: document.getElementById('roomEquipment').value,
            isAvailable: parseInt(document.getElementById('roomIsAvailable').value, 10)
        };
        const method = isEdit ? 'PUT' : 'POST';
        const msg = isEdit ? 'Facility details updated' : 'Facility registered';
        await postData('/api/rooms', payload, method, msg);
        resetRoomForm();
        loadRooms();
    });
    document.getElementById('btnCancelRoomEdit').addEventListener('click', resetRoomForm);

    // Break Mode Toggle (Fixed vs Flexible)
    document.getElementById('brkIsFlexible').addEventListener('change', (e) => {
        const isFlex = e.target.value === '1';
        document.getElementById('groupFixedSlot').classList.toggle('hidden', isFlex);
        document.getElementById('groupFlexWindow').classList.toggle('hidden', !isFlex);
    });

    // Break Form
    document.getElementById('formAddBreak').addEventListener('submit', async (e) => {
        e.preventDefault();
        const isFlex = document.getElementById('brkIsFlexible').value === '1';
        const payload = {
            name: document.getElementById('brkName').value,
            isFlexible: isFlex ? 1 : 0,
            slot: parseInt(document.getElementById('brkSlot').value || '3', 10),
            windowStartSlot: parseInt(document.getElementById('brkWindowStart').value || '0', 10),
            windowEndSlot: parseInt(document.getElementById('brkWindowEnd').value || '5', 10),
            durationSlots: parseInt(document.getElementById('brkDuration').value, 10),
            day: parseInt(document.getElementById('brkDay').value, 10),
            scopeType: parseInt(document.getElementById('brkScopeType').value, 10),
            scopeVal: 0
        };
        await postData('/api/breaks', payload, 'POST', 'Break interval scheduled');
        loadBreaks();
    });

    // Session Modal Controls
    document.getElementById('btnCloseSessionModal').addEventListener('click', () => {
        document.getElementById('sessionModal').classList.add('hidden');
    });
    document.getElementById('btnModalSave').addEventListener('click', async () => {
        if (!appState.activeCell) return;
        const payload = {
            action: 'edit',
            divIdx: appState.activeCell.divIdx,
            day: appState.activeCell.day,
            slot: appState.activeCell.slot,
            facIdx: parseInt(document.getElementById('modalFacultySelect').value, 10),
            roomIdx: parseInt(document.getElementById('modalRoomSelect').value, 10)
        };
        await postData('/api/timetable/session', payload, 'POST', 'Session updated');
        document.getElementById('sessionModal').classList.add('hidden');
        loadTimetable(appState.activeDivision);
        loadFaculty();
    });
    document.getElementById('btnModalDelete').addEventListener('click', async () => {
        if (!appState.activeCell) return;
        if (!confirm('Are you sure you want to clear this scheduled session?')) return;
        const payload = {
            action: 'delete',
            divIdx: appState.activeCell.divIdx,
            day: appState.activeCell.day,
            slot: appState.activeCell.slot
        };
        await postData('/api/timetable/session', payload, 'POST', 'Session removed');
        document.getElementById('sessionModal').classList.add('hidden');
        loadTimetable(appState.activeDivision);
        loadFaculty();
    });
    document.getElementById('btnModalReschedule').addEventListener('click', async () => {
        if (!appState.activeCell) return;
        const payload = {
            action: 'reschedule',
            divIdx: appState.activeCell.divIdx,
            day: appState.activeCell.day,
            slot: appState.activeCell.slot,
            newDay: parseInt(document.getElementById('modalNewDaySelect').value, 10),
            newSlot: parseInt(document.getElementById('modalNewSlotSelect').value, 10)
        };
        await postData('/api/timetable/session', payload, 'POST', 'Session rescheduled');
        document.getElementById('sessionModal').classList.add('hidden');
        loadTimetable(appState.activeDivision);
        loadFaculty();
    });

    // Global Undo Button
    document.getElementById('btnUndoSession').addEventListener('click', async () => {
        await postData('/api/timetable/session', { action: 'undo' }, 'POST', 'Last modification undone');
        loadTimetable(appState.activeDivision);
        loadFaculty();
    });

    // Reset Timetable Handlers
    const handleResetAction = async () => {
        if (!confirm('Are you sure you want to reset the timetable? This will clear all scheduled sessions in the C++ engine and update storage.')) return;
        showToast('Resetting timetable via C++ engine...', 'success');
        const res = await fetch('/api/timetable/reset', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ division: -1 })
        });
        const result = await res.json();
        if (result.status === 'success') {
            showToast('Timetable has been completely reset!', 'success');
            document.getElementById('statusLastGenBadge').textContent = `Last Gen: ${result.lastGenerated || 'Reset'}`;
            const resContainer = document.getElementById('genResultsContainer');
            if (resContainer) resContainer.classList.add('hidden');
            await loadTimetable(appState.activeDivision);
            await loadFaculty();
        } else {
            showToast(result.message || 'Failed to reset timetable', 'error');
        }
    };

    if (document.getElementById('btnResetTimetable')) {
        document.getElementById('btnResetTimetable').addEventListener('click', handleResetAction);
    }
    if (document.getElementById('btnGenResetTimetable')) {
        document.getElementById('btnGenResetTimetable').addEventListener('click', handleResetAction);
    }

    // Section & Full System Reset Handlers
    const btnResetAll = document.getElementById('btnResetAllSystem');
    if (btnResetAll) {
        btnResetAll.addEventListener('click', async () => {
            if (!confirm('WARNING: Are you sure you want to clear ALL data?\n\nThis will remove all faculty, subjects, rooms, breaks, divisions, and timetables so you have a completely clean slate with zero default data.')) return;
            const ok = await postData('/api/reset', { target: 'all' }, 'POST', 'All data cleared. No defaults remaining.');
            if (ok) {
                await fetchInitialData();
            }
        });
    }

    const btnResetDept = document.getElementById('btnResetDept');
    if (btnResetDept) {
        btnResetDept.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to clear department divisions and reset operational hours?')) return;
            const ok = await postData('/api/reset', { target: 'department' }, 'POST', 'Department divisions cleared');
            if (ok) {
                await fetchInitialData();
            }
        });
    }

    const btnResetFaculty = document.getElementById('btnResetFaculty');
    if (btnResetFaculty) {
        btnResetFaculty.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to remove ALL faculty members?\n\nThis will clear the entire faculty directory so you can add your faculty members from scratch.')) return;
            const ok = await postData('/api/reset', { target: 'faculty' }, 'POST', 'All faculty removed. You can now add faculty.');
            if (ok) {
                await loadFaculty();
                await loadTimetable(appState.activeDivision);
            }
        });
    }

    const btnResetSubjects = document.getElementById('btnResetSubjects');
    if (btnResetSubjects) {
        btnResetSubjects.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to remove ALL curriculum subjects?\n\nThis will clear all subjects so you can add your subjects from scratch.')) return;
            const ok = await postData('/api/reset', { target: 'subjects' }, 'POST', 'All subjects removed. You can now add subjects.');
            if (ok) {
                await loadSubjects();
                await loadTimetable(appState.activeDivision);
            }
        });
    }

    const btnResetRooms = document.getElementById('btnResetRooms');
    if (btnResetRooms) {
        btnResetRooms.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to remove ALL classrooms and laboratories?\n\nThis will clear the facilities directory so you can register facilities from scratch.')) return;
            const ok = await postData('/api/reset', { target: 'rooms' }, 'POST', 'All facilities removed. You can now add rooms.');
            if (ok) {
                await loadRooms();
            }
        });
    }

    const btnResetBreaks = document.getElementById('btnResetBreaks');
    if (btnResetBreaks) {
        btnResetBreaks.addEventListener('click', async () => {
            if (!confirm('Are you sure you want to remove ALL break schedules?')) return;
            const ok = await postData('/api/reset', { target: 'breaks' }, 'POST', 'All breaks cleared');
            if (ok) {
                await loadBreaks();
                await loadTimetable(appState.activeDivision);
            }
        });
    }

    // Print & Export PDF
    document.getElementById('btnPrintTimetable').addEventListener('click', () => window.print());
    document.getElementById('btnExportPdf').addEventListener('click', () => window.print());
}

// Fetch Initial Configuration & Datasets
async function fetchInitialData() {
    try {
        const confRes = await fetch('/api/config');
        appState.dept = await confRes.json();
        appState.divisions = appState.dept.divisions || [];

        // Update headers & config
        document.getElementById('deptHeaderTitle').textContent = `${appState.dept.name} Timetable System`;
        document.getElementById('deptSubtitle').textContent = `Department of ${appState.dept.name} | Weekly Master Schedule`;
        document.getElementById('cfgDeptName').value = appState.dept.name || 'CSDS';
        document.getElementById('cfgStartTime').value = appState.dept.startTime || '09:00';
        document.getElementById('cfgEndTime').value = appState.dept.endTime || '16:00';
        document.getElementById('cfgInterval').value = appState.dept.interval || '60';
        document.getElementById('statusLastGenBadge').textContent = `Last Gen: ${appState.dept.lastGenerated || 'Ready'}`;

        // Populate Division Select dropdown
        const divSel = document.getElementById('divSelect');
        divSel.innerHTML = '';
        appState.divisions.forEach(d => {
            const opt = document.createElement('option');
            opt.value = d.id;
            opt.textContent = `Year ${d.year} - ${d.name}`;
            divSel.appendChild(opt);
        });
        if (appState.divisions.length > 0 && appState.activeDivision >= appState.divisions.length) {
            appState.activeDivision = 0;
        }
        divSel.value = appState.activeDivision;
        updateDivisionHeadings();

        // Populate Division Quick Pills
        const pillsContainer = document.getElementById('divisionPillsList');
        pillsContainer.innerHTML = '';
        appState.divisions.forEach(d => {
            const pill = document.createElement('div');
            pill.className = 'pill-item';
            pill.innerHTML = `<span><strong>${d.name}</strong> (Yr ${d.year})</span> <span class="pill-del" onclick="deleteDivision(${d.id})">&times;</span>`;
            pillsContainer.appendChild(pill);
        });

        // Populate Break Form Selectors
        const brkSlotSel = document.getElementById('brkSlot');
        const brkWsSel = document.getElementById('brkWindowStart');
        const brkWeSel = document.getElementById('brkWindowEnd');
        brkSlotSel.innerHTML = ''; brkWsSel.innerHTML = ''; brkWeSel.innerHTML = '';
        (appState.dept.times || []).forEach((t, i) => {
            const label = `Slot ${i + 1} (${t})`;
            brkSlotSel.innerHTML += `<option value="${i}">${label}</option>`;
            brkWsSel.innerHTML += `<option value="${i}">${label}</option>`;
            brkWeSel.innerHTML += `<option value="${i}">${label}</option>`;
        });

        const brkDaySel = document.getElementById('brkDay');
        brkDaySel.innerHTML = '';
        (appState.dept.days || []).forEach((day, i) => {
            brkDaySel.innerHTML += `<option value="${i}">${day}</option>`;
        });

        // Generator Division Checkboxes
        const cbGrid = document.getElementById('divisionCheckboxes');
        cbGrid.innerHTML = '';
        appState.divisions.forEach(d => {
            const label = document.createElement('label');
            label.className = 'checkbox-card';
            label.innerHTML = `<input type="checkbox" class="div-checkbox" value="${d.id}" checked> <span>${d.name} (Yr ${d.year})</span>`;
            cbGrid.appendChild(label);
        });

        await Promise.all([
            loadFaculty(),
            loadSubjects(),
            loadRooms(),
            loadBreaks(),
            loadTimetable(appState.activeDivision)
        ]);

        updateDashboardMetrics();
    } catch (err) {
        console.error('Initialization error:', err);
        showToast('Error connecting to C++ backend', 'error');
    }
}

// Update Division Headings on Dashboard
function updateDivisionHeadings() {
    const activeDiv = appState.divisions.find(d => d.id === appState.activeDivision);
    if (activeDiv) {
        document.getElementById('currentDivHeading').textContent = `Weekly Master Schedule – Division ${activeDiv.name}`;
        document.getElementById('currentDivLabel').textContent = `Academic Year ${activeDiv.year} • Operational Hours: ${appState.dept.startTime} - ${appState.dept.endTime}`;
    }
}

// Load Faculty Directory
async function loadFaculty() {
    const res = await fetch('/api/faculty');
    const data = await res.json();
    appState.faculty = data.faculty || [];

    const tbody = document.querySelector('#facultyTable tbody');
    tbody.innerHTML = '';

    const subFacSel = document.getElementById('subFaculty');
    const subPracFacSel = document.getElementById('subPracticalFaculty');
    const modalFacSel = document.getElementById('modalFacultySelect');
    subFacSel.innerHTML = ''; subPracFacSel.innerHTML = ''; modalFacSel.innerHTML = '';

    if (appState.faculty.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align:center; padding:24px; color:#6b7280;">No faculty members registered. Add faculty members using the form on the left.</td></tr>';
        const emptyOpt = '<option value="-1">-- No Faculty Registered --</option>';
        subFacSel.innerHTML = emptyOpt;
        subPracFacSel.innerHTML = emptyOpt;
        modalFacSel.innerHTML = emptyOpt;
    } else {
        appState.faculty.forEach((f, idx) => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${f.id}</strong></td>
                <td>${f.name}</td>
                <td>${f.dept}</td>
                <td><span class="status-pill ${f.isOutside ? 'blue' : 'green'}">${f.isOutside ? 'Outside' : 'Dept'}</span></td>
                <td><strong>${f.currHours}</strong> / ${f.maxHours} hrs</td>
                <td><span class="text-muted">${f.qualifications || 'General'}</span></td>
                <td>
                    <div class="action-btn-group">
                        <button class="btn btn-sm btn-outline" onclick="editFaculty('${f.id}')">Edit</button>
                        <button class="btn btn-sm btn-outline" style="color:#dc2626;" onclick="deleteFaculty('${f.id}')">Delete</button>
                    </div>
                </td>
            `;
            tbody.appendChild(tr);

            const opt = `<option value="${idx}">${f.name} (${f.dept})</option>`;
            subFacSel.innerHTML += opt;
            subPracFacSel.innerHTML += opt;
            modalFacSel.innerHTML += opt;
        });
    }

    updateDashboardMetrics();
}

function editFaculty(id) {
    const f = appState.faculty.find(item => item.id === id);
    if (!f) return;
    document.getElementById('facultyFormTitle').textContent = `Edit Faculty Member (${f.id})`;
    document.getElementById('facEditMode').value = '1';
    document.getElementById('facId').value = f.id;
    document.getElementById('facId').readOnly = true;
    document.getElementById('facName').value = f.name;
    document.getElementById('facDept').value = f.dept;
    document.getElementById('facQualifications').value = f.qualifications || '';
    document.getElementById('facIsOutside').value = f.isOutside ? '1' : '0';
    document.getElementById('facMaxHours').value = f.maxHours;
    document.getElementById('btnCancelFacultyEdit').classList.remove('hidden');
}

function resetFacultyForm() {
    document.getElementById('formAddFaculty').reset();
    document.getElementById('facultyFormTitle').textContent = 'Add Faculty Member';
    document.getElementById('facEditMode').value = '0';
    document.getElementById('facId').readOnly = false;
    document.getElementById('btnCancelFacultyEdit').classList.add('hidden');
}

async function deleteFaculty(id) {
    if (!confirm(`Are you sure you want to delete faculty member '${id}'? This will verify dependencies.`)) return;
    await postData('/api/faculty', { id: id }, 'DELETE', 'Faculty member removed');
    loadFaculty();
}

// Load Subjects Directory
async function loadSubjects() {
    const res = await fetch('/api/subjects');
    const data = await res.json();
    appState.subjects = data.subjects || [];

    const tbody = document.querySelector('#subjectsTable tbody');
    tbody.innerHTML = '';
    if (appState.subjects.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align:center; padding:24px; color:#6b7280;">No subjects configured. Add academic subjects using the form on the left.</td></tr>';
    } else {
        appState.subjects.forEach(s => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${s.id}</strong></td>
                <td>${s.name}</td>
                <td>Year ${s.year}</td>
                <td>${s.facName}</td>
                <td>${s.weeklyLec}L (${s.lecDuration}m) + ${s.practicalCount}P (${s.practicalDuration}m)</td>
                <td><span class="status-pill ${s.roomTypeReq === 'Laboratory' ? 'blue' : 'green'}">${s.roomTypeReq}</span></td>
                <td>
                    <div class="action-btn-group">
                        <button class="btn btn-sm btn-outline" onclick="editSubject('${s.id}')">Edit</button>
                        <button class="btn btn-sm btn-outline" style="color:#dc2626;" onclick="deleteSubject('${s.id}')">Delete</button>
                    </div>
                </td>
            `;
            tbody.appendChild(tr);
        });
    }

    updateDashboardMetrics();
}

function editSubject(id) {
    const s = appState.subjects.find(item => item.id === id);
    if (!s) return;
    document.getElementById('subjectFormTitle').textContent = `Edit Subject (${s.id})`;
    document.getElementById('subEditMode').value = '1';
    document.getElementById('subId').value = s.id;
    document.getElementById('subId').readOnly = true;
    document.getElementById('subName').value = s.name;
    document.getElementById('subYear').value = s.year;
    document.getElementById('subFaculty').value = s.facIdx;
    document.getElementById('subPracticalFaculty').value = s.practicalFacIdx;
    document.getElementById('subWeeklyLec').value = s.weeklyLec;
    document.getElementById('subLecDuration').value = s.lecDuration || 60;
    document.getElementById('subPracticals').value = s.practicalCount;
    document.getElementById('subPracticalDuration').value = s.practicalDuration || 120;
    document.getElementById('subRoomTypeReq').value = s.roomTypeReq;
    document.getElementById('btnCancelSubEdit').classList.remove('hidden');
}

function resetSubjectForm() {
    document.getElementById('formAddSubject').reset();
    document.getElementById('subjectFormTitle').textContent = 'Add / Edit Academic Subject';
    document.getElementById('subEditMode').value = '0';
    document.getElementById('subId').readOnly = false;
    document.getElementById('btnCancelSubEdit').classList.add('hidden');
}

async function deleteSubject(id) {
    if (!confirm(`Delete subject '${id}'? This will check for active timetable dependencies.`)) return;
    await postData('/api/subjects', { id: id }, 'DELETE', 'Subject deleted');
    loadSubjects();
}

async function deleteDivision(id) {
    if (!confirm(`Delete this division? All its existing timetable entries will be purged.`)) return;
    await postData('/api/divisions', { id: id }, 'DELETE', 'Division removed');
    fetchInitialData();
}

// Load Rooms Directory
async function loadRooms() {
    const res = await fetch('/api/rooms');
    const data = await res.json();
    appState.rooms = data.rooms || [];

    const tbody = document.querySelector('#roomsTable tbody');
    tbody.innerHTML = '';
    const modalRoomSel = document.getElementById('modalRoomSelect');
    modalRoomSel.innerHTML = '';

    if (appState.rooms.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align:center; padding:24px; color:#6b7280;">No facilities registered. Register classrooms or labs using the form on the left.</td></tr>';
        modalRoomSel.innerHTML = '<option value="-1">-- No Facilities Available --</option>';
    } else {
        appState.rooms.forEach((r, idx) => {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><strong>${r.id}</strong></td>
                <td>${r.name}</td>
                <td><span class="status-pill ${r.isLab ? 'blue' : 'green'}">${r.type}</span></td>
                <td>${r.capacity} Seats</td>
                <td><span class="text-muted">${r.equipment || 'Standard'}</span></td>
                <td><span class="status-pill ${r.isAvailable ? 'green' : 'warning'}">${r.isAvailable ? 'Available' : 'Blocked'}</span></td>
                <td>
                    <div class="action-btn-group">
                        <button class="btn btn-sm btn-outline" onclick="editRoom('${r.id}')">Edit</button>
                        <button class="btn btn-sm btn-outline" style="color:#dc2626;" onclick="deleteRoom('${r.id}')">Delete</button>
                    </div>
                </td>
            `;
            tbody.appendChild(tr);
            modalRoomSel.innerHTML += `<option value="${idx}">${r.name} (${r.type})</option>`;
        });
    }

    updateDashboardMetrics();
}

function editRoom(id) {
    const r = appState.rooms.find(item => item.id === id);
    if (!r) return;
    document.getElementById('roomFormTitle').textContent = `Edit Facility (${r.id})`;
    document.getElementById('roomEditMode').value = '1';
    document.getElementById('roomId').value = r.id;
    document.getElementById('roomId').readOnly = true;
    document.getElementById('roomName').value = r.name;
    document.getElementById('roomIsLab').value = r.isLab ? '1' : '0';
    document.getElementById('roomCapacity').value = r.capacity;
    document.getElementById('roomEquipment').value = r.equipment || '';
    document.getElementById('roomIsAvailable').value = r.isAvailable ? '1' : '0';
    document.getElementById('btnCancelRoomEdit').classList.remove('hidden');
}

function resetRoomForm() {
    document.getElementById('formAddRoom').reset();
    document.getElementById('roomFormTitle').textContent = 'Add / Edit Room or Laboratory';
    document.getElementById('roomEditMode').value = '0';
    document.getElementById('roomId').readOnly = false;
    document.getElementById('btnCancelRoomEdit').classList.add('hidden');
}

async function deleteRoom(id) {
    if (!confirm(`Delete facility '${id}'? This will verify whether it is currently occupied.`)) return;
    await postData('/api/rooms', { id: id }, 'DELETE', 'Facility removed');
    loadRooms();
}

// Load Breaks Directory
async function loadBreaks() {
    const res = await fetch('/api/breaks');
    const data = await res.json();
    appState.breaks = data.breaks || [];

    const tbody = document.querySelector('#breaksTable tbody');
    tbody.innerHTML = '';
    const days = appState.dept.days || [];
    const times = appState.dept.times || [];

    if (appState.breaks.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; padding:24px; color:#6b7280;">No breaks scheduled. Configure breaks using the form on the left.</td></tr>';
    } else {
        appState.breaks.forEach((b, idx) => {
            const tr = document.createElement('tr');
            const scopeStr = b.scopeType === 0 ? 'All Department' : (b.scopeType === 1 ? `Year ${b.scopeVal}` : `Division`);
            const timeStr = b.isFlexible
                ? `Window: Slot ${b.windowStartSlot + 1}-${b.windowEndSlot + 1} (${b.durationSlots} slot)`
                : (times[b.slot] || `Slot ${b.slot + 1}`);
            tr.innerHTML = `
                <td><strong>${b.name}</strong></td>
                <td><span class="status-pill ${b.isFlexible ? 'blue' : 'green'}">${b.isFlexible ? 'Flexible' : 'Fixed'}</span></td>
                <td>${days[b.day] || `Day ${b.day + 1}`} • ${timeStr}</td>
                <td><span class="badge-status">${scopeStr}</span></td>
                <td>
                    <button class="btn btn-sm btn-outline" style="color:#dc2626;" onclick="deleteBreak(${idx})">Delete</button>
                </td>
            `;
            tbody.appendChild(tr);
        });
    }
}

async function deleteBreak(idx) {
    if (!confirm('Remove this scheduled break?')) return;
    await postData('/api/breaks', { index: idx }, 'DELETE', 'Break removed');
    loadBreaks();
}

// Load Timetable Grid
async function loadTimetable(divIdx) {
    try {
        const res = await fetch(`/api/timetable?division=${divIdx}`);
        const data = await res.json();
        const grid = data.grid || [];

        const headerRow = document.getElementById('timetableHeaderRow');
        headerRow.innerHTML = '<th>Time Slot</th>';
        const days = appState.dept.days || ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"];
        days.forEach(day => {
            const th = document.createElement('th');
            th.textContent = day;
            headerRow.appendChild(th);
        });

        // Setup Modal New Day / Slot Selectors
        const modalNewDaySel = document.getElementById('modalNewDaySelect');
        const modalNewSlotSel = document.getElementById('modalNewSlotSelect');
        modalNewDaySel.innerHTML = ''; modalNewSlotSel.innerHTML = '';
        days.forEach((day, i) => modalNewDaySel.innerHTML += `<option value="${i}">${day}</option>`);

        const tbody = document.getElementById('timetableBody');
        tbody.innerHTML = '';
        const times = appState.dept.times || [];
        times.forEach((t, i) => modalNewSlotSel.innerHTML += `<option value="${i}">Slot ${i + 1} (${t})</option>`);

        for (let s = 0; s < times.length; s++) {
            const tr = document.createElement('tr');
            const slotTd = document.createElement('td');
            slotTd.className = 'slot-col';
            slotTd.innerHTML = `Slot ${s + 1}<br><span style="font-size:11px; font-weight:400; color:#64748b;">${times[s]}</span>`;
            tr.appendChild(slotTd);

            for (let day = 0; day < days.length; day++) {
                const td = document.createElement('td');
                const cell = (grid[day] && grid[day][s]) ? grid[day][s] : null;

                if (!cell || (cell.subIdx === -1 && !cell.isBreak)) {
                    td.innerHTML = '<div class="cell-empty">Free</div>';
                } else if (cell.isBreak) {
                    td.innerHTML = `<div class="cell-break">${cell.label || 'BREAK'}</div>`;
                } else {
                    const isLab = cell.label && cell.label.includes('[Lab]');
                    const isMan = cell.isManual;
                    const cellDiv = document.createElement('div');
                    cellDiv.className = `cell-card ${isLab ? 'lab' : ''} ${isMan ? 'manual' : ''}`;
                    cellDiv.innerHTML = `
                        <div class="cell-sub">${cell.subject}</div>
                        <div class="cell-fac">${cell.faculty || 'Staff'}</div>
                        <div class="cell-tags">
                            <span class="cell-room">${cell.room || 'CR'}</span>
                            ${isLab ? '<span class="badge-lab">Lab</span>' : ''}
                            ${isMan ? '<span class="badge-manual">Edited</span>' : ''}
                        </div>
                    `;
                    cellDiv.addEventListener('click', () => openSessionModal(divIdx, day, s, cell));
                    td.appendChild(cellDiv);
                }
                tr.appendChild(td);
            }
            tbody.appendChild(tr);
        }
    } catch (err) {
        console.error('Failed to load timetable:', err);
    }
}

// Open Interactive Session Modal
function openSessionModal(divIdx, day, slot, cell) {
    appState.activeCell = { divIdx, day, slot, cell };
    const days = appState.dept.days || [];
    const times = appState.dept.times || [];

    document.getElementById('modalSessionTitle').textContent = `${cell.subject} (${cell.label && cell.label.includes('[Lab]') ? 'Lab' : 'Lecture'})`;
    document.getElementById('modalSessionDesc').textContent = `Division: ${appState.divisions.find(d => d.id === divIdx)?.name || ''} • Current: ${days[day] || ''}, ${times[slot] || ''}`;
    document.getElementById('modalFacultySelect').value = cell.facIdx;
    document.getElementById('modalRoomSelect').value = cell.roomIdx;
    document.getElementById('modalNewDaySelect').value = day;
    document.getElementById('modalNewSlotSelect').value = slot;

    document.getElementById('sessionModal').classList.remove('hidden');
}

// Execute Selective Timetable Generator
async function executeGenerator() {
    const selected = [];
    document.querySelectorAll('.div-checkbox:checked').forEach(cb => {
        selected.push(parseInt(cb.value, 10));
    });

    if (selected.length === 0) {
        showToast('Please select at least one division to generate', 'error');
        return;
    }

    showToast('Executing C++ Scheduling Engine...', 'success');
    const res = await fetch('/api/generate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ divisions: selected })
    });
    const result = await res.json();

    const resContainer = document.getElementById('genResultsContainer');
    resContainer.classList.remove('hidden');
    document.getElementById('resScheduledCount').textContent = result.scheduledCount || 0;
    document.getElementById('resUnscheduledCount').textContent = result.unscheduledCount || 0;
    document.getElementById('resTimestamp').textContent = result.lastGenerated || 'Just now';
    document.getElementById('statusLastGenBadge').textContent = `Last Gen: ${result.lastGenerated || 'Ready'}`;

    const unschedSection = document.getElementById('unscheduledSection');
    const unschedTable = document.querySelector('#unscheduledTable tbody');
    unschedTable.innerHTML = '';
    if (result.unscheduledCount > 0 && result.unscheduled) {
        unschedSection.classList.remove('hidden');
        result.unscheduled.forEach(u => {
            const tr = document.createElement('tr');
            tr.innerHTML = `<td><strong>${u.subject}</strong></td><td>${u.division}</td><td style="color:#b91c1c;">${u.reason}</td>`;
            unschedTable.appendChild(tr);
        });
    } else {
        unschedSection.classList.add('hidden');
    }

    showToast(`Generation complete! Scheduled: ${result.scheduledCount}`, 'success');
    await loadTimetable(appState.activeDivision);
    await loadFaculty();
}

// Load Reports & Graph BFS Dependency Analysis
async function loadReports() {
    const res = await fetch('/api/reports');
    const data = await res.json();

    // Faculty load bars
    const facContainer = document.getElementById('facultyLoadBars');
    facContainer.innerHTML = '';
    (data.facultyWorkload || []).forEach(f => {
        const item = document.createElement('div');
        item.className = 'progress-item';
        item.innerHTML = `
            <div class="progress-item-header">
                <span>${f.name} (${f.dept})</span>
                <span><strong>${f.current}</strong> / ${f.max} hrs (${f.percent}%)</span>
            </div>
            <div class="progress-bar-bg">
                <div class="progress-bar-fill" style="width: ${Math.min(100, f.percent)}%; ${f.percent > 90 ? 'background-color:#ea580c;' : ''}"></div>
            </div>
        `;
        facContainer.appendChild(item);
    });

    // Room utilization bars
    const roomContainer = document.getElementById('roomUtilizationBars');
    roomContainer.innerHTML = '';
    (data.roomUtilization || []).forEach(r => {
        const item = document.createElement('div');
        item.className = 'progress-item';
        item.innerHTML = `
            <div class="progress-item-header">
                <span>${r.name} (${r.type} • ${r.capacity} Seats)</span>
                <span><strong>${r.used}</strong> / ${r.total} slots (${r.percent}%)</span>
            </div>
            <div class="progress-bar-bg">
                <div class="progress-bar-fill" style="width: ${Math.min(100, r.percent)}%; background-color: #2563eb;"></div>
            </div>
        `;
        roomContainer.appendChild(item);
    });

    // Cascade Graph BFS with Direct vs Indirect distinction
    const cascadeTbody = document.querySelector('#cascadeTable tbody');
    cascadeTbody.innerHTML = '';
    (data.cascadeAnalysis || []).forEach(c => {
        const tr = document.createElement('tr');
        const directList = (c.direct && c.direct.length > 0)
            ? c.direct.map(d => `<span class="status-pill green">${d}</span>`).join(' ')
            : '<span class="text-muted">None</span>';
        const indirectList = (c.indirect && c.indirect.length > 0)
            ? c.indirect.map(d => `<span class="status-pill blue">${d}</span>`).join(' ')
            : '<span class="text-muted">None</span>';
        tr.innerHTML = `
            <td><strong>${c.division}</strong></td>
            <td>${directList}</td>
            <td>${indirectList}</td>
            <td><span class="badge-status">${c.connectedCount} Linked Divisions</span></td>
        `;
        cascadeTbody.appendChild(tr);
    });
}

// Generic Post Helper
async function postData(url, data, method = 'POST', successMsg = 'Operation succeeded') {
    try {
        const res = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        const result = await res.json();
        if (result.status === 'success') {
            showToast(successMsg, 'success');
            return true;
        } else {
            showToast(result.message || 'Operation failed', 'error');
            return false;
        }
    } catch (err) {
        showToast('Network communication error with C++ backend', 'error');
        return false;
    }
}

// Update Dashboard Statistics
function updateDashboardMetrics() {
    document.getElementById('statFacultyCount').textContent = appState.faculty.length;
    const outsideCount = appState.faculty.filter(f => f.isOutside).length;
    document.getElementById('statOutsideFac').textContent = `${outsideCount} Outside Faculty`;

    document.getElementById('statDivisionCount').textContent = appState.divisions.length;
    document.getElementById('statSubjectCount').textContent = appState.subjects.length;

    const labCount = appState.subjects.reduce((sum, s) => sum + (s.practicalCount || 0), 0);
    document.getElementById('statLabCount').textContent = `${labCount} Lab Practicals`;

    document.getElementById('statRoomCount').textContent = appState.rooms.length;
}

// Toast helper
function showToast(message, type = 'success') {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = `toast ${type}`;
    setTimeout(() => {
        toast.className = 'toast hidden';
    }, 4000);
}
