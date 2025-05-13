import numpy as np
import struct

with open("circle_map.bin","rb") as f:
    cnt = struct.unpack("<I", f.read(4))[0]
    data = np.fromfile(f, dtype=np.float32, count=cnt*3)
    circles = data.reshape(cnt,3)

print("count:", cnt)
for x,y,r in circles:
    print(f"{x:.2f}, {y:.2f}, {r:.2f}")
