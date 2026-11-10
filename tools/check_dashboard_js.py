#!/usr/bin/env python3
"""Validate embedded firmware dashboard JavaScript in CI using installed Node."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/main.cpp").read_text(encoding="utf-8")
match = re.search(r'R"SKBHTML\((.*?)\)SKBHTML"', source, re.DOTALL)
if not match:
    raise SystemExit("Embedded HTML dashboard not found")
html = match.group(1)
scripts = re.findall(r"<script(?:\s[^>]*)?>(.*?)</script>", html, re.DOTALL | re.IGNORECASE)
if not scripts:
    raise SystemExit("No inline JavaScript found in firmware dashboard")
with tempfile.TemporaryDirectory() as tmp:
    for i, script in enumerate(scripts):
        path = Path(tmp) / f"dashboard_{i}.js"
        path.write_text(script, encoding="utf-8")
        subprocess.run(["node", "--check", str(path)], check=True)
print(f"Validated syntax for {len(scripts)} embedded script(s)")
