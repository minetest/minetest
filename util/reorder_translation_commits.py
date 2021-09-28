#!/usr/bin/env python3
import sys
import subprocess

ret = subprocess.run(["git", "config", "rebase.instructionFormat"], capture_output=True)
if ret.returncode != 0 or ret.stdout.decode('ascii').strip() != "(%an <%ae>) %s":
	print("Git is using the wrong rebase instruction format, reconfigure it.")
	exit(1)

try:
	f = open(".git/rebase-merge/git-rebase-todo", "r")
except:
	print("Initiate the rebase first!")
	exit(1)
lines = list(s.strip("\r\n") for s in f.readlines())
f.close()

for i in range(len(lines)):
	line = lines[i]
	if line.startswith("#") or " Translated using Weblate " not in line: continue
	pos = line.rfind("(")
	lang = line[pos:]
	author = line[line.find("("):line.rfind(")", 0, pos)+1]
	# try to grab the next commit by the same author for the same language
	for j in range(i+1, len(lines)):
		if lines[j].startswith("#") or not lines[j].endswith(lang): continue
		if author in lines[j]:
			lines.insert(i+1, "f " + lines.pop(j)[5:])
		break

with open(".git/rebase-merge/git-rebase-todo", "w") as f:
	f.write("\n".join(lines) + "\n")
print("You can now continue with the rebase.")
