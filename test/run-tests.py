import os
import subprocess
import filecmp

TEST_DIR = "."
LIBERTY2JSON_EXE = "../build/liberty2json"

def check_liberty_json(json_filename):
  """Load a Liberty JSON file"""
  with open(json_filename, encoding="utf-8") as json_file:
    json_contents = json_file.read()
    if '"cell_rise":' in json_contents or '"cell_fall":' in json_contents:
      print("  NLDM timing found in file:", json_filename)
    if '"output_current_rise":' in json_contents or '"output_current_fall":' in json_contents:
      print("  CCS timing found in file:", json_filename)

def create_reference_files():
	for file_name in os.listdir(TEST_DIR):
		if file_name.endswith(".lib"):
			lib_file = os.path.join(TEST_DIR, file_name)
			ref_file = lib_file.replace(".lib", ".ref.json")
			
			# Run liberty2json on the .lib file to create the reference JSON file
			subprocess.run([LIBERTY2JSON_EXE, lib_file, "--outfile", ref_file])

def run_tests():
	# Delete old .test.json files
	for file_name in os.listdir(TEST_DIR):
		if file_name.endswith(".test.json"):
			os.remove(os.path.join(TEST_DIR, file_name))
	for file_name in os.listdir(TEST_DIR):
		if file_name.endswith(".lib"):
			lib_file = os.path.join(TEST_DIR, file_name)
			json_file = lib_file.replace(".lib", ".test.json")
			ref_file = lib_file.replace(".lib", ".ref.json")
			
			# Run liberty2json on the .lib file
			subprocess.run([LIBERTY2JSON_EXE, lib_file, "--outfile", json_file])
			
			# Compare the output JSON file to the reference file
			if "syntaxerr" in json_file or "example.include" in json_file:
				continue
			try:
				check_liberty_json(json_file)
				if filecmp.cmp(json_file, ref_file, shallow=False):
					print(f"Test passed for {file_name}")
				else:
					print(f"Test failed for {file_name}")
			except FileNotFoundError:
				print(f"File not found: check {json_file} or {ref_file}")

if __name__ == "__main__":
	# create_reference_files()
	run_tests()
