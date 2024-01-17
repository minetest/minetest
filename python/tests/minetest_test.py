import logging
import os
import shutil
import sys
import tempfile
from pathlib import Path

import gymnasium as gym
import numpy as np
import pytest
from PIL import Image

from minetester.minetest_env import INVERSE_KEY_MAP


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


def test_minetest_basic(world_dir, caplog):
    caplog.set_level(logging.DEBUG)
    is_mac = sys.platform == "darwin"
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

    artifact_dir = tempfile.mkdtemp()
    env = gym.make(
        "minetest-v0",
        minetest_executable=minetest_executable,
        artifact_dir=artifact_dir,
        render_mode="rgb_array",
        display_size=(223, 111),
        world_dir=world_dir,
        headless=True,
    )
    env.reset()

    # Context manager to make sure close() is called even if test fails.
    with env:
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
            # TODO: I've seen the system get into a mode where the output is always 480, 640, 3
            # Seems like something to do with OpenGL driver initialization.
            expected_shape = (111, 223, 3)
            obs_sum = obs.sum()
            expected_reward = 1
            # clunky `if`` and then assert to make sure we get a screenshot if the test fails.
            if (
                (obs.shape != expected_shape)
                or (obs_sum <= 0)
                or (reward != expected_reward)
            ):
                screenshot_path = os.path.join(
                    artifact_dir, f"minetst_test_obs_{i}.png"
                )
                Image.fromarray(obs).save(screenshot_path)
                assert obs.shape == expected_shape, f"see image: {screenshot_path}"
                assert obs.sum() > 0, f"All black image, see image: {screenshot_path}"
                assert reward == expected_reward, f"see image: {screenshot_path}"

    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.
