from pathlib import Path
from PIL import Image, ImageChops
import subprocess, os, sys, math, operator

# Arguments
minetestpath = "./bin/minetest"
TEST_DIR = "util/fstest/tests"
REPORT_DIR = "util/fstest/reports"

if not os.path.exists(REPORT_DIR):
	os.mkdir(REPORT_DIR)

tolerance = 0.1
errors = False

def fail(msg):
	global errors
	errors = True

	print("ERR: " + msg)

def success(msg):
	print(msg)

def compare_images(one, two):
	diff = ImageChops.difference(Image.open(str(one)), Image.open(str(two)))
	if diff.getbbox() is None:
		return 0
	else:
		return 1337

def run_minetest(testfile, outputfile):
	config_path = "/tmp/minetest.conf"
	with open(config_path, "w") as f:
		f.write("""
			screen_w = 1024
			screen_h = 600
			video_driver = burningsvideo
		""")

	subprocess.run([minetestpath,
		"--render-formspec",
		"--file", testfile,
		"--o", outputfile,
		"--config", config_path ])


def make_diff(expected, actual, out):
	diff = ImageChops.difference(Image.open(expected), Image.open(actual))
	diff.save(out)


def run_test(dir, name):
	testfile = Path(dir, name + ".fs")
	expectedfile = Path(dir, name + ".expected.png")
	outputfile = Path(REPORT_DIR, name + ".result.png")

	# print("Running test {} (source={}, expected={}, output={}) ".format(name, testfile, expectedfile, outputfile))

	run_minetest(testfile, outputfile)

	assert(outputfile.exists())

	if expectedfile.exists():
		delta = compare_images(str(expectedfile), str(outputfile))
		if delta < tolerance:
			success("Test %s passed (delta=%.3f, tolerance=%.3f)" % (name, delta, tolerance))
			os.remove(outputfile)
		else:
			difffile = Path(REPORT_DIR, name + ".diff.png")
			make_diff(expectedfile, outputfile, difffile)

			msg = "Test %s failed, expected and actual are different (delta=%f, tolerance=%.3f)" % (name, delta, tolerance)
			msg += "\n  Source: " + str(testfile)
			msg += "\n  Difference: " + str(difffile)
			msg += "\n  Expected: " + str(expectedfile)
			msg += "\n  Actual: " + str(outputfile)
			fail(msg)
	else:
		fail("Test {} failed, no expected image found".format(name))


for testfile in Path(TEST_DIR).rglob("*.fs"):
	run_test(testfile.parent, testfile.stem)

sys.exit(1 if errors else 0)
