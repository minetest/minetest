from gymnasium.envs.registration import register

from minetest.minetest_env import MinetestEnv

register(
    id="minetest-v0",
    entry_point="minetest:MinetestEnv",
)

__all__ = ["MinetestEnv"]
