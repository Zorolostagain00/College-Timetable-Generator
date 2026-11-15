
#include <iostream>
#include <string>
#include <queue>
#include <iomanip>
#include <cstdlib>
#include <ctime>
using namespace std;
struct Subject
{
    string name, teacher;
    int lectures, teacherId;
};
int main()
{
    const int DAYS = 5, SLOTS = 10, MAX = 10;
    string divisions[] = {"A", "B", "C"};
    string days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    string times[] = {"8-9", "9-10", "10-11", "11-12", "12-1",
                      "1-2", "2-3", "3-4", "4-5", "5-6"};
    Subject s[MAX];
    string teachers[MAX];
    int t[3][DAYS][SLOTS], n, tc = 0, total = 0;
    bool busy[MAX][DAYS][SLOTS] = {};
    cout << "COLLEGE TIMETABLE GENERATOR\n";
    cout << "Enter number of subjects: ";
    cin >> n;
    if (n < 1 || n > MAX) return 0;
    for (int i = 0; i < n; i++)
    {
        cout << "\nSubject: ";
        getline(cin >> ws, s[i].name);
        cout << "Teacher: ";
        getline(cin >> ws, s[i].teacher);
        cout << "Lectures per week: ";
        cin >> s[i].lectures;
        if (s[i].lectures < 1 || s[i].lectures > 5) return 0;
        total += s[i].lectures;
        int id = 0;
        while (id < tc && teachers[id] != s[i].teacher) id++;
        if (id == tc) teachers[tc++] = s[i].teacher;
        s[i].teacherId = id;
    }
    if (total > 40) return cout << "Too many lectures.\n", 0;
    for (int i = 0; i < tc; i++)
    {
        int load = 0;
        for (int j = 0; j < n; j++)
            if (s[j].teacherId == i) load += s[j].lectures;
        if (load * 3 > 40) return cout << "Teacher workload too high.\n", 0;
    }
    srand(time(0));
    bool success = false;
    for (int attempt = 0; attempt < 500 && !success; attempt++)
    {
        for (int i = 0; i < tc; i++)
            for (int j = 0; j < DAYS; j++)
                for (int k = 0; k < SLOTS; k++)
                    busy[i][j][k] = false;
        for (int d = 0; d < 3; d++)
            for (int i = 0; i < DAYS; i++)
                for (int j = 0; j < SLOTS; j++)
                    t[d][i][j] = (j == 4 || j == 7) ? -2 : -1;
        success = true;
        for (int d = 0; d < 3 && success; d++)
        {
            queue<int> q;
            for (int i = 0; i < n; i++)
                for (int j = 0; j < s[i].lectures; j++) q.push(i);
            while (!q.empty())
            {
                int x = q.front();
                q.pop();
                bool placed = false;
                for (int day = 0; day < DAYS && !placed; day++)
                {
                    bool found = false;
                    for (int j = 0; j < SLOTS; j++)
                        if (t[d][day][j] == x) found = true;
                    if (found) continue;
                    for (int j = 0; j < SLOTS; j++)
                        if (t[d][day][j] == -1 &&
                            !busy[s[x].teacherId][day][j])
                        {
                            t[d][day][j] = x;
                            busy[s[x].teacherId][day][j] = true;
                            placed = true;
                            break;
                        }
                }
                if (!placed) success = false;
                if (!success) break;
            }
        }
    }
    if (!success) return cout << "Scheduling failed. Try again.\n", 0;
    for (int d = 0; d < 3; d++)
    {
        cout << "\nDIVISION " << divisions[d] << "\n";
        cout << left << setw(14) << "DAY";
        for (int j = 0; j < SLOTS; j++) cout << setw(12) << times[j];
        cout << "\n";
        for (int i = 0; i < DAYS; i++)
        {
            cout << left << setw(14) << days[i];
            for (int j = 0; j < SLOTS; j++)
            {
                string v = t[d][i][j] == -2 ? "BREAK" :
                           t[d][i][j] == -1 ? "FREE" : s[t[d][i][j]].name;
                cout << setw(12) << v;
            }
            cout << "\n";
        }
    }
    cout << "\nSUBJECT DETAILS\n";
    for (int i = 0; i < n; i++)
        cout << s[i].name << " - " << s[i].teacher
             << " - " << s[i].lectures << " lectures\n";

    return 0;
}
