#pragma once

#include <memory>
#include <vector>

namespace api
{
namespace server
{
class Entity;
class Environment;
class Global;
class Inventory;
class Item;
class ModChannels;
class Node;
class NodeMeta;
class Player;
}
}

extern std::vector<std::shared_ptr<api::server::Entity>> get_minetest_entity_apis();
extern std::vector<std::shared_ptr<api::server::Environment>>
get_minetest_environment_apis();
extern std::vector<std::shared_ptr<api::server::Global>> get_minetest_global_apis();
extern std::vector<std::shared_ptr<api::server::Inventory>> get_minetest_inventory_apis();
extern std::vector<std::shared_ptr<api::server::Item>> get_minetest_item_apis();
extern std::vector<std::shared_ptr<api::server::ModChannels>>
get_minetest_modchannels_apis();
extern std::vector<std::shared_ptr<api::server::Node>> get_minetest_node_apis();
extern std::vector<std::shared_ptr<api::server::NodeMeta>> get_minetest_nodemeta_apis();
extern std::vector<std::shared_ptr<api::server::Player>> get_minetest_player_apis();