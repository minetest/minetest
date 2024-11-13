import asyncio
import logging
import os
import shutil
import sys
import tempfile
import unittest.mock
from pathlib import Path

import gymnasium as gym
import numpy as np
import pytest
from minetest import minetest_env
from minetest.minetest_env import KEY_NAME_TO_INDEX, action_noop
from PIL import Image


@pytest.fixture
def artifact_dir():
    return tempfile.mkdtemp()


@pytest.fixture
def world_dir():
    world_dir = Path(__file__).parent / "worlds"
    original_world_dir = world_dir / "test_world_minetestenv"
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_world_dir = Path(temp_dir) / "test_world_minetestenv"
        shutil.copytree(original_world_dir, temp_world_dir)
        yield temp_world_dir


@pytest.fixture
def minetest_executable():
    executable = shutil.which("minetest")
    if not executable:
        repo_root = Path(__file__).parent.parent.parent
        if sys.platform == "darwin":
            executable = (
                repo_root
                / "build"
                / "macos"
                / "minetest.app"
                / "Contents"
                / "MacOS"
                / "minetest"
            )
        else:
            executable = repo_root / "bin" / "minetest"
        assert executable.exists()
    return executable


@pytest.fixture
def env(artifact_dir, world_dir, minetest_executable):
    render_size = (223, 111)
    env = gym.make(
        "minetest-v0",
        executable=minetest_executable,
        artifact_dir=artifact_dir,
        headless=sys.platform == "linux",
        render_size=render_size,
        world_dir=world_dir,
        verbose_logging=True,
        additional_observation_spaces={
            "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
        },
    )
    yield env
    env.close()


def contains_key(config_path: os.PathLike, key: str):
    with open(config_path) as f:
        for line in f:
            if line.startswith(key):
                return True
    return False


def test_new_env_after_exception(artifact_dir, world_dir, minetest_executable, caplog):
    caplog.set_level(logging.DEBUG)

    def make_env():
        return gym.make(
            "minetest-v0",
            executable=minetest_executable,
            artifact_dir=artifact_dir,
            headless=sys.platform == "linux",
            world_dir=world_dir,
            verbose_logging=True,
            additional_observation_spaces={
                "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
            },
        )

    env = make_env()
    with unittest.mock.patch.object(
        env.unwrapped, "_poll_minetest_process", return_value=1
    ), pytest.raises(RuntimeError):
        initial_obs, info = env.reset()
    with make_env() as env:
        initial_obs, info = env.reset()

    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.


def test_double_reset(env, artifact_dir, caplog):
    caplog.set_level(logging.DEBUG)

    # Context manager to make sure close() is called even if test fails.
    initial_obs, info = env.reset()
    initial_obs, info = env.reset()

    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.


def test_minetest_basic(artifact_dir, world_dir, minetest_executable, caplog):
    caplog.set_level(logging.DEBUG)
    render_size = (223, 111)
    env = gym.make(
        "minetest-v0",
        headless=sys.platform == "linux",
        executable=minetest_executable,
        artifact_dir=artifact_dir,
        render_size=render_size,
        world_dir=world_dir,
        verbose_logging=True,
        additional_observation_spaces={
            "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
        },
    )

    # Context manager to make sure close() is called even if test fails.
    with env:
        initial_obs, info = env.reset()
        # should not be set when we specify world_dir and don't set world_seed.
        assert not contains_key(env.unwrapped.config_path, "fixed_map_seed")
        nonzero_reward = False
        expected_shape = render_size + (3,)
        for i in range(5):
            action = action_noop()

            if i == 3:
                action["keys"][KEY_NAME_TO_INDEX["forward"]] = True
                action["keys"][KEY_NAME_TO_INDEX["left"]] = True
                action["mouse"] = np.array([0.0, 1.0])

            obs, reward, terminated, truncated, info = env.step(action)
            assert not terminated
            assert not truncated
            assert "return" in obs
            assert "image" in obs
            # TODO: I've seen the system get into a mode where the output is always 480, 640, 3
            # Seems like something to do with OpenGL driver initialization.
            # clunky `if`` and then assert to make sure we get a screenshot if the test fails.
            img_data = obs["image"]
            if img_data.shape != expected_shape:
                screenshot_path = os.path.join(
                    artifact_dir, f"minetest_test_obs_{i}.png"
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


def test_minetest_game_dir(artifact_dir, minetest_executable, caplog):
    caplog.set_level(logging.DEBUG)
    repo_root = Path(__file__).parent.parent.parent
    devetest_game_dir = repo_root / "games" / "devtest"
    assert devetest_game_dir.exists()
    env = gym.make(
        "minetest-v0",
        executable=minetest_executable,
        artifact_dir=artifact_dir,
        headless=sys.platform == "linux",
        world_dir=None,
        game_dir=devetest_game_dir,
        verbose_logging=True,
        additional_observation_spaces={
            "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
        },
    )

    # Context manager to make sure close() is called even if test fails.
    with env:
        initial_obs, info = env.reset()
        # should be set when we specify game_dir and not world_dir
        assert contains_key(env.unwrapped.config_path, "fixed_map_seed")
    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.


def test_async_vector_env(artifact_dir, world_dir, minetest_executable, caplog):
    caplog.set_level(logging.DEBUG)
    num_envs = 2
    max_episode_steps = 3
    async_env_context = None
    headless = True
    if sys.platform == "darwin":
        # https://github.com/Farama-Foundation/Gymnasium/issues/222
        async_env_context = "fork"
        headless = False
    envs = [
        lambda: gym.make(
            "minetest-v0",
            max_episode_steps=max_episode_steps,
            headless=headless,
            executable=minetest_executable,
            artifact_dir=artifact_dir,
            world_dir=world_dir,
            verbose_logging=True,
            additional_observation_spaces={
                "return": gym.spaces.Box(low=-(2**20), high=2**20, shape=(1,))
            },
        )
        for _ in range(num_envs)
    ]
    env = gym.vector.AsyncVectorEnv(envs, context=async_env_context)

    initial_obs, info = env.reset()
    assert len(initial_obs["image"]) == num_envs
    assert len(initial_obs["return"]) == num_envs
    for i in range(5):
        action = {
            "keys": np.zeros((num_envs, len(KEY_NAME_TO_INDEX)), dtype=bool),
            "mouse": np.zeros((num_envs, 2)),
        }
        if i == 3:
            action["keys"][:, KEY_NAME_TO_INDEX["forward"]] = True
            action["keys"][:, KEY_NAME_TO_INDEX["left"]] = True
            action["mouse"] = np.tile(np.array([0.0, 1.0]), (num_envs, 1))
        obs, reward, terminated, truncated, info = env.step(action)
        assert len(obs["image"]) == num_envs
        assert len(obs["return"]) == num_envs
        assert len(reward) == num_envs
        assert len(terminated) == num_envs
        assert len(truncated) == num_envs
        assert not terminated.any()
        if i == max_episode_steps - 1:
            assert truncated.all()
        else:
            assert not truncated.any()

    shutil.rmtree(artifact_dir)  # Only on success so we can inspect artifacts.
    env.close()


def test_run_on_event_loop_timeout(env):
    env = env.unwrapped
    with pytest.raises(asyncio.TimeoutError):
        env._run_on_event_loop(asyncio.sleep(1), timeout=0.1)


def test_build_action_message():
    action_message = minetest_env.remoteclient_capnp.Action.new_message()
    action = minetest_env.action_from_key_name("toggleHud")
    minetest_env.serialize_action(action, action_message)
