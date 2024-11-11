import asyncio
import contextlib
import ctypes
import ctypes.util
import datetime
import logging
import os
import shutil
import signal
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

# np.bool8 is removed in newer numpy but some deps still use it.
np.bool8 = np.bool_

try:
    import pygame
except ImportError:
    pygame = None

remoteclient_capnp = capnp.load(
    os.path.join(os.path.dirname(__file__), "proto/remoteclient.capnp")
)

# See `python/minetest/proto/remoteclient.capnp`. {'forward': 0, 'backward': 1,
# 'left': 2, 'right': 3, 'jump': 4, 'aux1': 5, 'sneak': 6, 'autoforward': 7,
# 'dig': 8, 'place': 9, 'esc': 10, 'drop': 11, 'inventory': 12, 'chat': 13,
# 'cmd': 14, 'cmdLocal': 15, 'console': 16, 'minimap': 17, 'freemove': 18,
# 'pitchmove': 19, 'fastmove': 20, 'noclip': 21, 'hotbarPrev': 22, 'hotbarNext':
# 23, 'mute': 24, 'incVolume': 25, 'decVolume': 26, 'cinematic': 27,
# 'screenshot': 28, 'toggleBlockBounds': 29, 'toggleHud': 30, 'toggleChat': 31,
# 'toggleFog': 32, 'toggleUpdateCamera': 33, 'toggleDebug': 34,
# 'toggleProfiler': 35, 'cameraMode': 36, 'increaseViewingRange': 37,
# 'decreaseViewingRange': 38, 'rangeselect': 39, 'zoom': 40, 'quicktuneNext':
# 41, 'quicktunePrev': 42, 'quicktuneInc': 43, 'quicktuneDec': 44, 'slot1': 45,
# 'slot2': 46, 'slot3': 47, 'slot4': 48, 'slot5': 49, 'slot6': 50, 'slot7': 51,
# 'slot8': 52, 'slot9': 53, 'slot10': 54, 'slot11': 55, 'slot12': 56, 'slot13':
# 57, 'slot14': 58, 'slot15': 59, 'slot16': 60, 'slot17': 61, 'slot18': 62,
# 'slot19': 63, 'slot20': 64, 'slot21': 65, 'slot22': 66, 'slot23': 67,
# 'slot24': 68, 'slot25': 69, 'slot26': 70, 'slot27': 71, 'slot28': 72,
# 'slot29': 73, 'slot30': 74, 'slot31': 75, 'slot32': 76, 'middle': 77, 'ctrl':
# 78, 'internalEnumCount': 79}
KEY_NAMES = remoteclient_capnp.KeyPressType.Key.schema.enumerants.keys()
KEY_INDEX_TO_NAME = {index: name for index, name in enumerate(KEY_NAMES)}
KEY_NAME_TO_INDEX = {name: index for index, name in enumerate(KEY_NAMES)}

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


def action_noop():
    return {
        "keys": np.zeros(len(KEY_NAMES), dtype=np.bool_),
        "mouse": np.zeros(2, dtype=np.int32),
    }


def action_from_key_name(name):
    action = action_noop()
    action["keys"][KEY_NAME_TO_INDEX[name]] = True
    return action


def _set_process_death_signal():
    """When the parent thread exits, kill the child process.

    Only works on Linux.
    """
    libc_path = ctypes.util.find_library("c")
    if not libc_path:
        return
    libc = ctypes.CDLL(libc_path, use_errno=True)
    if not hasattr(libc, "prctl"):
        return
    # https://github.com/torvalds/linux/blob/0cac73eb3875f6ecb6105e533218dba1868d04c9/include/uapi/linux/prctl.h#L9
    PR_SET_PDEATHSIG = 1
    libc.prctl(PR_SET_PDEATHSIG, signal.SIGKILL)


def _free_port():
    # Have the OS return a free port, then immediately close the socket.
    # Not guaranteed to be free, but should be good enough
    with socket.socket() as s:
        s.bind(("", 0))
        return s.getsockname()[1]


class MinetestEnv(gym.Env):
    metadata = {"render_modes": ["rgb_array", "human"]}

    def __init__(
        self,
        executable: Optional[os.PathLike] = "minetest",
        world_dir: Optional[os.PathLike] = None,
        artifact_dir: Optional[os.PathLike] = None,
        config_path: Optional[os.PathLike] = None,
        server_addr: Optional[str] = None,
        render_size: Tuple[int, int] = (600, 400),
        display_size: Optional[Tuple[int, int]] = None,
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
        remote_input_handler_time_step: float = 0.125,
        hide_hud: bool = True,
    ):
        self._prev_score = None
        if config_dict is None:
            config_dict = {}
        self.unique_env_id = str(uuid.uuid4())

        self.render_size = DisplaySize(*(render_size))
        self.display_size = DisplaySize(
            *(render_size if display_size is None else display_size)
        )
        self.remote_input_handler_time_step = remote_input_handler_time_step
        self._hide_hud = hide_hud
        self.fov_y = fov
        self.fov_x = self.fov_y * self.render_size.width / self.render_size.height
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
            if sys.platform == "linux":
                self.socket_addr = os.path.join(
                    self.artifact_dir, f"minetest_{self.unique_env_id}_0.sock"
                )
            elif sys.platform == "darwin":
                # mac doesn't support abstract unix sockets, so use TCP
                self.socket_addr = ("127.0.0.1", _free_port())
            else:
                raise ValueError(f"unsupported platform {sys.platform}. ")
        self.socket = None

        self.verbose_logging = verbose_logging

        self.capnp_client = None
        self.minetest_process: Optional[subprocess.Popen] = None
        self._minetest_stderr_path: Optional[os.PathLike] = None

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
        self._event_loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        self._kj_loop: contextlib.AbstractAsyncContextManager = None

    def _configure_spaces(self, additional_observation_spaces):
        # Define action and observation space
        self.max_mouse_move_x = self.render_size[0] // 2
        self.max_mouse_move_y = self.render_size[1] // 2
        self.action_space = gym.spaces.Dict(
            {
                "keys": gym.spaces.MultiBinary(len(KEY_NAMES)),
                "mouse": gym.spaces.Box(
                    low=np.array([-self.max_mouse_move_x, -self.max_mouse_move_y]),
                    high=np.array([self.max_mouse_move_x, self.max_mouse_move_y]),
                    shape=(2,),
                    dtype=np.int32,
                ),
            },
        )
        obs_d = {
            "image": gym.spaces.Box(
                0,
                255,
                shape=(self.render_size.height, self.render_size.width, 3),
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
        max_attempts = 100
        for _ in range(max_attempts):
            if isinstance(self.socket_addr, tuple):
                my_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            else:
                my_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
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

        self._write_config()
        self.minetest_process = self._start_minetest(
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
            screen_w=self.render_size.width,
            screen_h=self.render_size.height,
            fov=self.fov_y,
            # Adapt HUD size to display size, based on (1024, 600) default
            hud_scaling=self.render_size[0] / 1024,
            # Ensure server doesn't advance if client hasn't called step().
            server_step_wait_for_all_clients=True,
            # How much time is simulated in between each environment step.
            remote_input_handler_time_step=self.remote_input_handler_time_step,
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
        if self.game_dir:
            config["game_dir"] = self.game_dir
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
                "pygame is required for rendering in human mode. Please install it.",
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
        self.screen.blit(pygame.transform.scale(img, self.display_size), (0, 0))
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

    def _poll_minetest_process(self):
        # a function so we can override it in the unit test
        return self.minetest_process.poll()

    def _log_minetest_stderr_tail(self):
        # log the last 2k bytes of stderr
        if not self._minetest_stderr_path:
            return
        with open(self._minetest_stderr_path) as f:
            f.seek(0, os.SEEK_END)
            end = f.tell()
            f.seek(max(0, end - 2048), os.SEEK_SET)
            self._logger.error("Minetest stderr tail:")
            for line in f:
                self._logger.error(line.strip())

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
                if self.minetest_process and (
                    self._poll_minetest_process() is not None
                ):
                    self._log_minetest_stderr_tail()
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
                self._logger.debug(
                    f"Received first obs: {obs['image'].shape}, Disabling HUD"
                )
                obs, _, _, _, _ = await self._async_step(
                    action_from_key_name("toggleHud")
                    if self._hide_hud
                    else action_noop()
                )
                self.last_obs = obs
                return obs, {}
        raise RuntimeError(
            f"Failed to get a valid observation after {valid_obs_max_attempts} attempts"
        )

    def _run_on_event_loop(self, coro, timeout=10000):
        try:
            try:
                return self._event_loop.run_until_complete(
                    asyncio.wait_for(coro, timeout=timeout)
                )
            except:
                # Ensure we close. If clients don't do it, bad stuff happens:
                # - minetest process leaks
                # - the kj loop  stays associated with this thread, making
                #   it impossible to create another MinetestEnv in this thread
                if coro.__qualname__ != "MinetestEnv._async_close":
                    self._event_loop.run_until_complete(self._async_close())
                raise
        # catch KjException and raise as a different type since cannot be pickled,
        # which leads to horribly misleading error messages when using AsyncVectorEnv
        except capnp.lib.capnp.KjException as e:
            raise RuntimeError(
                f"minetest capnp error: {e}",
            ).with_traceback(e.__traceback__) from None

    def reset(
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ):
        self.close()  # ensure processes, event loops are cleaned up.
        self._event_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self._event_loop)
        return self._run_on_event_loop(
            self._async_reset(seed=seed, options=options), timeout=10000
        )

    async def _async_step(self, action: Dict[str, Any]):
        if self.minetest_process and self.minetest_process.poll() is not None:
            raise RuntimeError(
                "Minetest terminated! Return code: "
                f"{self.minetest_process.returncode}, logs in {self.log_dir}",
            )

        # Send action
        if isinstance(action["mouse"], np.ndarray):
            action["mouse"] = action["mouse"].tolist()
        step_request = self.capnp_client.step_request()
        serialize_action(action, step_request.action)
        step_response = step_request.send()

        # TODO more robust check for whether minetest is alive while receiving observations
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

    def step(self, action: Dict[str, Any]):
        return self._run_on_event_loop(self._async_step(action))

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
        return self._run_on_event_loop(self._async_close())

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _start_minetest(
        self,
        log_path: str,
    ):
        remote_input_addr = self.socket_addr
        if isinstance(remote_input_addr, tuple):
            remote_input_addr = f"{remote_input_addr[0]}:{remote_input_addr[1]}"
        elif ":" not in remote_input_addr:
            remote_input_addr = f"unix:{remote_input_addr}"
        cmd = []
        # vglrun is the only way we've gotten accelerated OpenGL in containers.
        if self.headless and shutil.which("vglrun"):
            cmd.append("vglrun")
        cmd.extend(
            [
                self.executable,
                "--go",  # skip menu
                "--config",
                self.config_path,
                "--remote-input",
                remote_input_addr,
            ]
        )
        if self.verbose_logging:
            cmd.append("--verbose")
        if self.world_dir is not None:
            cmd.extend(["--world", self.world_dir])

        process_env = os.environ.copy()
        # Avoids error in logs about missing XDG_RUNTIME_DIR
        process_env["XDG_RUNTIME_DIR"] = self.artifact_dir
        # disable vsync
        process_env["__GL_SYNC_TO_VBLANK"] = "0"
        process_env["vblank_mode"] = "0"
        process_env["MINETEST_USER_PATH"] = self.artifact_dir

        if self.headless:
            # https://github.com/carla-simulator/carla/issues/225
            process_env["SDL_VIDEODRIVER"] = "offscreen"

        stdout_file = log_path.format("minetest_stdout")
        self._minetest_stderr_path = log_path.format("minetest_stderr")
        with open(stdout_file, "w") as out, open(
            self._minetest_stderr_path, "w"
        ) as err:
            out.write(
                f"Starting minetest with command: {' '.join(str(x) for x in cmd)}\n"
            )
            out.write(f"minetest environment: {process_env}\n")
            minetest_process = subprocess.Popen(
                cmd,
                stdout=out,
                stderr=err,
                env=process_env,
                preexec_fn=_set_process_death_signal,
            )
            out.write(f"minetest started with pid {minetest_process.pid}\n")
        self._logger.debug(f"minetest started with pid {minetest_process.pid}")
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


def _lazy_nonzero(arr):
    return [index for index, value in enumerate(arr) if value]


def serialize_action(action: Dict[str, Any], action_msg) -> None:
    mouse = np.array(action["mouse"])
    action_msg.mouseDx = mouse[0].item()
    action_msg.mouseDy = mouse[1].item()

    num_keys_pressed = np.count_nonzero(action["keys"])
    key_events = action_msg.init("keyEvents", num_keys_pressed)
    for list_index, key_index in enumerate(_lazy_nonzero(action["keys"])):
        key_events[list_index] = KEY_INDEX_TO_NAME[key_index]


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


def _value_to_config_string(value):
    if isinstance(value, bool):
        return "true" if value else "false"
    return value


def write_config_file(file_path, config):
    with open(file_path, "w") as f:
        for key, value in config.items():
            f.write(f"{key} = {_value_to_config_string(value)}\n")
