Import("env")
from pathlib import Path
import base64

root = Path(env["PROJECT_DIR"])
asset = root / "assets" / "SKB_Audio_Logo.jpg"
out = root / "include" / "skb_logo.h"
out.parent.mkdir(parents=True, exist_ok=True)
if asset.exists():
    data = asset.read_bytes()
    bytes_text = ",".join(str(x) for x in data)
    out.write_text("#pragma once\n#include <stdint.h>\nstatic const uint8_t SKB_LOGO_DATA[] = {" + bytes_text + "};\nstatic constexpr unsigned SKB_LOGO_LENGTH = " + str(len(data)) + ";\n")
else:
    out.write_text("#pragma once\n#include <stdint.h>\nstatic const uint8_t SKB_LOGO_DATA[] = {0};\nstatic constexpr unsigned SKB_LOGO_LENGTH = 0;\n")
