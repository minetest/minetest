import os
import socket
from pathlib import Path

import capnp
import numpy as np
from PIL import Image

"""
For debugging, it's useful to do something like this:
- lldb -- bin/minetest --go --worldname test_world_minetestenv --config artifacts/2dd22d78-8c03-445e-83ad-8fff429569d4.conf --remote-input 127.0.0.1:54321
- Then run python tools/handshaker.py
"""

remoteclient_capnp = capnp.load(
    os.path.join(Path(__file__).parent.parent, "minetest/proto/remoteclient.capnp")
)
my_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
my_socket.connect(("127.0.0.1", 54321))
print("connected")
capnp_client = (
    capnp.TwoPartyClient(my_socket).bootstrap().cast_as(remoteclient_capnp.Minetest)
)
print("capnp_client.init()")
capnp_client.init().wait()
print("init() done")

i = 0
inp = ""
while inp != "stop":
    step_request = capnp_client.step_request()
    step_response = step_request.send().wait()
    print(type(step_response))
    img_data = np.frombuffer(
        step_response.observation.image.data, dtype=np.uint8
    ).reshape(
        (
            step_response.observation.image.height,
            step_response.observation.image.width,
            3,
        )
    )
    if img_data.size > 0:
        Image.fromarray(img_data).save(f"observation_{i}.png")

    i += 1
    inp = input("Stop with 'stop':")
