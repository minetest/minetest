import itertools
import os
import tempfile
from collections import OrderedDict
from copy import deepcopy
from typing import Any, Callable, Iterator, Optional, SupportsFloat, TypedDict

import gymnasium
import numpy as np
from gymnasium.core import WrapperActType, WrapperObsType

import minetest.minetest_env
from minetest.minetest_env import KEY_MAP as KEYBOARD_ACTION_KEYS

# Keyboard actions
N_KEYBOARD_ACTIONS = len(KEYBOARD_ACTION_KEYS)
KEYBOARD_NOOP = np.zeros(N_KEYBOARD_ACTIONS)
MOVEMENT_KEYS = ["forward", "left", "backward", "right"]
BOAD_KEYBOARD_ACTION_KEYS = MOVEMENT_KEYS + ["dig"]


def get_keyboard_actions(key: str, idx: int) -> Iterator[np.ndarray]:
    action_value = deepcopy(KEYBOARD_NOOP)
    action_value[idx] = 1
    if key in MOVEMENT_KEYS:
        return tuple(
            itertools.chain(
                itertools.repeat(action_value, times=1),
                itertools.repeat(KEYBOARD_NOOP, times=0),
            )
        )
    return (action_value,)


KEYBOARD_ACTIONS = OrderedDict(
    [(k, get_keyboard_actions(k, idx)) for idx, k in enumerate(KEYBOARD_ACTION_KEYS)]
)

# Mouse actions
MOUSE_NOOP = [0, 0]
MOUSE_SCALE = 64
MOUSE_ACTIONS = OrderedDict(
    [
        ("mouse_left", [MOUSE_SCALE, 0]),
        ("mouse_right", [-MOUSE_SCALE, 0]),
        ("mouse_up", [0, MOUSE_SCALE]),
        ("mouse_down", [0, -MOUSE_SCALE]),
    ]
)
N_MOUSE_ACTIONS = len(MOUSE_ACTIONS)
BOAD_MOUSE_ACTION_KEYS = ["mouse_left", "mouse_right", "mouse_up", "mouse_down"]


def get_mouse_actions(unit_action: list[int], n_steps: int) -> Iterator[list[int]]:
    cum_sweep = 0
    for step in range(1, n_steps + 1):
        next_cum_sweep = MOUSE_SCALE * (step // n_steps)
        diff = next_cum_sweep - cum_sweep
        yield [x * diff for x in unit_action]
        cum_sweep = next_cum_sweep
    yield from itertools.repeat(MOUSE_NOOP)


# actions
NOOP_ACTION = {
    "keys": (KEYBOARD_NOOP,),
    "mouse": MOUSE_NOOP,
}
ACTION_KEYS = KEYBOARD_ACTION_KEYS + list(MOUSE_ACTIONS.keys())
REPEATED_ACTION_KEYS = set(MOVEMENT_KEYS + list(MOUSE_ACTIONS.keys()))
REPEATED_ACTION_IDXS = set(
    [i for i, v in enumerate(ACTION_KEYS) if v in REPEATED_ACTION_KEYS]
)


def get_action_dicts(
    keyboard_action_keys: list[str] = KEYBOARD_ACTION_KEYS,
    mouse_action_keys: list[str] = list(MOUSE_ACTIONS.keys()),
) -> list[dict[str, Any]]:
    n_keyboard_actions = len(keyboard_action_keys)
    n_mouse_actions = len(mouse_action_keys)
    action_dicts = [
        deepcopy(NOOP_ACTION) for _ in range(n_keyboard_actions + n_mouse_actions)
    ]

    for action_idx, keyboard_key in enumerate(keyboard_action_keys):
        action_dicts[action_idx]["keys"] = KEYBOARD_ACTIONS[keyboard_key]

    mouse_action_values = [
        v for k, v in MOUSE_ACTIONS.items() if k in mouse_action_keys
    ]
    for action_idx, mouse_value in enumerate(
        mouse_action_values, start=n_keyboard_actions
    ):
        action_dicts[action_idx]["mouse"] = mouse_value

    return action_dicts


class BoadConfig(TypedDict, total=False):
    food_consumption_per_second: int
    water_consumption_per_second: int
    disable_night: bool
    apple_scale: float
    rose_scale: float
    food_gain_reward: float
    water_gain_reward: float
    rose_gain_reward: float
    death_reward: float


def _get_boad_config(
    food_consumption_per_second: int = 20,
    water_consumption_per_second: int = 20,
    disable_night: bool = True,
    apple_scale: float = 2.5,
    rose_scale: float = 1.5,
    food_gain_reward: float = 0.0,
    water_gain_reward: float = 0.0,
    rose_gain_reward: float = 1.0,
    death_reward: float = -5.0,
) -> str:
    return f"""\
-- Set to a number to fix the rng seed.
--RANDOM_SEED=nil

BOAD_DISABLE_NIGHT={"true" if disable_night else "false"}

-- Automatic respawn does not work well in our minetest fork, this avoids death while still penalizing would-be-death.
BOAD_DEATH_WORKAROUND=true

-- Arena configuration
-- BOAD_ARENA_SIZE=60
-- BOAD_NUM_SNOW=10
-- BOAD_NUM_APPLE=10
-- BOAD_APPLE_DESPAWN_CHANCE_PERCENTAGE=25
-- BOAD_SNOW_DESPAWN_CHANCE_PERCENTAGE=25
-- BOAD_RESPAWN_FOOD=true
BOAD_APPLE_SCALE={apple_scale}
BOAD_ROSE_SCALE={rose_scale}

-- Starvation configuration
-- BOAD_FOOD_MAX=1000
BOAD_FOOD_CONSUMPTION_PER_SECOND={food_consumption_per_second}
-- BOAD_STARVATION_DAMAGE_PER_SECOND=1

-- Dehydration configuration
-- BOAD_WATER_MAX=1000
BOAD_WATER_CONSUMPTION_PER_SECOND={water_consumption_per_second}
-- BOAD_DEHYDRATION_DAMAGE_PER_SECOND=1

-- Score configuration
BOAD_FOOD_GAIN_REWARD={food_gain_reward}
BOAD_WATER_GAIN_REWARD={water_gain_reward}
BOAD_ROSE_GAIN_REWARD={rose_gain_reward}
BOAD_DEATH_REWARD={death_reward}
"""


def _write_boad_config(config: BoadConfig, game_dir: str) -> None:
    config_lua = _get_boad_config(**(config or {}))
    config_path = os.path.join(game_dir, "config.lua")
    with open(config_path, "w") as f:
        f.write(config_lua)


BOAD_ADDITIONAL_OBSERVATION_SPACES = {
    "health": gymnasium.spaces.Box(0, 20, (1,), dtype=np.float32),
    "food": gymnasium.spaces.Box(0, 1000, (1,), dtype=np.float32),
    "water": gymnasium.spaces.Box(0, 1000, (1,), dtype=np.float32),
}


class MinetestGymnasium(gymnasium.Wrapper):
    def __init__(
        self,
        game: str,
        screen_size: int = 128,
        config: Optional[dict[str, Any]] = None,
        **kwargs,
    ):
        """Wrapper for the MineRL environments.

        Args:
            game (str): the minetest game to play.
            screen_size (int): the height of the pixels observations.
                Default to 128.
            config (dict): game specific configuration
                Default to None
        """

        self._observation_reward_systems: list[Callable[[WrapperObsType], float]] = []

        temp_dir = tempfile.mkdtemp(prefix="minetest_")
        # game_dir = os.path.join(
        #     os.environ["CONDA_PREFIX"], "share/minetest/games/", game
        # )
        game_dir = "/home/mick/astera/boad"
        if game == "boad" or game == "boad_local":
            _write_boad_config(config, game_dir)
            additional_observation_spaces = BOAD_ADDITIONAL_OBSERVATION_SPACES
            keyboard_action_keys = BOAD_KEYBOARD_ACTION_KEYS
            mouse_action_keys = BOAD_MOUSE_ACTION_KEYS
        else:
            additional_observation_spaces = {}
            keyboard_action_keys = KEYBOARD_ACTION_KEYS
            mouse_action_keys = list(MOUSE_ACTIONS.keys())

        env = minetest.minetest_env.MinetestEnv(
            display_size=(screen_size, screen_size),
            artifact_dir=os.path.join(temp_dir, "artifacts"),
            game_dir=game_dir,
            additional_observation_spaces=additional_observation_spaces,
            verbose_logging=True,
            **kwargs,
        )
        super().__init__(env)
        self._action_dicts = get_action_dicts(keyboard_action_keys, mouse_action_keys)
        self.action_space = gymnasium.spaces.Discrete(
            len(keyboard_action_keys) + len(mouse_action_keys)
        )

    def step(
        self, action_idx: WrapperActType
    ) -> tuple[WrapperObsType, SupportsFloat, bool, bool, dict[str, Any]]:
        action_dict = self._action_dicts[action_idx]
        cum_reward = 0
        for keyboard_action in action_dict["keys"]:
            action = {"keys": keyboard_action, "mouse": action_dict["mouse"]}
            print(action)
            obs, reward, terminated, truncated, info = self.env.step(action)
            cum_reward += reward
            cum_reward += sum(ors(obs) for ors in self._observation_reward_systems)
            if terminated or truncated:
                break
        return obs, cum_reward, terminated, truncated, info


import signal

import pygame

KEY_TO_ACTION_INDEX = dict(
    [
        (pygame.K_w, 0),
        (pygame.K_a, 1),
        (pygame.K_s, 2),
        (pygame.K_d, 3),
        (pygame.K_j, 4),
        (pygame.K_UP, 8),
        (pygame.K_DOWN, 7),
        (pygame.K_LEFT, 6),
        (pygame.K_RIGHT, 5),
    ]
)


def game_loop():
    running = True
    automatic = False
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                    break
                if event.key in KEY_TO_ACTION_INDEX:
                    state, reward, terminated, truncated, info = env.step(
                        KEY_TO_ACTION_INDEX[event.key]
                    )
                    health = state["health"].item()
                    food = state["food"].item()
                    water = state["water"].item()
                    print(
                        f"reward: {reward:3.1f}, health: {health:2.0f}, food: {food:4.0f}, water: {water:4.0f}"
                    )
                    if terminated:
                        running = False
                        break

            env.render()

        if automatic:
            state, reward, terminated, truncated, info = env.step(
                KEY_TO_ACTION_INDEX[pygame.K_LEFT]
            )
            health = state["health"].item()
            food = state["food"].item()
            water = state["water"].item()
            print(
                f"reward: {reward:3.1f}, health: {health:2.0f}, food: {food:4.0f}, water: {water:4.0f}"
            )
            if terminated:
                running = False
                break


def sigint_handler(_sig, _frame):
    env.close()
    pygame.quit()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, sigint_handler)

    pygame.init()

    config: BoadConfig = {
        "food_consumption_per_second": 100,
        "water_consumption_per_second": 100,
    }

    with MinetestGymnasium(
        "boad",
        screen_size=1024,
        config=config,
        render_mode="human",
        executable="/home/mick/astera/minetest/bin/minetest",
        log_to_stderr=True,
    ) as env:  # ,
        env.reset()
        game_loop()

    pygame.quit()
