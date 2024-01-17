import argparse
import shutil
import signal
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

import gymnasium as gym
import numpy as np
import pygame

from minetester.minetest_env import INVERSE_KEY_MAP, KEY_MAP

KEY_TO_KEYTYPE = {
    "W": "forward",
    "A": "left",
    "S": "backward",
    "D": "right",
    "SPACE": "jump",
    "LEFT SHIFT": "sneak",
    "J": "dig",
    "K": "place",
    "C": "cameraMode",
}
ARROW_KEYS_TO_MOUSE_DIRECTION = {
    "UP": (0, -20),
    "DOWN": (0, 20),
    "LEFT": (-20, 0),
    "RIGHT": (20, 0),
}


@dataclass
class Mouse:
    dx: float
    dy: float


def handle_key_event(event):
    key = pygame.key.name(event.key).upper()

    if event.type == pygame.KEYDOWN:
        if key in KEY_TO_KEYTYPE:
            keys_down.add(key)
        if key in ARROW_KEYS_TO_MOUSE_DIRECTION:
            mouse.dx += ARROW_KEYS_TO_MOUSE_DIRECTION[key][0]
            mouse.dy += ARROW_KEYS_TO_MOUSE_DIRECTION[key][1]
    elif event.type == pygame.KEYUP:
        if key in KEY_TO_KEYTYPE and key in keys_down:
            keys_down.remove(key)
        if key in ARROW_KEYS_TO_MOUSE_DIRECTION:
            mouse.dx -= ARROW_KEYS_TO_MOUSE_DIRECTION[key][0]
            mouse.dy -= ARROW_KEYS_TO_MOUSE_DIRECTION[key][1]


def get_action_from_key_cache(key_cache, mouse):
    keys = np.zeros(len(KEY_MAP), dtype=bool)
    for key in key_cache:
        keys[INVERSE_KEY_MAP[KEY_TO_KEYTYPE[key]]] = True

    mouse = np.array([mouse.dx, mouse.dy])
    return {"KEYS": keys, "MOUSE": mouse}


def game_loop():
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type in [pygame.KEYDOWN, pygame.KEYUP]:
                handle_key_event(event)

        action = get_action_from_key_cache(keys_down, mouse)
        state, reward, terminated, truncated, info = env.step(action)
        env.render()
        print(reward)

        if terminated:
            print("\n\n--TERMINATED--\n\n")
            running = False


def signal_handler(sig, frame):
    env.close()
    pygame.quit()


arg_parser = argparse.ArgumentParser()
arg_parser.add_argument(
    "--host_port",
    type=str,
    default=None,
    help=(
        "Minetest host:port to connect to. If set, will not "
        "start minetest (will assume it's already running)."
    ),
)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    keys_down = set()
    mouse = Mouse(0, 0)
    pygame.init()

    gym_make_args = {
        "id": "minetest-v0",
        "render_mode": "human",
        "display_size": (1024, 768),
    }
    if arg_parser.parse_args().host_port:
        host, port = arg_parser.parse_args().host_port.split(":")
        gym_make_args["zmq_host"] = host
        gym_make_args["zmq_port"] = int(port)
    else:
        original_world_dir = (
            Path(__file__).parent / "tests" / "worlds" / "test_world_minetestenv"
        )
        temp_dir = tempfile.TemporaryDirectory()
        temp_world_dir = Path(temp_dir) / "test_world_minetestenv"
        shutil.copytree(original_world_dir, temp_world_dir)
        gym_make_args["world_dir"] = temp_world_dir
        # The Makefile puts the binary into build/macos
        repo_root = Path(__file__).parent.parent.parent
        minetest_executable = repo_root / "bin" / "minetest"
        if sys.platform == "darwin":
            minetest_executable = (
                repo_root
                / "build"
                / "macos"
                / "minetest.app"
                / "Contents"
                / "MacOS"
                / "minetest"
            )
        gym_make_args["minetest_executable"] = minetest_executable
        gym_make_args["headless"] = False

    env = gym.make(**gym_make_args)
    env.reset()
    game_loop()
    env.close()
    pygame.quit()
