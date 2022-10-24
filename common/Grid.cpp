#include "Grid.h"

#include <algorithm>
#include <map>

#include "utility.h"

using namespace std;

bool Field::can_step_here() const { return !is_bush && grenades.empty() && !bat.has_value(); }

bool Field::illuminate(queue<Grenade>& explodes_queue, int started_by_vampire) {
  if (!has_light) {
    // Here for the first time
    for (const auto& grenade : grenades) {
      explodes_queue.push(grenade);
      illuminated_by_vampire.insert(grenade.vampire_id);
    }
  }
  has_light = true;
  illuminated_by_vampire.insert(started_by_vampire);
  return !is_bush && !bat.has_value();
}

void Field::reset_light() {
  has_light = false;
  illuminated_by_vampire.clear();
}

void Field::evaluate_light(Grid& grid) {
  if (!has_light) return;
  if (is_bush) {
    has_light = false;
  }
  grenades.clear();
  if (bat.has_value()) {
    for (int vampire_id : illuminated_by_vampire) {
      grid.scores[ScoreType::BAT][vampire_id] += 12.0 / illuminated_by_vampire.size();
    }
    --bat.value().density;
    if (bat.value().density == 0) {
      bat.reset();
    }
    has_light = false;
  }
  for (Vampire& vampire : vampires) {
    if (vampire.invulnerable) continue;
    --vampire.health;
    double score = vampire.health ? 48 : 144;
    for (int vampire_id : illuminated_by_vampire) {
      if (vampire_id != vampire.id)
        grid.scores[ScoreType::ATTACK][vampire_id] += score / illuminated_by_vampire.size();
      else
        grid.scores[ScoreType::MINUS][vampire_id] -= score / illuminated_by_vampire.size();
    }
    vampire.invulnerable = 3;
  }
  for (auto vampire_it = vampires.begin(); vampire_it != vampires.end();) {
    if (vampire_it->health == 0) {
      vampire_it = vampires.erase(vampire_it);
    } else {
      ++vampire_it;
    }
  }
}

void Field::step_powerup(Grid& grid) {
  for (Vampire& vampire : vampires) {
    grid.grenades_before_step[vampire.id] = vampire.grenades;
    grid.shoes_before_step[vampire.id] = vampire.shoes;
    if (vampire.shoes > 0) --vampire.shoes;
  }

  if (!powerup.has_value()) return;

  if (powerup.value().ticks < 0) {
    ++powerup.value().ticks;
    if (powerup.value().ticks < 0) return;
    // TODO: in the bot it might cause that the ticks of the powerup change by more than one
    // when we get the "real" value for the first time. 10 seems to be a good pessimistic value
    // observing the logs.
    powerup.value().ticks = grid.server ? grid.random_generator.get_powerup_positive_ticks() : 10;
  }

  if (powerup.value().ticks > 0) --powerup.value().ticks;

  bool taken = false;
  for (Vampire& vampire : vampires) {
    ++grid.powerup_protection[vampire.id];
    if (grid.powerup_protection[vampire.id] < powerup.value().protect || vampire.invulnerable) continue;
    taken = true;
    grid.scores[ScoreType::POWERUP][vampire.id] += 48.0;
    if (powerup.value().type == PowerupType::TOMATO && vampire.health < 3) {
      ++vampire.health;
    } else if (powerup.value().type == PowerupType::GRENADE) {
      ++vampire.grenades;
    } else if (powerup.value().type == PowerupType::BATTERY) {
      ++vampire.range;
    } else if (powerup.value().type == PowerupType::SHOE) {
      vampire.shoes += grid.fields[0].size() * 2;
    }
  }

  if (taken || (vampires.empty() && powerup.value().ticks == 0)) {
    powerup.reset();
  }
}

string XXX_with_number(int n) {
  string ret = "X" + (n == 0 ? "" : to_string(n));
  while (ret.length() < 5) ret.append("X");
  return ret;
}

void Field::print_vampire(std::ostream& os) const {
  if (is_bush)
    os << "XXXXX ";
  else if (bat.has_value())
    os << " B " << bat.value().density << "  ";
  else if (vampires.size() > 0) {
    os << 'V';
    for (const auto& v : vampires) os << v.id;
    os << string(5 - vampires.size(), ' ');
  } else
    os << "      ";
}

void Field::print_grenade(std::ostream& os, int rowIdx) const {
  if (is_bush)
    os << XXX_with_number(rowIdx) << " ";
  else if (!grenades.empty()) {
    int min_tick = grenades[0].tick;
    string vampire_ids;
    for (const auto& grenade : grenades) {
      min_tick = min(min_tick, grenade.tick);
      vampire_ids += to_string(grenade.vampire_id);
    }
    os << 'G' << vampire_ids << ' ' << min_tick << string(max(0, 3 - (int)vampire_ids.length()), ' ');
  } else if (has_light)
    os << "  L   ";
  else
    os << "      ";
}

void Field::print_powerup(std::ostream& os, int colIdx) const {
  if (is_bush)
    os << XXX_with_number(colIdx) << " ";
  else if (powerup.has_value()) {
    os << 'P' << powerup.value().protect << "TGBS"[(int)powerup.value().type] << powerup.value().ticks;
    if (0 <= powerup.value().ticks && powerup.value().ticks <= 9)
      os << "  ";
    else
      os << ' ';
  } else
    os << "      ";
}

Grid::Grid(int seed, bool server) : random_generator(seed), server(server) {}

Grid::Grid(const GameState& state, const InitialData& init_data) : server(false) { init(state, init_data); }

Field& Grid::field_at(const Pos& pos) { return fields[pos.y][pos.x]; }
const Field& Grid::field_at(const Pos& pos) const { return fields[pos.y][pos.x]; }

Field& Grid::operator[](const Pos& pos) { return fields[pos.y][pos.x]; }
const Field& Grid::operator[](const Pos& pos) const { return fields[pos.y][pos.x]; }

void Grid::init(const GameState& state, const InitialData& init_data) {
  scores.assign(4, unordered_map<int, double>());
  tick = state.tick;
  max_tick = init_data.max_tick;
  max_throw_length = init_data.grenade_radius + 1;
  int n = init_data.size;
  fields.assign(n, vector<Field>(n));
  for (int i = 0; i < n; i++) {
    fields[0][i].is_bush = true;
    fields[i][0].is_bush = true;
    fields[n - 1][i].is_bush = true;
    fields[i][n - 1].is_bush = true;
  }
  for (int i = 0; i < n; i += 2) {
    for (int j = 0; j < n; j += 2) {
      fields[i][j].is_bush = true;
    }
  }

  for (const Grenade& grenade : state.grenades) {
    field_at(grenade.pos).grenades.push_back(grenade);
  }
  for (const Powerup& powerup : state.powerups) {
    field_at(powerup.pos).powerup = powerup;
  }
  for (const Bat& bat : state.bats) {
    field_at(bat.pos).bat = bat;
  }
  for (const Vampire& vampire : state.vampires) {
    field_at(vampire.pos).vampires.push_back(vampire);
    grenades_before_step[vampire.id] = vampire.grenades;
    shoes_before_step[vampire.id] = vampire.shoes;
  }
}

const GameState& Grid::get_state() const {
  if (!state_cache.has_value()) {
    GameState state;
    state.tick = tick;
    for (const auto& row : fields) {
      for (const auto& field : row) {
        if (field.bat.has_value()) state.bats.push_back(field.bat.value());
        if (field.powerup.has_value()) state.powerups.push_back(field.powerup.value());
        state.vampires.insert(state.vampires.end(), field.vampires.begin(), field.vampires.end());
        state.grenades.insert(state.grenades.end(), field.grenades.begin(), field.grenades.end());
      }
    }
    state_cache = move(state);
  }
  return *state_cache;
}

void Grid::place_possible_grenades(int self_id) {
  for (auto& row : fields) {
    for (Field& field : row) {
      for (auto& vampire : field.vampires) {
        if (vampire.id != self_id && grenades_before_step[vampire.id]) {
          field.grenades.push_back({vampire.pos, vampire.id, GRENADE_TICKS, vampire.range});
          --vampire.grenades;
        }
      }
    }
  }
}

vector<Pos> Grid::from_where_can_throw_to(Pos target) {
  const Field& to = field_at(target);
  if (to.is_bush || to.bat.has_value()) return {};
  vector<Pos> froms;
  for (Pos delta : pos_deltas) {
    Pos pos = target;
    for (int i = 1; i <= max_throw_length; i++) {
      pos += delta;
      if (pos.y < 0 || pos.x < 0 || pos.y >= (int)fields.size() || pos.x >= (int)fields[0].size()) break;
      if (field_at(pos).can_step_here()) froms.push_back(pos);
    }
  }
  return froms;
}

void Grid::step(const map<int, Step>& vampire_steps) {
  ++tick;
  step_powerups();
  step_grenades();
  step_vampires(vampire_steps);
  state_cache.reset();
}

void Grid::step_vampires(map<int, Step> vampire_steps) {
  if (vampire_steps.empty()) return;

  // Place / throw grenades first
  for (auto& row : fields) {
    for (Field& field : row) {
      for (auto& vampire : field.vampires) {
        if (vampire.invulnerable) {
          --vampire.invulnerable;
          continue;
        }
        auto step_it = vampire_steps.find(vampire.id);
        if (step_it != vampire_steps.end()) {
          if (step_it->second.place_grenade && step_it->second.throw_grenades.has_value())
            error("Cannot place and throw grenade at the same time");
          if (step_it->second.place_grenade && grenades_before_step[vampire.id]) {
            field.grenades.push_back({vampire.pos, vampire.id, GRENADE_TICKS, vampire.range});
            --vampire.grenades;
          } else if (step_it->second.throw_grenades.has_value()) {
            handle_throw(step_it->second.throw_grenades.value(), vampire);
          }
        }
      }
    }
  }

  // Move vampires
  for (auto& row : fields) {
    for (Field& field : row) {
      for (auto vampire_it = field.vampires.begin(); vampire_it != field.vampires.end(); ++vampire_it) {
        Vampire vampire = *vampire_it;
        auto step_it = vampire_steps.find(vampire.id);
        if (step_it == vampire_steps.end() || !step_it->second.move.has_value()) continue;
        const vector<Direction>& move = step_it->second.move.value();
        Pos pos = vampire.pos, new_pos = vampire.pos;
        for (size_t i = 0; i < move.size(); i++) {
          pos += pos_deltas[(int)move[i]];
          if (!field_at(pos).can_step_here() || (i == 2 && !shoes_before_step[vampire.id])) {
            break;
          }
          new_pos = pos;
        }
        vampire_steps.erase(step_it);

        if (new_pos != vampire.pos) {
          powerup_protection[vampire.id] = 0;  // vampire moved, resetting powerup protection counter
          vampire.pos = new_pos;
          field_at(new_pos).vampires.push_back(vampire);
          vampire_it = field.vampires.erase(vampire_it);
          if (field.vampires.empty()) break;
          vampire_it--;
        }
      }
    }
  }
}

void Grid::handle_throw(const Throw& thro, const Vampire& vampire) {
  if (thro.length > max_throw_length) throw runtime_error("Cannot throw this far");
  Pos from_pos = vampire.pos;
  if (!thro.from_place) {
    from_pos += pos_deltas[(int)thro.dir];
  }
  Field& from_field = field_at(from_pos);
  bool has_grenade = false;
  for (const auto& grenade : from_field.grenades)
    if (grenade.vampire_id == vampire.id) has_grenade = true;
  if (!has_grenade) return;
  Pos to_pos = from_pos + thro.length * pos_deltas[(int)thro.dir];
  Field& to_field = field_at(to_pos);
  if (to_field.is_bush || to_field.bat.has_value()) return;
  vector<Grenade> remain;
  for (const auto& grenade : from_field.grenades) {
    if (grenade.vampire_id == vampire.id) {
      to_field.grenades.push_back(grenade);
      to_field.grenades.back().pos = to_pos;
    } else
      remain.push_back(grenade);
  }
  from_field.grenades = remain;
}

void Grid::step_powerups() {
  int num_vampires = 0;
  for (auto& row : fields) {
    for (auto& field : row) {
      field.step_powerup(*this);
      num_vampires += field.vampires.size();
    }
  }
  if (server) {
    vector<Powerup> powerups = random_generator.get_possible_powerups_to_place(fields.size(), num_vampires);
    for (const auto& powerup : powerups) {
      field_at(powerup.pos).powerup = powerup;
    }
  }
}

void Grid::step_grenades() {
  unordered_map<int, int> grenades_exploded_of_vampire;

  queue<Grenade> explodes_queue;
  for (auto& row : fields) {
    for (auto& field : row) {
      field.reset_light();
      for (auto& grenade : field.grenades) {
        --grenade.tick;
        if (grenade.tick == 0) {
          field.illuminate(explodes_queue, grenade.vampire_id);
        }
      }
    }
  }

  while (!explodes_queue.empty()) {
    Grenade grenade = explodes_queue.front();
    explodes_queue.pop();
    ++grenades_exploded_of_vampire[grenade.vampire_id];
    for (Pos delta : pos_deltas) {
      Pos pos = grenade.pos;
      for (int i = 1; i <= grenade.range; i++) {
        pos += delta;
        Field& field = field_at(pos);
        if (!field.illuminate(explodes_queue, grenade.vampire_id)) {
          break;
        }
      }
    }
  }

  switch_lights_at_end();

  for (auto& row : fields) {
    for (auto& field : row) {
      field.evaluate_light(*this);
      for (auto& vampire : field.vampires) {
        vampire.grenades += grenades_exploded_of_vampire[vampire.id];
      }
    }
  }
}

void Grid::switch_lights_at_end() {
  for (int nTh = 0; nTh < tick - max_tick; nTh++) {
    int size = fields.size();
    int j = size * size - 4 * nTh;
    if (j >= 0) {
      int line = (size - sqrt(j)) / 2;
      int column = nTh - line * (size - 1 - line);

      fields[line][column].has_light = true;
      if (line != (size - 1) / 2) {
        fields[column][size - 1 - line].has_light = true;
        fields[size - 1 - column][line].has_light = true;
        fields[size - 1 - line][size - 1 - column].has_light = true;
      }
    }
  }
}

void Grid::print(std::ostream& os) const {
  int rowIdx = 0;
  for (const auto& row : fields) {
    for (const Field& field : row) {
      field.print_grenade(os, rowIdx);
    }
    os << endl;
    for (const Field& field : row) {
      field.print_vampire(os);
    }
    os << endl;

    int colIdx = 0;
    for (const Field& field : row) {
      field.print_powerup(os, colIdx);
      colIdx++;
    }
    os << endl;
    rowIdx++;
  }
  os << endl;
}

optional<Vampire> Grid::get_vampire(int id) const {
  auto vampires = get_state().vampires;
  auto it = find_if(vampires.begin(), vampires.end(), [&](const Vampire& vampire) { return vampire.id == id; });
  return it == vampires.end() ? nullopt : make_optional(*it);
}

vector<Vampire> Grid::get_enemies(int self_id) const {
  vector<Vampire> enemies;
  // for each enemy vampire
  const auto& state = get_state();
  for (const auto& vampire : state.vampires) {
    if (vampire.id == self_id) continue;
    auto enemy_vampire = get_vampire(vampire.id);
    if (enemy_vampire.has_value()) {
      enemies.push_back(enemy_vampire.value());
    }
  }

  return enemies;
}