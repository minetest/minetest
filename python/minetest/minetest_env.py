import asyncio
import contextlib
import datetime
import logging
import os
import shutil
import socket
import subprocess
import sys
import time
import uuid
from collections import namedtuple
from pathlib import Path
from typing import Any, Dict, Optional, Tuple

import capnp
import gymnasium as gym
import numpy as np

try:
    import pygame
except ImportError:
    pygame = None

remoteclient_capnp = capnp.load(
    os.path.join(os.path.dirname(__file__), "proto/remoteclient.capnp")
)

# Define default keys / buttons
KEY_MAP = [
    "forward",
    "left",
    "backward",
    "right",
    "jump",
    "sneak",
    "dig",
    "place",
]

INVERSE_KEY_MAP = {name: idx for idx, name in enumerate(KEY_MAP)}

DisplaySize = namedtuple("DisplaySize", ["height", "width"])

_still_loading_colors = (
    np.array([59, 59, 59], dtype=np.uint8),
    np.array([58, 58, 58], dtype=np.uint8),
    np.array([57, 57, 57], dtype=np.uint8),
)


def _is_loading(obs) -> bool:
    # Best way I've thought of to deal with this is to check if the image is
    # mostly the particular color.
    return any(
        (obs == color).all(axis=2).mean() > 0.5 for color in _still_loading_colors
    )


_TOGGLE_HUD_KEY = remoteclient_capnp.KeyPressType.Key.schema.enumerants["toggleHud"]
_TOGGLE_HUD_ACTION = {
    "keys": np.zeros(_TOGGLE_HUD_KEY + 1, dtype=bool),
    "mouse": [0, 0],
}
_TOGGLE_HUD_ACTION["keys"][_TOGGLE_HUD_KEY] = True


class MinetestEnv(gym.Env):
    metadata = {"render_modes": ["rgb_array", "human"]}

    def __init__(
        self,
        executable: Optional[os.PathLike] = "minetest",
        world_dir: Optional[os.PathLike] = None,
        artifact_dir: Optional[os.PathLike] = None,
        config_path: Optional[os.PathLike] = None,
        server_addr: Optional[str] = None,
        display_size: Tuple[int, int] = (600, 400),
        render_mode: str = "rgb_array",
        game_dir: Optional[os.PathLike] = None,
        fov: int = 72,
        base_seed: int = 0,
        world_seed: Optional[int] = None,
        config_dict: Dict[str, Any] = None,
        headless: bool = True,
        verbose_logging: bool = False,
        log_to_stderr: bool = False,
        additional_observation_spaces: Optional[Dict[str, gym.Space]] = None,
    ):
        self._prev_score = None
        if config_dict is None:
            config_dict = {}
        self.unique_env_id = str(uuid.uuid4())

        self.display_size = DisplaySize(*display_size)
        self.fov_y = fov
        self.fov_x = self.fov_y * self.display_size.width / self.display_size.height
        self.render_mode = render_mode
        if (not server_addr) and headless and sys.platform != "linux":
            raise ValueError(
                "headless mode only supported on linux. "
                "Note this may work with the latest upstream minetest which should support "
                "building with SDL2 on MacOS. You can try it if you care."
            )

        self.headless = headless
        self.game_dir = game_dir
        if not (server_addr or game_dir or world_dir):
            raise ValueError(
                "Either server_addr or game_dir or a world_dir must be provided!"
            )

        if render_mode == "human":
            self._start_pygame()

        # Define action and observation space
        self._configure_spaces(additional_observation_spaces)

        # Define Minetest paths
        self._set_artifact_dirs(
            artifact_dir, world_dir, config_path
        )  # Stores minetest artifacts and outputs

        self._logger = logging.getLogger(f"{__name__}_{self.unique_env_id}")
        self._logger.setLevel(logging.DEBUG)
        handler = logging.FileHandler(
            os.path.join(self.log_dir, f"env_{self.unique_env_id}.log")
        )
        handler.setLevel(logging.DEBUG)
        handler.setFormatter(
            logging.Formatter(
                "%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s",
                datefmt="%H:%M:%S",
            )
        )
        self._logger.addHandler(handler)
        if log_to_stderr:
            handler = logging.StreamHandler()
            handler.setLevel(logging.DEBUG)
            self._logger.addHandler(handler)

        self._logger.debug("logging to %s", self.log_dir)

        if executable:
            assert shutil.which(executable), f"executable not found: {executable}"
            self.executable = Path(executable)
        else:
            self.executable = None
            self._logger.debug(
                "executable not specified, will attempt to connect "
                "to running minetest instance."
            )
            if not server_addr:
                raise ValueError("if executable not specified, server_addr must be")

        if server_addr:
            # Force IPv4 and don't rely on DNS.
            self.socket_addr = server_addr.replace("localhost", "127.0.0.1")
            if ":" in self.socket_addr:
                addr_parts = self.socket_addr.split(":")
                assert len(addr_parts) == 2, server_addr
                self.socket_addr = (addr_parts[0], int(addr_parts[1]))
        else:
            assert (
                sys.platform == "linux"
            ), "abstract sockets require linux. you can set server_addr = host:port to use TCP"
            self.socket_addr = os.path.join(
                self.artifact_dir, f"minetest_{self.unique_env_id}_0.sock"
            )
        self.socket = None

        self.verbose_logging = verbose_logging

        self.capnp_client = None
        self.minetest_process: Optional[subprocess.Popen] = None

        # Env objects
        self.last_obs = None
        self.render_fig = None
        self.render_img = None

        # Seed the environment
        self.base_seed = base_seed
        self.world_seed = world_seed
        # If no world_seed is provided
        # seed the world with a random seed
        # generated by the RNG from base_seed
        self.reseed_on_reset = world_seed is None
        self.seed(self.base_seed)

        # Write minetest.conf
        self.config_dict = config_dict
        self._write_config()
        self._event_loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        self._kj_loop: contextlib.AbstractAsyncContextManager = None

    def _configure_spaces(self, additional_observation_spaces):
        # Define action and observation space
        self.max_mouse_move_x = self.display_size[0] // 2
        self.max_mouse_move_y = self.display_size[1] // 2
        self.action_space = gym.spaces.Dict(
            {
                "keys": gym.spaces.MultiBinary(len(KEY_MAP)),
                "mouse": gym.spaces.Box(
                    low=np.array([-self.max_mouse_move_x, -self.max_mouse_move_y]),
                    high=np.array([self.max_mouse_move_x, self.max_mouse_move_y]),
                    shape=(2,),
                    dtype=int,
                ),
            },
        )
        obs_d = {
            "image": gym.spaces.Box(
                0,
                255,
                shape=(self.display_size.height, self.display_size.width, 3),
                dtype=np.uint8,
            ),
        }
        if additional_observation_spaces:
            assert isinstance(additional_observation_spaces, dict)
            for key, space in additional_observation_spaces.items():
                assert isinstance(key, str)
                assert isinstance(space, gym.spaces.Box)
                assert space.shape is None or space.shape == (1,)
                obs_d[key] = space

        self.observation_space = gym.spaces.Dict(obs_d)

    def _set_artifact_dirs(self, artifact_dir, world_dir, config_path):
        if artifact_dir is None:
            self.artifact_dir = os.path.join(os.getcwd(), "artifacts")
        else:
            self.artifact_dir = artifact_dir

        if config_path is None:
            self.config_path = os.path.join(
                self.artifact_dir, f"{self.unique_env_id}.conf"
            )
        else:
            self.config_path = config_path

        if world_dir is None:
            self.reset_world = True
            self.world_dir = None
        else:
            self.reset_world = False
            self.world_dir = world_dir

        self.log_dir = os.path.join(self.artifact_dir, "log")
        self.media_cache_dir = os.path.join(self.artifact_dir, "media_cache")

        os.makedirs(self.log_dir, exist_ok=True)
        os.makedirs(self.media_cache_dir, exist_ok=True)

    async def _reset_capnp(self):
        if self.socket:
            self.socket.close()
            self.socket = None
        if isinstance(self.socket_addr, tuple):
            my_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        else:
            my_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        max_attempts = 100
        for _ in range(max_attempts):
            try:
                my_socket.connect(self.socket_addr)
                self.socket = my_socket
                break
            except (ConnectionRefusedError, FileNotFoundError):
                time.sleep(0.1)
        if not self.socket:
            raise RuntimeError(
                f"Failed to connect to minetest server at {self.socket_addr} after "
                f"{max_attempts} attempts. See logs in {self.log_dir}."
            )
        # NOTE: This is not how this function is meant to be used. They expect
        # all of the code that uses a connection to be in the same async function,
        # and that function wrapped in capnp.run().
        # We instead manage the kj_loop ourselves because
        # we don't want to force the caller to wrap all use of this class
        # in a single function.
        self._kj_loop = capnp.kj_loop()
        await self._kj_loop.__aenter__()
        connection = await capnp.AsyncIoStream.create_connection(sock=self.socket)
        self.capnp_client = (
            capnp.TwoPartyClient(connection)
            .bootstrap()
            .cast_as(remoteclient_capnp.Minetest)
        )

    def _reset_minetest(self):
        if not self.executable:
            logging.warning("Minetest executable not provided, reset is a no-op.")
            return None
        reset_timestamp = datetime.datetime.now().strftime("%m-%d-%Y,%H:%M:%S")
        log_path = os.path.join(
            self.log_dir,
            f"{{}}_{reset_timestamp}_{self.unique_env_id}.log",
        )

        if self.minetest_process:
            self.minetest_process.kill()
            self.minetest_process.communicate(timeout=15)

        self.minetest_process = self._start_minetest_client(
            log_path,
        )

    def _check_world_dir(self):
        if self.world_dir is None:
            raise RuntimeError(
                "World directory was not set. Please, provide a world directory "
                "in the constructor or seed the environment!",
            )

    def _delete_world(self):
        if self.world_dir and os.path.exists(self.world_dir):
            shutil.rmtree(self.world_dir, ignore_errors=True)

    def _check_config_path(self):
        if not self.config_path:
            raise RuntimeError(
                "Minetest config path was not set. Please, provide a config path "
                "in the constructor or seed the environment!",
            )

    def _delete_config(self):
        if os.path.exists(self.config_path):
            os.remove(self.config_path)

    def _write_config(self):
        config = dict(
            # Base config
            enable_sound=False,
            show_debug=False,
            enable_client_modding=True,
            csm_restriction_flags=0,
            enable_mod_channels=True,
            screen_w=self.display_size.width,
            screen_h=self.display_size.height,
            fov=self.fov_y,
            game_dir=self.game_dir,
            # Adapt HUD size to display size, based on (1024, 600) default
            hud_scaling=self.display_size[0] / 1024,
            # Attempt to improve performance. Impact unclear.
            server_map_save_interval=1000000,
            profiler_print_interval=0,
            active_block_range=2,
            abm_time_budget=0.01,
            abm_interval=0.1,
            active_block_mgmt_interval=4.0,
            server_unload_unused_data_timeout=1000000,
            client_unload_unused_data_timeout=1000000,
            full_block_send_enable_min_time_from_building=0.0,
            max_block_send_distance=100,
            max_block_generate_distance=100,
            num_emerge_threads=0,
            emergequeue_limit_total=1000000,
            emergequeue_limit_diskonly=1000000,
            emergequeue_limit_generate=1000000,
        )
        # Some games we carea bout currently use insecure lua features.
        config["secure.enable_security"] = False

        # Seed the map generator if not using a custom map
        if self.world_seed:
            config.update(fixed_map_seed=self.world_seed)
        # Update config from existing config file
        if os.path.exists(self.config_path):
            config.update(read_config_file(self.config_path))
        # Set from custom config dict
        config.update(self.config_dict)
        write_config_file(self.config_path, config)

    def _start_pygame(self):
        if pygame is None:
            raise ImportError(
                "pygame is required for rendering in human mode. "
                "Please install it: mamba install -c conda-forge pygame",
            )
        pygame.init()
        self.screen = pygame.display.set_mode(
            (self.display_size.width, self.display_size.height)
        )
        pygame.display.set_caption(f"Minetester - {self.unique_env_id}")

    def _display_pygame(self):
        # for some reason pydata expects the transposed image
        img_data = self.last_obs["image"].transpose((1, 0, 2))

        # Convert the numpy array to a Pygame Surface and display it
        img = pygame.surfarray.make_surface(img_data)
        self.screen.blit(img, (0, 0))
        pygame.display.update()

    def seed(self, seed: Optional[int] = None):
        self._np_random = np.random.RandomState(seed or 0)

    def _increment_socket_addr(self):
        if not isinstance(self.socket_addr, str):
            return
        self._logger.debug("incrementing socket")
        addr, ext = self.socket_addr.rsplit("_", 1)
        num = int(ext.split(".")[0])
        self.socket_addr = f"{addr}_{num+1}.sock"

    async def _async_reset(
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ):
        self.seed(seed=seed)
        if self.reset_world:
            self._delete_world()
            if self.reseed_on_reset:
                self.world_seed = self._np_random.randint(np.iinfo(np.int64).max)
        self._increment_socket_addr()
        self._reset_minetest()
        await self._reset_capnp()

        self._logger.debug("capnp_client.init()...")
        await self.capnp_client.init()
        empty_request = {}
        # Annoyingly sometimes minetest returns observations but has not finished loading.
        valid_obs_max_attempts = 100
        # And sometimes it goes valid -> invalid -> valid.
        min_num_valid_obs = 3
        valid_obs_seen = 0
        self._logger.debug("Waiting for first obs...")
        for attempt in range(valid_obs_max_attempts):
            step_promise = self.capnp_client.step(empty_request)
            if not attempt:
                time.sleep(1)
                # Check for crash triggered by first action
                if self.minetest_process and (self.minetest_process.poll() is not None):
                    raise RuntimeError(
                        "Minetest terminated during handshake! Return code: "
                        f"{self.minetest_process.returncode}, logs in {self.log_dir}",
                    )
            (
                obs,
                _,
                _,
            ) = deserialize_obs(await step_promise, self.observation_space)
            if _is_loading(obs["image"]):
                valid_obs_seen = 0
                self._logger.debug(
                    f"Still loading... {attempt}/{valid_obs_max_attempts}"
                )
                continue
            valid_obs_seen += 1
            if valid_obs_seen >= min_num_valid_obs:
                self._logger.debug(f"Received first obs: {obs['image'].shape}")
                self._logger.debug("Disabling HUD")
                obs, _, _, _, _ = await self._async_step(
                    _TOGGLE_HUD_ACTION,
                    _key_map={_TOGGLE_HUD_KEY: "toggleHud"},
                )
                self.last_obs = obs
                return obs, {}
        raise RuntimeError(
            f"Failed to get a valid observation after {valid_obs_max_attempts} attempts"
        )

    def reset(
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ):
        self._event_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._event_loop)
        result = self._event_loop.run_until_complete(
            self._async_reset(seed=seed, options=options)
        )
        return result

    async def _async_step(self, action: Dict[str, Any], _key_map=KEY_MAP):
        if self.minetest_process and self.minetest_process.poll() is not None:
            raise RuntimeError(
                "Minetest terminated! Return code: "
                f"{self.minetest_process.returncode}, logs in {self.log_dir}",
            )

        # Send action
        if isinstance(action["mouse"], np.ndarray):
            action["mouse"] = action["mouse"].tolist()
        step_request = self.capnp_client.step_request()
        serialize_action(action, step_request.action, key_map=_key_map)
        step_response = step_request.send()

        # TODO more robust check for whether a server/client is alive while receiving observations
        if self.minetest_process and self.minetest_process.poll() is not None:
            return self.last_obs, 0.0, True, False, {}

        next_obs, score, done = deserialize_obs(
            await step_response, self.observation_space
        )
        self.last_obs = next_obs

        if self.render_mode == "human":
            self._display_pygame()

        reward = 0 if self._prev_score is None else score - self._prev_score
        self._prev_score = score

        return next_obs, reward, done, False, {}

    def step(self, action: Dict[str, Any], _key_map=KEY_MAP):
        return self._event_loop.run_until_complete(
            self._async_step(action, _key_map=_key_map)
        )

    def render(self):
        if self.render_mode == "human":
            # rendering happens during step, as per gymnasium API
            return None
        if self.render_mode == "rgb_array":
            if self.last_obs is None:
                return None
            return self.last_obs["image"]
        raise ValueError(f"unsupported render mode {self.render_mode}")

    async def _async_close(self):
        if self.socket:
            self.socket.close()
        if self.minetest_process:
            self.minetest_process.kill()
        if self.reset_world:
            self._delete_world()
        if self._kj_loop:
            await self._kj_loop.__aexit__(None, None, None)

    def close(self):
        return self._event_loop.run_until_complete(self._async_close())

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _start_minetest_client(
        self,
        log_path: str,
    ):
        remote_input_addr = self.socket_addr
        if ":" not in remote_input_addr:
            remote_input_addr = f"unix:{remote_input_addr}"
        cmd = [
            self.executable,
            "--go",  # skip menu
            "--config",
            self.config_path,
            "--remote-input",
            remote_input_addr,
        ]
        if self.verbose_logging:
            cmd.append("--verbose")
        if self.world_dir is not None:
            cmd.extend(["--world", self.world_dir])

        client_env = os.environ.copy()
        # Avoids error in logs about missing XDG_RUNTIME_DIR
        client_env["XDG_RUNTIME_DIR"] = self.artifact_dir
        # disable vsync
        client_env["__GL_SYNC_TO_VBLANK"] = "0"
        client_env["vblank_mode"] = "0"

        if self.headless:
            # https://github.com/carla-simulator/carla/issues/225
            client_env["SDL_VIDEODRIVER"] = "offscreen"

        stdout_file = log_path.format("client_stdout")
        stderr_file = log_path.format("client_stderr")
        with open(stdout_file, "w") as out, open(stderr_file, "w") as err:
            out.write(
                f"Starting client with command: {' '.join(str(x) for x in cmd)}\n"
            )
            out.write(f"Client environment: {client_env}\n")
            minetest_process = subprocess.Popen(
                cmd, stdout=out, stderr=err, env=client_env
            )
            out.write(f"Client started with pid {minetest_process.pid}\n")
        self._logger.debug(f"Client started with pid {minetest_process.pid}")
        return minetest_process


def deserialize_obs(
    step_response, observation_space: gym.spaces.Dict
) -> Tuple[Dict[str, Any], float, bool]:
    img = step_response.observation.image
    img_data = np.frombuffer(img.data, dtype=np.uint8).reshape(
        (img.height, img.width, 3)
    )
    obs = {
        "image": img_data,
    }
    for aux_item in step_response.observation.aux.entries:
        assert aux_item.key in observation_space.spaces, (
            f"Unknown observation space: {aux_item.key}. "
            "Please set additional_observation_spaces when constructing."
        )
        obs[aux_item.key] = np.array([aux_item.value], dtype=np.float32)
    for key in observation_space.spaces:
        if key not in obs:
            obs[key] = np.zeros((1,), dtype=np.float32)
    reward = step_response.observation.reward
    done = step_response.observation.done
    return obs, reward, done


def serialize_action(action: Dict[str, Any], action_msg, key_map=KEY_MAP):
    action_msg.mouseDx = action["mouse"][0]
    action_msg.mouseDy = action["mouse"][1]

    keyEvents = action_msg.init("keyEvents", action["keys"].sum())
    setIdx = 0
    for idx, pressed in enumerate(action["keys"]):
        if pressed:
            keyEvents[setIdx] = key_map[idx]
            setIdx += 1


# Python impl of Minetest's config file parser in main.cpp:read_config_file
# https://github.com/Astera-org/minetest/blob/b18fb18138c3ec658d9fef9fc84b085a7f4f9a01/src/main.cpp#L731
def read_config_file(file_path):
    config = {}
    with open(file_path) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                key, value = line.split("=", 1)
                key = key.strip()
                value = value.strip()
                if value.isdigit():
                    value = int(value)
                elif value.replace(".", "", 1).isdigit():
                    value = float(value)
                elif value.lower() == "true":
                    value = True
                elif value.lower() == "false":
                    value = False
                config[key] = value
    return config


def write_config_file(file_path, config):
    with open(file_path, "w") as f:
        for key, value in config.items():
            f.write(f"{key} = {value}\n")
