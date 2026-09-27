#include "exclusive_output.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool formula_like(const std::string& id) {
    const auto first = id.find_first_not_of(" \t\f\v\r\n");
    return first != std::string::npos && std::string("=+-@").find(id[first]) != std::string::npos;
}

void run(const fs::path& input, const fs::path& output) {
    std::ifstream in(input);
    if (!in) throw std::runtime_error("cannot open input file");
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("empty input file");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != "student,exam") throw std::runtime_error("expected header student,exam");
    std::map<std::string, std::set<std::string>> by_student;
    std::map<std::string, std::set<std::string>> by_exam;
    std::size_t count = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto comma = line.find(',');
        if (comma == std::string::npos || comma == 0 || comma + 1 == line.size() ||
            line.find(',', comma + 1) != std::string::npos || line.find('"') != std::string::npos)
            throw std::runtime_error("invalid enrollment row");
        const std::string student = line.substr(0, comma);
        const std::string exam = line.substr(comma + 1);
        if (formula_like(exam)) throw std::runtime_error("formula-like exam name is not allowed");
        if (!by_student[student].insert(exam).second)
            throw std::runtime_error("duplicate enrollment: " + student + "," + exam);
        if (by_student[student].size() > 100)
            throw std::runtime_error("student exceeds 100 exams");
        by_exam[exam].insert(student);
        if (++count > 100000) throw std::runtime_error("too many enrollments");
    }
    if (!in.eof()) throw std::runtime_error("error reading input");
    if (count == 0) throw std::runtime_error("empty enrollment list");
    std::map<std::string, std::set<std::string>> neighbors;
    for (const auto& [exam, students] : by_exam) neighbors[exam];
    for (const auto& [student, exams] : by_student) {
        (void)student;
        for (auto first = exams.begin(); first != exams.end(); ++first) {
            for (auto second = std::next(first); second != exams.end(); ++second) {
                neighbors[*first].insert(*second);
                neighbors[*second].insert(*first);
            }
        }
    }
    std::vector<std::string> order;
    for (const auto& [exam, adjacent] : neighbors) { (void)adjacent; order.push_back(exam); }
    std::sort(order.begin(), order.end(), [&](const std::string& a, const std::string& b) {
        return neighbors[a].size() != neighbors[b].size() ? neighbors[a].size() > neighbors[b].size() : a < b;
    });
    std::map<std::string, int> slots;
    int slot_count = 0;
    for (const auto& exam : order) {
        std::set<int> used;
        for (const auto& neighbor : neighbors[exam]) {
            const auto existing = slots.find(neighbor);
            if (existing != slots.end()) used.insert(existing->second);
        }
        int slot = 1;
        while (used.count(slot)) ++slot;
        slots[exam] = slot;
        slot_count = std::max(slot_count, slot);
    }
    ExclusiveOutput file(output);
    std::ostream& out = file.stream();
    out << "exam,slot,students\n";
    for (const auto& [exam, students] : by_exam)
        out << exam << ',' << slots[exam] << ',' << students.size() << '\n';
    file.publish();
    std::cout << "exams=" << by_exam.size() << " slots=" << slot_count << '\n';
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: timetable <enrollments.csv> <slots.csv>");
        run(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
