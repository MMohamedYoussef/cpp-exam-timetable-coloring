import pathlib
import subprocess
import sys
import tempfile
import unittest

BIN = pathlib.Path(sys.argv.pop(1)).resolve()


class TimetableTests(unittest.TestCase):
    def run_cli(self, data):
        with tempfile.TemporaryDirectory() as directory:
            source = pathlib.Path(directory) / "enrollments.csv"
            target = pathlib.Path(directory) / "slots.csv"
            source.write_text(data, encoding="utf-8")
            result = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            output = target.read_text(encoding="utf-8") if target.exists() else None
            return result, output

    def test_conflicting_exams_have_distinct_slots(self):
        data = "student,exam\nS1,Math\nS1,Physics\nS2,Math\nS2,Chemistry\nS3,Chemistry\nS3,History\n"
        result, output = self.run_cli(data)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, "exam,slot,students\nChemistry,1,2\nHistory,2,1\nMath,2,2\nPhysics,1,1\n")
        self.assertEqual(result.stdout.strip(), "exams=4 slots=2")

    def test_rejects_duplicate_enrollment(self):
        result, output = self.run_cli("student,exam\nS1,Math\nS1,Math\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate", result.stderr.lower())
        self.assertIsNone(output)

    def test_rejects_header_only(self):
        result, output = self.run_cli("student,exam\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("empty", result.stderr.lower())
        self.assertIsNone(output)

    def test_rejects_unbounded_single_student_conflict_graph(self):
        data = "student,exam\n" + "".join(f"S1,Exam{i}\n" for i in range(101))
        result, output = self.run_cli(data)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("100 exams", result.stderr.lower())
        self.assertIsNone(output)

    def test_rejects_formula_like_exam_names(self):
        for exam in ("=1+1", "+2", "-2", "@SUM", "  =1+1"):
            with self.subTest(exam=exam):
                result, output = self.run_cli(f"student,exam\nS1,{exam}\n")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("formula", result.stderr.lower())
                self.assertIsNone(output)

    def test_rejects_existing_output_without_temporary_residue(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "enrollments.csv"
            target = root / "slots.csv"
            source.write_text("student,exam\nS1,Math\n", encoding="utf-8")
            target.write_text("keep", encoding="utf-8")
            existing = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            self.assertNotEqual(existing.returncode, 0)
            self.assertEqual(target.read_text(encoding="utf-8"), "keep")
            self.assertEqual(list(root.glob("slots.csv.tmp-*")), [])

    def test_refuses_dangling_symlink_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "enrollments.csv"
            target = root / "slots.csv"
            missing = root / "missing.csv"
            source.write_text("student,exam\nS1,Math\n", encoding="utf-8")
            try:
                target.symlink_to(missing)
            except (OSError, NotImplementedError):
                self.skipTest("creating symlinks is unavailable on this host")
            result = subprocess.run([str(BIN), str(source), str(target)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue(target.is_symlink())
            self.assertFalse(missing.exists())
            self.assertEqual(list(root.glob("slots.csv.tmp-*")), [])


if __name__ == "__main__":
    unittest.main()
