import logging
import os
import shutil
import socket
import sys
import tempfile
from pathlib import Path

import gymnasium as gym
import numpy as np
import pytest
from PIL import Image

from minetest.minetest_env import INVERSE_KEY_MAP


@pytest.fixture
def world_dir():
    repo_root = Path(__file__).parent.parent.parent
    original_world_dir = (
        repo_root / "python" / "tests" / "worlds" / "test_world_minetestenv"
    )
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_world_dir = Path(temp_dir) / "test_world_minetestenv"
        shutil.copytree(original_world_dir, temp_world_dir)
        yield temp_world_dir


# take a lucky guess at a free port
# this works by having the OS return a free port, then immediately closing the socket
# not guaranteed to be free, but should be good enough for our purposes
def get_free_port():
    s = socket.socket()
    s.bind(("", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def test_minetest_basic(world_dir, caplog):
    caplog.set_level(logging.DEBUG)
    minetest_executable = shutil.which("minetest")
    is_mac = sys.platform == "darwin"
    if not minetest_executable:
        repo_root = Path(__file__).parent.parent.parent
        if is_mac:
            minetest_executable = (
                repo_root
                / "build"
                / "macos"
                / "minetest.app"
                / "Contents"
                / "MacOS"
                / "minetest"
            )
        else:
            minetest_executable = repo_root / "bin" / "minetest"
        assert minetest_executable.exists()

    server_addr = None
    if is_mac:
        # mac doesn't support abstract unix sockets, so use TCP
        server_addr = f"localhost:{get_free_port()}"
    artifact_dir = tempfile.mkdtemp()
    display_size = (223, 111)
    env = gym.make(
        "minetest-v0",
        executable=minetest_executable,
        artifact_dir=artifact_dir,
        server_addr=server_addr,
        render_mode="rgb_array",
        display_size=display_size,
        world_dir=world_dir,
        headless=True,
        verbose_logging=True,
        additional_observation_spaces={
            "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
        },
    )

    # Context manager to make sure close() is called even if test fails.
    with env:
        initial_obs, info = env.reset()
        nonzero_reward = False
        expected_shape = display_size + (3,)
        for i in range(5):
            action = {
                "KEYS": np.zeros(len(INVERSE_KEY_MAP), dtype=bool),
                "MOUSE": np.array([0.0, 0.0]),
            }

            if i == 3:
                action["KEYS"][INVERSE_KEY_MAP["forward"]] = True
                action["KEYS"][INVERSE_KEY_MAP["left"]] = True
                action["MOUSE"] = np.array([0.0, 1.0])

            obs, reward, terminated, truncated, info = env.step(action)
            assert not terminated and not truncated
            assert "return" in obs
            assert "IMAGE" in obs
            # TODO: I've seen the system get into a mode where the output is always 480, 640, 3
            # Seems like something to do with OpenGL driver initialization.
            # clunky `if`` and then assert to make sure we get a screenshot if the test fails.
            img_data = obs["IMAGE"]
            if img_data.shape != expected_shape:
                screenshot_path = os.path.join(
                    artifact_dir, f"minetst_test_obs_{i}.png"
                )
                Image.fromarray(img_data).save(screenshot_path)
                assert img_data.shape == expected_shape, f"see image: {screenshot_path}"
            if reward > 0:
                nonzero_reward = True
            # The screen is always black when rendering with mesa on Linux.
            # This is a bug but we don't care about this case, so check only
            # on Mac or when rendering with nvidia.
            if sys.platform == "darwin" or shutil.which("nvidia-smi"):
                assert img_data.sum() > 0, "All black image"
    assert nonzero_reward, f"see images in {artifact_dir}"

    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.
