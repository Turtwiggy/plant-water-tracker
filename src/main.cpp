
// my libs
#include "serialize.hpp"
using namespace plantproject;

// other libs
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

// std libs
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using MyClock = std::chrono::system_clock;
using MyTimepoint = MyClock::time_point;
using MyDuration = MyClock::duration;

namespace plantproject {

struct Plant
{
  int version = 1;
  std::string key;
  std::string description;
  std::vector<uint64_t> watered_at;
};

using json = nlohmann::json;

void
to_json(json& j, const Plant& p)
{
  int V = Plant().version;
  // std::cout << "to_json: " << V << std::endl;

  json jvec(p.watered_at);
  j = json{
    { "version", V },
    { "key", p.key },
    { "description", p.description },
    { "watered_at", jvec },
  };
};

void
from_json(const json& j, Plant& p)
{
  int version = j.at("version").get_to(p.version);
  // std::cout << "from_json: " << version << std::endl;

  switch (version) {
    case 1:
      j.at("key").get_to(p.key);
      j.at("description").get_to(p.description);
      j.at("watered_at").get_to(p.watered_at);
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
};

}; // namespace plantproject

//
// QUERIES
//

int
num_plants_available(const entt::registry& registry)
{
  auto view = registry.view<const Plant>();
  return view.size();
};

std::optional<entt::entity>
get_plant(const entt::registry& registry, std::string name)
{
  std::optional<entt::entity> found = std::nullopt;

  auto view = registry.view<const Plant>();
  view.each([&found, &name](auto entity, const auto& plant) {
    if (plant.key == name) {
      found = entity;
    }
  });

  return found;
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
// Plant CRUD
//

void
print_plant_water_status(const Plant& p)
{
  int show_last_x_waters = 5;
  for (int i = p.watered_at.size() - 1; i >= 0; i--) {
    const auto epoch_time = p.watered_at[i];
    const MyDuration d(epoch_time);
    const MyTimepoint tp(d);
    const std::time_t time = MyClock::to_time_t(tp);
    auto* tm_gmt = std::gmtime(&time);

    std::cout << "Watered at: " << tm_gmt->tm_hour << ":" << tm_gmt->tm_min << ":" << tm_gmt->tm_sec << " GMT "
              << tm_gmt->tm_mday << "/" << (1 + tm_gmt->tm_mon) << "/" << (1900 + tm_gmt->tm_year) << std::endl;

    show_last_x_waters -= 1;
    if (show_last_x_waters == 0)
      break;
  }

  if (p.watered_at.size() > 0)
    std::cout << "Watered: " << p.watered_at.size() << " times" << std::endl;
  else
    std::cout << "Plant has never been watered!" << std::endl;
}

void
print_plant(const Plant& p)
{
  std::cout << "~~~~~ " << p.key << " ~~~~~" << std::endl;
  std::cout << "Description: " << p.description << std::endl;

  print_plant_water_status(p);

  std::cout << "~~~~~~~~~~~~" << std::endl;
};

void
add_plant_impl(entt::registry& registry, const std::string name, const std::string description)
{
  Plant p;
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
  const auto& plant = get_plant(registry, name);
  if (plant.has_value()) {
    const auto& p = registry.get<Plant>(plant.value());
    print_plant(p);
    return;
  }

  std::cout << "No info for plant: " << name << std::endl;
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

//
// Watering-status CRUD
//

void
add_water_to_plant_impl(entt::registry& registry, std::string name)
{
  const auto& plant = get_plant(registry, name);
  if (plant.has_value()) {

    MyTimepoint now = MyClock::now();
    const auto ts = now.time_since_epoch();

    auto& p = registry.get<Plant>(plant.value());
    p.watered_at.push_back(ts.count());
    return;
  }

  std::cout << "No info for plant: " << name << std::endl;
};

int
main()
{
  entt::registry registry;

  // plant CRUD
  std::string cmd_list("list");
  std::string cmd_list_param_all_plants("-a"); // usage: "list -a" list all plants
  std::string cmd_add("add");                  // usage: "add steve"
  std::string cmd_delete("delete");            // usage: "delete steve"
  std::string cmd_info("info");                // usage" "info steve"
  std::string cmd_water("water");              // usage" "water steve"

  // cli-app stuff
  std::string cmd_save("save");
  std::string cmd_load("load");
  std::string cmd_quit("quit");
  std::string filepath("plants.json");
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
    bool cmd_is_water = false;

    for (int i = 0; i < tokens.size(); i++) {
      std::string token = tokens[i];
      // std::cout << "processing token: " << token << std::endl;

      // first token
      if (i == 0) {
        cmd_is_add |= token.compare(cmd_add) == 0;
        cmd_is_delete |= token.compare(cmd_delete) == 0;
        cmd_is_list |= token.compare(cmd_list) == 0;
        cmd_is_info |= token.compare(cmd_info) == 0;
        cmd_is_water |= token.compare(cmd_water) == 0;
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
        if (cmd_is_water) {
          add_water_to_plant_impl(registry, token);
          save(registry, filepath); // save after watering a plant
        }
      }
    }

    if (input.compare(cmd_save) == 0) {
      std::cout << "saving..." << std::endl;
      save(registry, filepath);
    }

    if (input.compare(cmd_load) == 0) {
      std::cout << "loading.." << std::endl;
      load_if_exists(registry, filepath);
    }

    if (input.compare(cmd_quit) == 0)
      return 0;
  }

  return 0;
}