from gymnasium.envs.registration import register

from minetester.minetest_env import MinetestEnv

register(
    id="minetest-v0",
    entry_point="minetester:MinetestEnv",
)

__all__ = ["MinetestEnv"]
