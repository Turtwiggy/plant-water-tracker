
// my libs
#include "serialize.hpp"
using namespace plantproject;

// other libs
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

// std libs
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace plantproject {

struct Plant
{
  int version = 0;
  std::string key;
  std::string description;
  // std::vector<something> watered_at;
};

using json = nlohmann::json;

void
to_json(json& j, const Plant& p)
{
  std::cout << "to_json: " << p.version << std::endl;
  switch (p.version) {
    case 2:
      j = json{
        { "version", p.version },
        { "key", p.key },
        { "description", p.description },
      };
      break;
    case 1: // DEPRECATED
      j = json{
        { "version", p.version },
        { "key", p.key },
      };
      break;
  }
};

void
from_json(const json& j, Plant& p)
{
  int version = j.at("version").get_to(p.version);
  std::cout << "from_json: " << version << std::endl;
  switch (version) {
    case 2:
      j.at("key").get_to(p.key);
      j.at("description").get_to(p.description);
      break;
    case 1:
      j.at("key").get_to(p.key);
      break;
  }
};

void
save(const entt::registry& registry, std::string path)
{
  std::cout << "saving..." << std::endl;

  // convert entt registry to string(or bson)
  NJSONOutputArchive json_archive;
  entt::basic_snapshot{ registry }.entities(json_archive).component<Plant>(json_archive);
  json_archive.close();

  std::string data = json_archive.as_string();

  // save to disk
  std::ofstream fout(path);
  fout << data.c_str();
};

void
load(entt::registry& registry, std::string path)
{
  std::cout << "loading..." << std::endl;

  registry.clear();

  // load from disk
  std::ifstream t(path);
  std::stringstream buffer;
  buffer << t.rdbuf();
  const std::string data = buffer.str();

  // convert string (or bson) to entt registry
  auto& registry_to_load_in_to = registry;

  NJSONInputArchive json_in(data);
  entt::basic_snapshot_loader{ registry_to_load_in_to }.entities(json_in).component<Plant>(json_in);
};

void
load_if_exists(entt::registry& registry, std::string path)
{
  std::ifstream file(path.c_str());
  if (file)
    load(registry, path);
}

}; // namespace plantproject

//
// QUERIES
//

int
num_plants_available(const entt::registry& registry)
{
  int num = 0;
  auto view = registry.view<const Plant>();
  view.each([&num](const auto& plant) { num++; });
  return num;
};

std::optional<entt::entity>
get_plant(const entt::registry& registry, std::string name)
{
  auto view = registry.view<const Plant>();
  view.each([&name](auto entity, const auto& plant) {
    if (plant.key == name) {
      return entity;
    }
  });
  return std::nullopt;
}

std::optional<std::string>
get_description_of_plant(const entt::registry& registry, entt::entity entity)
{
  if (registry.all_of<Plant>(entity)) {
    const auto& p = registry.get<Plant>(entity);
    return p.description;
  }
  return std::nullopt;
}

//
// CRUD
//

void
print_plant(const Plant& p)
{
  std::cout << "- " << p.key << std::endl;
};

void
add_plant_impl(entt::registry& registry, const std::string name, const std::string description)
{
  Plant p;
  p.version = 2;
  p.key = name;
  p.description = "(Empty)";

  auto entity = registry.create();
  registry.emplace<Plant>(entity, p);
}

void
list_plants_impl(const entt::registry& registry)
{
  auto view = registry.view<const Plant>();
  view.each([](const auto& plant) { print_plant(plant); });
}

void
list_plant_info_impl(const entt::registry& registry, const std::string name)
{
  const auto plant = get_plant(registry, name);
  if (plant.has_value()) {
    const auto opt = get_description_of_plant(registry, plant.value());
    if (opt.has_value())
      std::cout << "Description: " << opt.value() << std::endl;
  } else {
    std::cout << "Description: N/A" << std::endl;
  }
}

void
delete_plant_impl(entt::registry& registry, std::string name)
{
  std::vector<entt::entity> entities;

  auto view = registry.view<const Plant>();
  view.each([&entities, &name](auto entity, const auto& plant) {
    if (plant.key == name)
      entities.push_back(entity);
  });

  for (auto& e : entities)
    registry.destroy(e);
}

int
main()
{
  entt::registry registry;
  std::string cmd_add("add"); // usage: "add steve"
  std::string cmd_list("list");
  std::string cmd_list_param_all_plants("-a"); // usage: "list -a" list all plants
  std::string cmd_list_param_all_cmds("-c");   // usage: "list -c" list all commands
  std::string cmd_info("info");                // usage" "info steve"
  std::string cmd_save("save");
  std::string cmd_load("load");
  std::string cmd_quit("quit");
  std::string cmd_delete("delete"); // usage: "delete steve"
  std::string filepath("./plants.txt");
  std::string input;

  load_if_exists(registry, filepath);
  std::cout << "~~ Welcome to PlantIt ~~" << std::endl;
  std::cout << "You have (" << num_plants_available(registry) << ") plants" << std::endl;

  while (true) {
    // Get input as string, then convert to tokens
    std::getline(std::cin, input);
    std::vector<std::string> tokens;
    std::string buf;
    std::stringstream ss(input);
    while (ss >> buf)
      tokens.push_back(buf);
    std::cout << "You typed: " << input << std::endl;

    bool cmd_is_add = false;
    bool cmd_is_list = false;
    bool cmd_is_delete = false;
    bool cmd_is_info = false;

    for (int i = 0; i < tokens.size(); i++) {
      std::string token = tokens[i];
      // std::cout << "processing token: " << token << std::endl;

      // first token
      if (i == 0) {
        cmd_is_add |= token.compare(cmd_add) == 0;
        cmd_is_delete |= token.compare(cmd_delete) == 0;
        cmd_is_list |= token.compare(cmd_list) == 0;
        cmd_is_info |= token.compare(cmd_info) == 0;
      }

      // second token
      if (i == 1) {
        if (cmd_is_add) {
          add_plant_impl(registry, token, std::string("")); // hmm, no description?
          save(registry, filepath);                         // save after adding a plant
        }
        if (cmd_is_delete) {
          delete_plant_impl(registry, token);
          save(registry, filepath); // save after deleting a plant
        }
        if (cmd_is_list && token.compare(cmd_list_param_all_plants) == 0) {
          list_plants_impl(registry);
        }
        if (cmd_is_info) {
          list_plant_info_impl(registry, token);
        }
      }
    }

    // process line for "save" command
    if (input.compare(cmd_save) == 0) {
      std::cout << "saving..." << std::endl;
      save(registry, filepath);
    }

    // process line for "load" command
    if (input.compare(cmd_load) == 0) {
      std::cout << "loading.." << std::endl;
      load_if_exists(registry, filepath);
    }

    if (input.compare(cmd_quit) == 0)
      return 0;
  }

  return 0;
}