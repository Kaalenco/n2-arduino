Import("env")
import re, os

build_file = os.path.join(env.subst("$PROJECT_DIR"), "src", "build_number.h")

build = 0
if os.path.exists(build_file):
    with open(build_file, "r") as f:
        for line in f:
            m = re.search(r"#define FW_BUILD (\d+)", line)
            if m:
                build = int(m.group(1)) + 1
                break

with open(build_file, "w") as f:
    f.write("#pragma once\n")
    f.write(f"#define FW_BUILD {build}\n")

print(f"Build number: {build}")
