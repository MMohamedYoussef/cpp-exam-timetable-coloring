# Exam timetable coloring

Build a conflict graph from student enrollments and assign exam slots so no student sits two exams at once. The C++17 program orders exams by descending conflict degree, breaks ties alphabetically, and gives each exam the lowest unused neighboring slot.

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o timetable
./timetable fixtures/enrollments.csv example-out.csv
python3 tests/test_cli.py ./timetable
```

Input is unquoted CSV with the exact header `student,exam`; each student/exam pair appears once. Output is `exam,slot,students`, sorted by exam name. The demo produces [examples/slots.csv](examples/slots.csv) and prints `exams=4 slots=2`. This greedy graph coloring guarantees conflict-free slots but may use more slots than an optimal timetable.

At most 100,000 enrollments and 100 exams per student are accepted. Exam names beginning with `=`, `+`, `-`, or `@` after leading whitespace are rejected to keep exported CSV safe to open in a spreadsheet. The program does not model room capacity, dates, invigilators, or quoted CSV fields. Output is staged and published only if the path is absent; this requires hard-link support in the output directory. Malformed or duplicate data exits nonzero.
