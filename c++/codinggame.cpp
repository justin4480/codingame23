#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

bool VERBOSE = false;

struct Move {
  int x;
  int y;
  int light;
  std::string strategy;

  void print() const {
    // std::cout << "Move " << x << " " << y << " " << light << " "
    //           << (VERBOSE ? strategy
    //                       : std::to_string(std::hash<std::string>{}(strategy)))
    //           << std::endl;
    std::cout << "Move " << x << " " << y << " " << light << " " << strategy << std::endl;
  }
};

struct Point {
  int x, y;
};

Point getXYInDirection(Point start, Point destination, int distance) {
  double angle = atan2(destination.y - start.y, destination.x - start.x);
  int x = start.x + distance * cos(angle);
  int y = start.y + distance * sin(angle);
  return Point{x, y};
}

double getDistance(Point a, Point b) { return hypot(b.x - a.x, b.y - a.y); }

double closestDistanceFromPointToLineSegment(Point a, Point b, Point pointC) {
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double t =
      ((pointC.x - a.x) * dx + (pointC.y - a.y) * dy) / (dx * dx + dy * dy);
  t = std::max(0.0, std::min(1.0, t));
  Point closestPoint = {static_cast<int>(std::round(a.x + t * dx)),
                        static_cast<int>(std::round(a.y + t * dy))};
  double distance = sqrt(pow(pointC.x - closestPoint.x, 2) +
                         pow(pointC.y - closestPoint.y, 2));
  return distance;
}

class Drone {
public:
  Drone(int drone_id, int x, int y, int emergency, int battery)
      : drone_id(drone_id), x(x), y(y), emergency(emergency), battery(battery) {
  }
  int drone_id;
  int x;
  int y;
  int emergency;
  int battery;
  int maxY = 0;
  int cachedPoints = 0;
  bool left;
  bool top;
  std::string otherDroneRadar;
  std::unordered_map<int, std::string> radar;
  std::unordered_map<int, int> distanceToFish;
  std::set<int> cachedScans;
  std::vector<Move> moveHistory = {Move{x, y, 0, "First"}};
  std::vector<std::string> maxRadarHistory;

  void addToCacheScan(int creature_id) {
    this->cachedScans.insert(creature_id);
  }

  void addToMaxRadarHistory(std::string radar) {
    this->maxRadarHistory.push_back(radar);
  }

  Point checkForMaxRadarHistoryPattern() const {
    if (this->maxRadarHistory.size() < 4) {
      return {-1, -1};
    }

    std::vector<std::string> patterns[8] = {
      {"BL", "BR", "BL"},
      {"TL", "TR", "TL"},
      {"TL", "BL", "TL"},
      {"TR", "BR", "TR"},
      {"BR", "BL", "BR"},
      {"TR", "TL", "TR"},
      {"BL", "TL", "BL"},
      {"BR", "TR", "BR"}
    };

    Point offsets[8] = {
      {x, y+600},
      {x, y-600},
      {x-600, y},
      {x+600, y},
      {x, y+600},
      {x, y-600},
      {x-600, y},
      {x+600, y}
    };

    int n = maxRadarHistory.size();
    for (int i = 0; i < 8; ++i) {
      if (patterns[i][0] == maxRadarHistory[n-3] && patterns[i][1] == maxRadarHistory[n-2] && patterns[i][1] == maxRadarHistory[n-1])
        return offsets[i];
      if (patterns[i][0] == maxRadarHistory[n-4] && patterns[i][1] == maxRadarHistory[n-3] && patterns[i][1] == maxRadarHistory[n-2])
        return offsets[i];
      }

    return {-1, -1};
  }

  void setAttributes(int x, int y, int emergency, int battery) {
    this->x = x;
    this->y = y;
    this->emergency = emergency;
    this->battery = battery;
    if (y > this->maxY) {
      this->maxY = y;
    }
  }

  void reset() {
    this->radar.clear();
    this->cachedScans.clear();
  }

  Point getXY() const { return Point{x, y}; }

  std::string getRadar(int creature_id) const {
    if (this->radar.find(creature_id) != this->radar.end()) {
      return this->radar.at(creature_id);
    }
    return "";
  }

  bool isCreatureAtRadarPoint(int creature_id, std::string radarPoint) const {
    return this->radar.at(creature_id) == radarPoint;
  }

  bool fishOnRadar(int creature_id) {
    if (this->radar.find(creature_id) != this->radar.end()) {
      return true;
    }
    return false;
  }

  void appendMoveHistory(Move move) { this->moveHistory.push_back(move); }

  std::string getLastStrategy() const {
    return this->moveHistory.back().strategy;
  }

  Move getLastMove() const { return this->moveHistory.back(); }

  int getLastLight() const {
    int counter = 0;
    for (auto it = this->moveHistory.rbegin(); it != this->moveHistory.rend();
         ++it) {
      if (it->light == 1) {
        return counter;
      }
      counter++;
    }
    return 99;
  }

  bool hasEscapeMonsters() const {
    for (auto it = this->moveHistory.rbegin(); it != this->moveHistory.rend();
         ++it) {
      if (it->strategy == "EscapeMonsters") {
        return true;
      }
    }
    return false;
  }

  int getCaughtCatchSize() { return this->cachedScans.size(); }

  Point getLastAttractionPoint() const {
    for (auto it = this->moveHistory.rbegin(); it != this->moveHistory.rend();
         ++it) {
      if (it->strategy != "EscapeMonsters") {
        return Point{it->x, it->y};
      }
    }
    return Point{0, 0};
  }
};

class Drones {
public:
  Drones() {}
  std::unordered_map<int, Drone> droneMap;
  std::vector<int> droneOrder;
  int maxY = 0;
  int lightOn = 0;

  void frameUpdate() {
    for (auto &[creature_id, drone] : this->droneMap) {
      drone.reset();
      getLeftDrone();
      getUpDrone();
      getOtherDroneRadar();
      int i;
    }
  }

  int getDistanceBetweenDrones() const {
    return getDistance({this->droneMap.at(droneOrder[0]).x,
                        this->droneMap.at(droneOrder[0]).y},
                       {this->droneMap.at(droneOrder[1]).x,
                        this->droneMap.at(droneOrder[1]).y});
  }

  bool isFirstDrone(int drone_id) const {
    return this->droneOrder.at(0) == drone_id;
  }

  Drone getOtherDrone(Drone me) const {
    if (me.drone_id == this->droneOrder[0]) {
      return this->droneMap.at(this->droneOrder[1]);
    } else {
      return this->droneMap.at(this->droneOrder[0]);
    }
  }

  void getUpDrone() {
    if (droneMap.at(droneOrder[0]).y < droneMap.at(droneOrder[1]).y) {
      droneMap.at(droneOrder[0]).top = true;
      droneMap.at(droneOrder[1]).top = false;
    } else {
      droneMap.at(droneOrder[0]).top = false;
      droneMap.at(droneOrder[1]).top = true;
    }
  }

  void getLeftDrone() {
    if (droneMap.at(droneOrder[0]).x < droneMap.at(droneOrder[1]).x) {
      droneMap.at(droneOrder[0]).left = true;
      droneMap.at(droneOrder[1]).left = false;
    } else {
      droneMap.at(droneOrder[0]).left = false;
      droneMap.at(droneOrder[1]).left = true;
    }
  }

  void getOtherDroneRadar() {
    std::string drone0 = "";
    std::string drone1 = "";

    if (droneMap.at(droneOrder[0]).y < droneMap.at(droneOrder[1]).y) {
      drone0 += "B";
      drone1 += "T";
    } else {
      drone0 += "T";
      drone1 += "B";
    }
    if (droneMap.at(droneOrder[0]).x < droneMap.at(droneOrder[1]).x) {
      drone0 += "R";
      drone1 += "L";
    } else {
      drone0 += "L";
      drone1 += "R";
    }
    droneMap.at(droneOrder[0]).otherDroneRadar = drone0;
    droneMap.at(droneOrder[1]).otherDroneRadar = drone1;
  }

  bool contains(int drone_id) {
    return this->droneMap.find(drone_id) != this->droneMap.end();
  }

  void addToCacheScan(int drone_id, int creature_id) {
    this->droneMap.at(drone_id).addToCacheScan(creature_id);
  }

  void addRadarBlip(int drone_id, int creature_id, std::string radar) {
    this->droneMap.at(drone_id).radar.emplace(creature_id, radar);
  }

  int size() { return this->droneMap.size(); }

  void addDrone(Drone drone) {
    this->droneMap.emplace(drone.drone_id, drone);
    this->droneOrder.push_back(drone.drone_id);
  }

  void update(int drone_id, int x, int y, int emergency, int battery) {
    this->droneMap.at(drone_id).setAttributes(x, y, emergency, battery);
    if (y > this->maxY) {
      this->maxY = y;
    }
  }
};

class Creature {
public:
  Creature(int creature_id, int lastSeenThreshold = 3)
      : creature_id(creature_id), lastSeenThreshold(lastSeenThreshold) {}
  int creature_id;
  int x = -1;
  int y = -1;
  int vx = -1;
  int vy = -1;
  int last_seen = -1;
  int lastSeenThreshold;
  bool stationary = false;

  void updateXY(int x, int y, int vx, int vy) {
    if (this->x != -1 && x != -1) {
      this->stationary = (this->x == x && this->y == y);
    }
    this->x = x;
    this->y = y;
    this->vx = vx;
    this->vy = vy;
    this->last_seen = 0;
  }

  void resetXY() {
    this->last_seen++;
    if (this->last_seen > this->lastSeenThreshold) {
      this->x = -1;
      this->y = -1;
      this->vx = -1;
      this->vy = -1;
    }
  }
};

class Fish : public Creature {
public:
  Fish(int creature_id, int color, int type)
      : Creature(creature_id), color(color), type(type) {}
  struct Caught {
    struct Type {
      bool saved;
      bool cached;
    } me, fo;
  };
  int color;
  int type;
  bool lost = false;
  Caught caught;
};

class Fishes {
public:
  Fishes() {}
  std::unordered_map<int, Fish> fishMap;
  int available = 0;
  int caught = 0;
  int lost = 0;
  int cached = 0;

  void addFish(Fish fish) { this->fishMap.emplace(fish.creature_id, fish); }

  void frameUpdate() {
    for (auto &[creature_id, fish] : this->fishMap) {
      fish.resetXY();
      fish.lost = false;
      fish.caught.fo.cached = false;
      fish.caught.fo.saved = false;
      fish.caught.me.cached = false;
      fish.caught.me.saved = false;
    }
  }

  bool contains(int creature_id) {
    return this->fishMap.find(creature_id) != this->fishMap.end();
  }

  void updateXY(int creature_id, int x, int y, int vx, int vy) {
    this->fishMap.at(creature_id).updateXY(x, y, vx, vy);
  }
};

class Monster : public Creature {
public:
  Monster(int _creature_id) : Creature(_creature_id) {}
};

class Monsters {
public:
  Monsters() {}
  std::unordered_map<int, Monster> monsterMap;

  void frameUpdate() {
    for (auto &[creature_id, monster] : this->monsterMap) {
      monster.resetXY();
    }
  }

  void addMonster(Monster monster) {
    this->monsterMap.emplace(monster.creature_id, monster);
  }

  void updateXY(int creature_id, int x, int y, int vx, int vy) {
    this->monsterMap.at(creature_id).updateXY(x, y, vx, vy);
  }
};

Point getOptimalXYOffset(Drone drone, Monsters monsters,
                         double xMovementFraction = 0.5) {

  int offsetX = 0;
  int offsetY = 0;
  int totalMonsters = 0;
  std::map<std::string, int> monsterCounts;

  for (const auto &[creature_id, Monster] : monsters.monsterMap) {
    std::string radar = drone.getRadar(creature_id);
    monsterCounts[radar]++;
    totalMonsters++;
  }

  if (totalMonsters == 0)
    return Point{0, 0};
  if (totalMonsters > 0) {
    double normalizedTR =
        static_cast<double>(monsterCounts["TR"]) / totalMonsters;
    double normalizedTL =
        static_cast<double>(monsterCounts["TL"]) / totalMonsters;
    offsetX = static_cast<int>((normalizedTR - normalizedTL) *
                               (9999 - drone.x) * xMovementFraction);
  }

  return Point{offsetX, offsetY};
}

class Strategy {
public:
  Strategy(const std::string &name) : name(name) {}
  virtual void calculate() = 0;
  bool check() { return this->x != -1; }
  Move getMove() { return Move{this->x, this->y, this->light, this->name}; }
  std::string getName() { return this->name; }

protected:
  std::string name;
  int x = -1;
  int y = -1;
  int light = 0;
};

class StrategyScareAway : public Strategy {
private:
  const Drone &drone;
  const Fishes &fishes;
  const Monsters &monsters;
  const int gameFrame;
  int distanceThreshold;
  int distanceBuffer;

public:
  StrategyScareAway(const Drone &drone, const Fishes &fishes,
                    const Monsters &monsters, const int gameFrame,
                    int distanceThreshold = 1500, int distanceBuffer = 500)
      : Strategy("ScareAway"), drone(drone), fishes(fishes), monsters(monsters),
        gameFrame(gameFrame),
        distanceThreshold(distanceThreshold), distanceBuffer(distanceBuffer) {
    calculate();
  }

  void calculate() {
    int minDistance = 100000;

    if (drone.cachedScans.size() >= 4)
      return;
    if (monsters.monsterMap.size() == 2 && drone.cachedScans.size() > 0)
      return;
    if (monsters.monsterMap.size() == 2 && fishes.available > 0)
      return;
    if (gameFrame <= 12)
      return;

    for (auto &[creature_id, fish] : this->fishes.fishMap) {
      if (fish.x == -1 || fish.lost || fish.last_seen > 1)
        continue;
      // if (drone.y > 5000 || !fish.caught.me.saved)
      //   continue;
      int distance = drone.distanceToFish.at(creature_id);
      if (!fish.caught.fo.cached && !fish.caught.fo.saved && distance < this->distanceThreshold && distance < minDistance) {
        if ((fish.x < this->distanceThreshold + distanceBuffer || (fishes.available == 0 && drone.cachedScans.size() == 0)) && this->drone.x > fish.x) {
          this->x = fish.x + (fish.last_seen == 0 ? -100 : 300);
          this->y = fish.y;
          this->light = 1;
          minDistance = drone.distanceToFish.at(creature_id);
        } else if ((fish.x > 9999 - this->distanceThreshold - distanceBuffer || (fishes.available == 0 && drone.cachedScans.size() == 0)) && this->drone.x < fish.x) {
          this->x = fish.x - (fish.last_seen == 0 ? 100 : 300);
          this->y = fish.y;
          this->light = 1;
          minDistance = drone.distanceToFish.at(creature_id);
        }
      }
    }
  }
};

class StrategySeperateDrones : public Strategy {
private:
  const Drones &myDrones;
  const Drone &drone;
  const int distanceThreshold;

public:
  StrategySeperateDrones(const Drones &myDrones, const Drone &drone,
                         int distanceThreshold = 1000)
      : Strategy("SeperateDrones"), myDrones(myDrones), drone(drone),
        distanceThreshold(distanceThreshold) {
    calculate();
  }
  void calculate() {
    if (this->myDrones.isFirstDrone(this->drone.drone_id))
      return;
    Drone otherDrone = this->myDrones.getOtherDrone(this->drone);
    int distance = this->myDrones.getDistanceBetweenDrones();
    if (distance > this->distanceThreshold)
      return;

    Point otherDroneStartXY = otherDrone.getXY();
    Point otherDroneEndXY = getXYInDirection(
        otherDrone.getXY(),
        {otherDrone.getLastMove().x, otherDrone.getLastMove().y}, 600);

    int offsetX = 2 * (otherDroneEndXY.x - otherDroneStartXY.x);
    int offsetY = 2 * (otherDroneEndXY.y - otherDroneStartXY.y);

    int newX = std::max(0, std::min(9999, this->drone.x - offsetX));
    int newY = std::max(0, std::min(9999, this->drone.y - offsetY));

    this->x = newX;
    this->y = newY;
    this->light = otherDrone.getLastLight() == 0 ? 1 : 0;
  }
};

class StrategyEmergency : public Strategy {
private:
  const Drone drone;

public:
  StrategyEmergency(const Drone &drone) : Strategy("Emergency"), drone(drone) {
    calculate();
  }
  void calculate() {
    if (this->drone.emergency == 1) {
      this->x = 5000;
      this->y = this->drone.y;
      this->light = 0;
    }
  }
};

class StrategyCenter : public Strategy {
public:
  StrategyCenter() : Strategy("Center") {}
  void calculate() {
    this->x = 5000;
    this->y = 2000;
    this->light = 0;
  }
};

class StrategyWait : public Strategy {
public:
  StrategyWait() : Strategy("Wait") {}
  void calculate() {
    this->x = 5000;
    this->y = 500;
    this->light = 0;
  }
};

class StrategyCashIn : public Strategy {
private:
  const Drone drone;
  const Drones drones;
  const Fishes fishes;
  const Monsters monsters;
  // the higher the threshold, the more likely the drone will cash in
  int cacheThreshold4;
  int cacheThreshold6;

public:
  StrategyCashIn(const Drone &drone, const Drones &drones, const Fishes &fishes,
                 const Monsters &monsters, int cacheThreshold4 = 250,
                 int cacheThreshold6 = 250)
      : Strategy("CashIn"), drone(drone), drones(drones), fishes(fishes), monsters(monsters),
        cacheThreshold4(cacheThreshold4), cacheThreshold6(cacheThreshold6) {
    calculate();
  }

  int getOffsetX() {
    int offset = 0;
    int totalMonsters = 0;
    std::map<std::string, int> monsterCounts;

    for (const auto &[creature_id, Monster] : this->monsters.monsterMap) {
      std::string radar = this->drone.getRadar(creature_id);
      monsterCounts[radar]++;
      totalMonsters++;
    }

    if (totalMonsters > 0) {
      double normalizedTR =
          static_cast<double>(monsterCounts["TR"]) / totalMonsters;
      double normalizedTL =
          static_cast<double>(monsterCounts["TL"]) / totalMonsters;

      offset = static_cast<int>((normalizedTR - normalizedTL) *
                                (9999 - drone.x) / 3);
    }

    return offset;
  }

  void calculate() {
    if (this->drones.maxY < 5000)
      return;
    if (this->drone.cachedScans.size() == 0)
      return;
    if (fishes.available == 1)
      return;

    int numberMonsters = this->monsters.monsterMap.size();
    int value = (this->drone.y - 500) / this->drone.cachedScans.size();
    if (value <= 0)
      return;
    if (numberMonsters < 4)
      return;
    if (numberMonsters == 4 && value > numberMonsters * this->cacheThreshold4)
      return;
    if (numberMonsters > 4 && value > numberMonsters * this->cacheThreshold6)
      return;

    this->x = drone.x;
    this->y = drone.y - 600;
    this->light = 0;
  }
};

class StrategyGoHome : public Strategy {
protected:
  const Drone drone;
  const Fishes fishes;

public:
  StrategyGoHome(const Drone &drone, const Fishes &fishes)
      : Strategy("GoHome"), drone(drone), fishes(fishes) {
    calculate();
  }
  void calculate() {
    if (this->fishes.available == 0 && this->drone.cachedScans.size() > 0 &&
        this->drone.y > 500) {
      // TODO add optimalX
      this->x = drone.x;
      this->y = drone.y - 600;
      this->light = 0;
    }
  }
};

class StrategySeekDenseFish : public Strategy {
protected:
  const Drones &myDrones;
  Drone &drone;
  const Fishes &fishes;
  const Monsters &monsters;
  int gameFrame;
  double monsterPenalty;
  double otherDronePenalty;
  double fishValueMultiplier;
  struct RadarData {
    Point xy;
    int numberFish = 0;
    int valueFish = 0;
    int numberMonsters = 0;
    int totalArea = 0;
    double density = 0;
  };
  std::unordered_map<std::string, RadarData> radar_map = {
      {"TL", {{0, 0}}},
      {"TR", {{9999, 0}}},
      {"BL", {{0, 9999}}},
      {"BR", {{9999, 9999}}}};

public:
  StrategySeekDenseFish(const Drones &myDrones, Drone &drone,
                        const Fishes &fishes, const Monsters &monsters,
                        int gameFrame, double monsterPenalty = 2,
                        double otherDronePenalty = 2,
                        double fishValueMultiplier = 0.2)
      : Strategy("SeekDenseFish"), myDrones(myDrones), drone(drone),
        fishes(fishes), monsters(monsters), gameFrame(gameFrame),
        monsterPenalty(monsterPenalty), otherDronePenalty(otherDronePenalty),
        fishValueMultiplier(fishValueMultiplier) {
    calculate();
  }

  void calculate() {
    if (fishes.available == 0)
      return;
    double maxDensity = -1;
    std::string maxRadar;
    for (const auto &[creature_id, fish] : this->fishes.fishMap) {
      if (!fish.caught.me.cached && !fish.caught.me.saved && !fish.lost) {
        this->radar_map.at(this->drone.radar.at(creature_id)).numberFish++;
        this->radar_map.at(this->drone.radar.at(creature_id)).valueFish +=
            (fish.type + 1) * 2;
      }
    }
    for (const auto &[creature_id, Monster] : this->monsters.monsterMap) {
      this->radar_map.at(this->drone.radar.at(creature_id)).numberMonsters++;
    }
    for (auto &[radar, radarData] : this->radar_map) {
      radarData.totalArea = abs(radarData.xy.x - this->drone.x) *
                            abs(radarData.xy.y - this->drone.y);
      double penalty = radarData.numberMonsters * this->monsterPenalty;
      if (drone.otherDroneRadar == radar)
        penalty += 1 * this->otherDronePenalty;

      radarData.density =
          (static_cast<double>(radarData.numberFish) +
           static_cast<double>(radarData.valueFish * fishValueMultiplier)) /
          (1 + static_cast<double>(penalty)) /
          (1 + static_cast<double>(radarData.totalArea));
      if (radarData.density > maxDensity) {
        maxDensity = radarData.density;
        maxRadar = radar;
      }
    }
    // } else if (this->drone.getLastLight() <=
    //            (rand() % lightRandomRangeMax + lightRandomRangeMin)) {
    if (this->radar_map.find(maxRadar) != this->radar_map.end()) {

      this->drone.addToMaxRadarHistory(maxRadar);
      Point maxRadarPattern = this->drone.checkForMaxRadarHistoryPattern();
      if (maxRadarPattern.x != -1) {
        this->x = maxRadarPattern.x;
        this->y = maxRadarPattern.y;
        // std::cerr << "straight line" << std::endl;
      } else {
        this->x = this->radar_map.at(maxRadar).xy.x;
        this->y = this->radar_map.at(maxRadar).xy.y;
        // std::cerr << "diagonal" << std::endl;
      }
      
      if (this->drone.getLastStrategy() == "escape_monsters") {
        this->light = 1;
      } else if (this->drone.y < 2500 and gameFrame > 10) {
        this->light = 0;
      } else if (this->myDrones.lightOn == 1 && this->myDrones.getDistanceBetweenDrones() < 1000) {
        this->light = 0;
      } else if (this->myDrones.lightOn == 0 && this->myDrones.getDistanceBetweenDrones() < 1000) {
        this->light = 0;
      } else if (drone.left && (
        gameFrame == 3 || gameFrame == 5 || gameFrame == 8 || gameFrame == 11 ||
        gameFrame == 14 || gameFrame == 17 || gameFrame == 20 || gameFrame == 23
        )) {
        this->light = 1;
      } else if (!drone.left && (
        gameFrame == 3 || gameFrame == 4 || gameFrame == 7 || gameFrame == 10 ||
        gameFrame == 13 || gameFrame == 16 || gameFrame == 19 || gameFrame == 22
        )) {
        this->light = 1;
      } else if (drone.battery >= 20 and gameFrame > 10) {
        this->light = 1;
      } else if (gameFrame > 6 && drone.left && gameFrame % 6 == 0) {
        this->light = 1;
      } else if (gameFrame > 6 && !drone.left && gameFrame % 6 == 3) {
        this->light = 1;
      } else {
        this->light = 0;
      }
    }
  }
};

class StrategySeekFoFish : public Strategy {
protected:
  const Drone drone;
  const Fishes fishes;
  struct RadarData {
    Point xy;
    int numberFish = 0;
    int totalArea = 0;
    double density = 0;
  };
  std::unordered_map<std::string, RadarData> radar_map = {
      {"TL", {{0, 0}}},
      {"TR", {{9999, 0}}},
      {"BL", {{0, 9999}}},
      {"BR", {{9999, 9999}}}};

public:
  StrategySeekFoFish(const Drone &drone, const Fishes &fishes)
      : Strategy("SeekDenseFoFish"), drone(drone), fishes(fishes) {
    calculate();
  }

  void calculate() {
    double maxDensity = -1;
    std::string maxRadar;
    for (const auto &[creature_id, fish] : this->fishes.fishMap) {
      if (!fish.caught.fo.saved && !fish.caught.fo.cached && !fish.lost) {
        this->radar_map.at(this->drone.radar.at(creature_id)).numberFish++;
      }
    }
    for (auto &[radar, radarData] : this->radar_map) {
      radarData.totalArea = abs(radarData.xy.x - this->drone.x) *
                            abs(radarData.xy.y - this->drone.y);
      radarData.density = static_cast<double>(radarData.numberFish) /
                          static_cast<double>((drone.otherDroneRadar == radar) ? 3 : 1) /
                          static_cast<double>(radarData.totalArea);
      if (radarData.density > maxDensity) {
        maxDensity = radarData.density;
        maxRadar = radar;
      }
    }
    if (this->radar_map.find(maxRadar) != this->radar_map.end()) {
      this->x = this->radar_map.at(maxRadar).xy.x;
      this->y = this->radar_map.at(maxRadar).xy.y;
      this->light = this->drone.getLastStrategy() == "escape_monsters" ? 0 : 1;
    }
  }
};

class StrategyNavigateMonsters : public Strategy {
protected:
  const Drone &drone;
  const Monsters &monsters;
  const Move &bestMove;
  int anglesToSimulation;
  int vetoThreshold;
  int monsterDistanceThreshold;
  struct Simulation {
    std::string resultName;
    Point resultXY;
    int CountOfAnglesSimulated;
    int CountOfVetoedAngles;
    int CountOfNonVetoedAngles;
    int MonsterDistanceCurrent;
    int MonsterDistanceWorstCaseScenario;
    int MonsterDistancePlanned;
    int LeastWorstVetoedMoveMonsterDistance;
    int LeastWorstVetoedMoveAngle;
    int LeastWorstVetoedMoveDegree;
    double LeastWorstVetoedMoveScore;
    Point LeastWorstVetoedMoveXY;
    int BestNonVetoedMoveMonsterDistance;
    int BestNonVetoedMoveAngle;
    int BestNonVetoedMoveDegree;
    double BestNonVetoedMoveScore;
    Point BestNonVetoedMoveXY;
    struct Angle {
      int angle;
      int degree;
      Point droneStartXY;
      Point droneEndXY;
      int droneDistanceTravelled;
      bool targetsHaveVetoed;
      int targetsMonsterMinDistance;
      int targetsBestMoveScore;
      struct Target {
        std::string name;
        Point startXY;
        Point endXY;
        int distanceTravelled;
        int distanceToDrone;
        int ptldistanceToDrone;
        double weight;
        double score;
        bool veto;
      };
      std::vector<Target> targets;
    };
    std::vector<Angle> angles;
  };

public:
  StrategyNavigateMonsters(const Drone &drone, const Monsters &monsters,
                           Move &bestMove, int anglesToSimulation = 6,
                           int vetoThreshold = 551,
                           int monsterDistanceThreshold = 601)
      : Strategy("EscapeMonsters"), drone(drone), monsters(monsters),
        bestMove(bestMove), anglesToSimulation(anglesToSimulation),
        vetoThreshold(vetoThreshold),
        monsterDistanceThreshold(monsterDistanceThreshold) {
    calculate();
  }

  void printSimulationResults(const Simulation &simulation) {
    std::cerr << std::endl;

    std::cerr << "Drone" << this->drone.drone_id << " Simulation Results"
              << " (" << simulation.resultName << "):" << std::endl;

    std::cerr << " - Monster Distances:"
              << " Current: " << simulation.MonsterDistanceCurrent
              << " | Planned: " << simulation.MonsterDistancePlanned
              << " | WCS: " << simulation.MonsterDistanceWorstCaseScenario
              << std::endl;

    std::cerr << " - Result:           "
              << " ProposedXY: (" << simulation.resultXY.x << ", "
              << simulation.resultXY.y << ")"
              << " | Vetoed Angles: " << simulation.CountOfVetoedAngles
              << " of " << simulation.CountOfAnglesSimulated << std::endl;

    std::cerr << " - Least Worst:       "
              << "Angle=" << simulation.LeastWorstVetoedMoveAngle
              << " | Degree=" << simulation.LeastWorstVetoedMoveDegree
              << " | Distance="
              << simulation.LeastWorstVetoedMoveMonsterDistance
              << " | Score=" << simulation.LeastWorstVetoedMoveScore
              << " | XY: (" << simulation.LeastWorstVetoedMoveXY.x << ", "
              << simulation.LeastWorstVetoedMoveXY.y << ")" << std::endl;

    std::cerr << " - Best:              "
              << "Angle=" << simulation.BestNonVetoedMoveAngle
              << " | Degree=" << simulation.BestNonVetoedMoveDegree
              << " | Distance=" << simulation.BestNonVetoedMoveMonsterDistance
              << " | Score=" << simulation.BestNonVetoedMoveScore << " | XY: ("
              << simulation.BestNonVetoedMoveXY.x << ", "
              << simulation.BestNonVetoedMoveXY.y << ")" << std::endl;
  }

  void printSimulationDetail(Simulation simulation) {
    if (!simulation.angles.empty()) {
      std::cerr << "Simulation Results:" << std::endl;
      for (const auto &angle : simulation.angles) {
        std::cerr << "- " << angle.degree << " degrees"
                  << " | "
                  << "(" << angle.droneStartXY.x << ", " << angle.droneStartXY.y
                  << ")->"
                  << "(" << angle.droneEndXY.x << ", " << angle.droneEndXY.y
                  << ") | " << angle.droneDistanceTravelled << std::endl;
        for (const auto &target : angle.targets) {
          std::cerr << "   - " << target.name << " | "
                    << "(" << target.startXY.x << ", " << target.startXY.y
                    << ")->"
                    << "(" << target.endXY.x << ", " << target.endXY.y << ") | "
                    << target.distanceToDrone << " | " << target.weight << " | "
                    << target.score << " | " << (target.veto ? "Yes" : "No")
                    << std::endl;
        }
      }
    } else {
      std::cerr << "No angles in the simulation." << std::endl;
    }
  }

  Simulation::Angle::Target getTargetData(Simulation::Angle simulationAngle,
                                          std::string name, Point startXY,
                                          Point endXY, double weight,
                                          int vetoThreshold) {
    Simulation::Angle::Target target;
    target.name = name;
    target.startXY = startXY;
    target.endXY = endXY;
    target.distanceTravelled = getDistance(startXY, endXY);
    target.ptldistanceToDrone = closestDistanceFromPointToLineSegment(
        simulationAngle.droneStartXY, simulationAngle.droneEndXY, endXY);
    target.distanceToDrone =
        getDistance(target.endXY, simulationAngle.droneEndXY);
    target.weight = weight;
    target.score = 10000 * (target.weight /
                            (1 + static_cast<double>(target.distanceToDrone)));
    target.veto = target.ptldistanceToDrone < vetoThreshold;
    return target;
  }

  void calculate() {

    Simulation simulation;
    simulation.MonsterDistanceCurrent = INT_MAX;
    simulation.MonsterDistancePlanned = INT_MAX;
    simulation.MonsterDistanceWorstCaseScenario = INT_MAX;

    // Generate Angles
    for (double angle = 0; angle < 2 * M_PI;
         angle += M_PI / this->anglesToSimulation) {
      int degree = angle * (180.0 / M_PI);
      double proposedX = this->drone.x + 600 * cos(angle);
      double proposedY = this->drone.y + 600 * sin(angle);
      int moveX = std::max(0.0, std::min(9999.0, proposedX));
      int moveY = std::max(0.0, std::min(9999.0, proposedY));
      Point droneStartXY = {this->drone.x, this->drone.y};
      Point droneEndXY = {static_cast<int>(moveX), static_cast<int>(moveY)};
      int droneDistanceTravelled = getDistance(droneStartXY, droneEndXY);
      Simulation::Angle simulationAngle;
      simulationAngle.angle = angle;
      simulationAngle.degree = degree;
      simulationAngle.droneStartXY = droneStartXY;
      simulationAngle.droneEndXY = droneEndXY;
      simulationAngle.droneDistanceTravelled = droneDistanceTravelled;

      // Append Monsters to simulation::angles
      for (auto &[creature_id, monster] : this->monsters.monsterMap) {
        if (monster.x == -1)
          continue;
        int distance =
            getDistance({this->drone.x, this->drone.y}, {monster.x, monster.y});
        if (distance < simulation.MonsterDistanceCurrent)
          simulation.MonsterDistanceCurrent = distance;
        std::string name = "Monster" + std::to_string(creature_id);
        int weight = -1;
        Point startXY;
        Point endXY;
        if (VERBOSE && angle == 0) {
          std::cerr << "Drone" << this->drone.drone_id << " (" << this->drone.x
                    << "," << this->drone.y << ")"
                    << " Monster" << monster.creature_id << " (" << monster.x
                    << "," << monster.y << ")"
                    << " Distance: " << distance << std::endl;
        }

        // t-0
        startXY = {monster.x, monster.y};
        endXY = startXY;
        simulationAngle.targets.push_back(
            getTargetData(simulationAngle, name + " (t0)", startXY, endXY,
                          weight, this->vetoThreshold));

        // drone
        startXY = {monster.x, monster.y};
        endXY = getXYInDirection({monster.x, monster.y},
                                 {this->drone.x, this->drone.y}, 540);
        simulationAngle.targets.push_back(
            getTargetData(simulationAngle, name + " (drone)", startXY, endXY,
                          weight, this->vetoThreshold));

        // velocity
        //   startXY = {monster.x, monster.y};
        //   endXY = getXYInDirection(
        //       {monster.x, monster.y},
        //       {monster.x + monster.vx, monster.y + monster.vy}, 540);
        //   simulationAngle.targets.push_back(
        //       getTargetData(simulationAngle, name + " (velocity)",
        //       startXY, endXY,
        //                     weight, this->vetoThreshold));
      }

      if (simulationAngle.targets.size() == 0) {
        return;
      }

      // Append BestMove to simulation::angles
      std::string name = "BestMove";
      Point startXY = {this->drone.x, this->drone.y};
      Point endXY = getXYInDirection(startXY, {bestMove.x, bestMove.y}, 1200);
      int weight = 1;
      simulationAngle.targets.push_back(
          getTargetData(simulationAngle, name, startXY, endXY, weight, -1));

      // Append simulation::angles to simulation::root
      simulation.angles.push_back(simulationAngle);
    }

    // Update new attributes for each angle
    for (auto &angle : simulation.angles) {
      angle.targetsHaveVetoed =
          std::any_of(angle.targets.begin(), angle.targets.end(),
                      [](const auto &target) { return target.veto; });
      angle.targetsMonsterMinDistance =
          std::min_element(angle.targets.begin(), angle.targets.end(),
                           [](const auto &a, const auto &b) {
                             return a.distanceToDrone < b.distanceToDrone;
                           })
              ->distanceToDrone;
      angle.targetsBestMoveScore =
          std::max_element(
              angle.targets.begin(), angle.targets.end(),
              [](const auto &a, const auto &b) { return a.score < b.score; })
              ->score;

      for (const auto &target : angle.targets) {
        if (target.name != "BestMove" &&
            target.distanceToDrone <
                simulation.MonsterDistanceWorstCaseScenario) {
          simulation.MonsterDistanceWorstCaseScenario = target.distanceToDrone;
        }
      }
    }

    simulation.CountOfAnglesSimulated = simulation.angles.size();
    simulation.CountOfNonVetoedAngles = std::count_if(
        simulation.angles.begin(), simulation.angles.end(),
        [](const auto &angle) { return !angle.targetsHaveVetoed; });
    simulation.CountOfVetoedAngles =
        simulation.CountOfAnglesSimulated - simulation.CountOfNonVetoedAngles;

    auto bestNonVetoedAngle =
        *std::max_element(simulation.angles.begin(), simulation.angles.end(),
                          [](const auto &a, const auto &b) {
                            if (a.targetsHaveVetoed)
                              return true;
                            if (!b.targetsHaveVetoed &&
                                a.targetsBestMoveScore < b.targetsBestMoveScore)
                              return true;
                            return false;
                          });

    simulation.BestNonVetoedMoveAngle = bestNonVetoedAngle.angle;
    simulation.BestNonVetoedMoveDegree = bestNonVetoedAngle.degree;
    simulation.BestNonVetoedMoveScore = bestNonVetoedAngle.targetsBestMoveScore;
    simulation.BestNonVetoedMoveXY = bestNonVetoedAngle.droneEndXY;
    simulation.BestNonVetoedMoveMonsterDistance =
        bestNonVetoedAngle.targetsMonsterMinDistance;

    auto bestVetoedAngle = *std::max_element(
        simulation.angles.begin(), simulation.angles.end(),
        [](const auto &a, const auto &b) {
          return a.targetsMonsterMinDistance < b.targetsMonsterMinDistance;
        });

    simulation.LeastWorstVetoedMoveAngle = bestVetoedAngle.angle;
    simulation.LeastWorstVetoedMoveDegree = bestVetoedAngle.degree;
    simulation.LeastWorstVetoedMoveScore = bestVetoedAngle.targetsBestMoveScore;
    simulation.LeastWorstVetoedMoveXY = bestVetoedAngle.droneEndXY;
    simulation.LeastWorstVetoedMoveMonsterDistance =
        bestVetoedAngle.targetsMonsterMinDistance;

    if (simulation.MonsterDistanceWorstCaseScenario >
        this->monsterDistanceThreshold) {
      simulation.resultName = "No Threat";
      simulation.resultXY = {0, 0};
      simulation.MonsterDistancePlanned =
          simulation.MonsterDistanceWorstCaseScenario;
    } else if (simulation.CountOfNonVetoedAngles == 0) {
      simulation.resultName = "LeastWorstMove";
      simulation.resultXY = simulation.LeastWorstVetoedMoveXY;
      simulation.MonsterDistancePlanned =
          simulation.LeastWorstVetoedMoveMonsterDistance;
    } else {
      simulation.resultName = "BestMove";
      simulation.resultXY = simulation.BestNonVetoedMoveXY;
      simulation.MonsterDistancePlanned =
          simulation.BestNonVetoedMoveMonsterDistance;
    }

    if (VERBOSE) {
      printSimulationResults(simulation);
      printSimulationDetail(simulation);
    }

    if (simulation.resultName != "No Threat") {
      this->x = simulation.resultXY.x;
      this->y = simulation.resultXY.y;
      this->light = 0;
    }
  }
};

class Input {
public:
  template <typename T> static T read() {
    T value;
    std::cin >> value;
    return value;
  }

  static void getCreatureInputs(Fishes &fishes, Monsters &monsters) {
    for (int i = 0, creature_count = read<int>(); i < creature_count; i++) {
      int creature_id = read<int>(), color = read<int>(), type = read<int>();
      if (type == -1) {
        monsters.addMonster(Monster(creature_id));
      } else {
        fishes.addFish(Fish(creature_id, color, type));
      }
    }
  }

  static void getScoreInputs(int &my_score, int &fo_score) {
    my_score = read<int>();
    fo_score = read<int>();
  }

  static void getSavedScanInputs(std::vector<int> &saved_scans) {
    for (int i = 0, saved_scan_count = read<int>(); i < saved_scan_count; i++) {
      saved_scans.push_back(read<int>());
    }
  }

  static void getDroneInputs(Drones &drones) {
    for (int i = 0, drone_count = read<int>(); i < drone_count; i++) {
      int drone_id = read<int>(), x = read<int>(), y = read<int>(),
          emergency = read<int>(), battery = read<int>();
      if (drones.size() < drone_count) {
        drones.addDrone(Drone(drone_id, x, y, emergency, battery));
      } else {
        drones.update(drone_id, x, y, emergency, battery);
        if (VERBOSE)
          std::cerr << "Drone: " << drone_id << " " << x << " " << y << " "
                    << emergency << " " << battery << std::endl;
      }
    }
  }

  static void getDroneScanInputs(Drones &myDrones, Drones &foDrones) {
    for (int i = 0, drone_scan_count = read<int>(); i < drone_scan_count; i++) {
      int drone_id = read<int>(), creature_id = read<int>();
      if (myDrones.contains(drone_id))
        myDrones.addToCacheScan(drone_id, creature_id);
      else
        foDrones.addToCacheScan(drone_id, creature_id);
    }
  }

  static void getVisibleCreatureInputs(Fishes &fishes, Monsters &monsters) {
    for (int i = 0, visible_creature_count = read<int>();
         i < visible_creature_count; i++) {
      int creature_id = read<int>(), x = read<int>(), y = read<int>(),
          vx = read<int>(), vy = read<int>();
      if (fishes.contains(creature_id)) {
        fishes.updateXY(creature_id, x, y, vx, vy);
      } else {
        monsters.updateXY(creature_id, x, y, vx, vy);
        if (VERBOSE)
          std::cerr << "Monster: " << creature_id << " " << x << " " << y << " "
                    << vx << " " << vy << std::endl;
      }
    }
  }

  static void getRadarBlipInputs(Drones &myDrones) {
    for (int i = 0, radar_count = read<int>(); i < radar_count; i++) {
      int drone_id = read<int>(), creature_id = read<int>();
      std::string radar = read<std::string>();
      myDrones.addRadarBlip(drone_id, creature_id, radar);
    }
  }
};

class PostProcessing {
public:
  static void updateFishes(Fishes &fishes, std::vector<int> &mySavedScans,
                           std::vector<int> &foSavedScans, Drones &myDrones,
                           Drones &foDrones) {
    for (auto &creature_id : mySavedScans) {
      fishes.fishMap.at(creature_id).caught.me.saved = true;
    }
    for (auto &creature_id : foSavedScans) {
      fishes.fishMap.at(creature_id).caught.fo.saved = true;
    }
    for (auto &[_, drone] : myDrones.droneMap) {
      for (auto &creature_id : drone.cachedScans) {
        fishes.fishMap.at(creature_id).caught.me.cached = true;
      }
    }
    for (auto &[_, drone] : foDrones.droneMap) {
      for (auto &creature_id : drone.cachedScans) {
        fishes.fishMap.at(creature_id).caught.fo.cached = true;
      }
    }
    for (auto &[creature_id, fish] : fishes.fishMap) {
      for (auto &[_, drone] : myDrones.droneMap) {
        if (!drone.fishOnRadar(creature_id))
          fish.lost = true;
      }
    }
    fishes.available = 0;
    fishes.caught = 0;
    fishes.cached = 0;
    fishes.lost = 0;
    for (auto &[_, fish] : fishes.fishMap) {
      if (!fish.caught.me.cached && !fish.caught.me.saved && !fish.lost)
        fishes.available++;
      if (fish.caught.me.saved)
        fishes.caught++;
      if (fish.caught.me.cached)
        fishes.cached++;
      if (fish.lost)
        fishes.lost++;
    }
    if (VERBOSE && false)
      std::cerr << "available: " << fishes.available << " | "
                << "caught: " << fishes.caught << " | "
                << "cached: " << fishes.cached << " | "
                << "lost: " << fishes.lost << std::endl;
  }
  static void updateDrones(const Fishes &fishes, Drones &myDrones) {
    for (auto &[drone_id, drone] : myDrones.droneMap) {
      drone.distanceToFish.clear();
      for (auto &[creature_id, fish] : fishes.fishMap) {
        if (fish.x != -1)
          drone.distanceToFish.emplace(
              creature_id,
              getDistance(Point{drone.x, drone.y}, Point{fish.x, fish.y}));
      }
    }
  }
};

class Game {
public:
  int frame = 0;
  int my_score = 0;
  int fo_score = 0;
  Fishes fishes = Fishes();
  Monsters monsters = Monsters();
  Drones myDrones = Drones();
  Drones foDrones = Drones();
  std::vector<int> mySavedScans, foSavedScans;
  std::unordered_map<std::string, int> moveStrategyCounter;

  void initialiseGame() { Input::getCreatureInputs(fishes, monsters); }

  void printMoveStrategyCounter() {
    std::cerr << std::endl;
    std::cerr << "Strategies Used" << std::endl;
    std::cerr << "---------------" << std::endl;
    for (auto &[strategy, count] : moveStrategyCounter) {
      std::cerr << strategy << ": " << count << std::endl;
    }
  }

  void addStrategyToStrategyCounter(std::string strategy) {
    if (moveStrategyCounter.find(strategy) == moveStrategyCounter.end()) {
      moveStrategyCounter.emplace(strategy, 1);
    } else {
      moveStrategyCounter.at(strategy)++;
    }
  }

  void initiliseFrame() {
    frame++;
    if (VERBOSE)
      std::cerr << "Frame: " << frame << std::endl;
    // reset objects
    myDrones.frameUpdate();
    foDrones.frameUpdate();
    fishes.frameUpdate();
    monsters.frameUpdate();

    // Get Inputs
    Input::getScoreInputs(my_score, fo_score); // current score
    Input::getSavedScanInputs(mySavedScans);   // no. of scans saved my drones
    Input::getSavedScanInputs(foSavedScans);   // no. of scans saved fo drones
    Input::getDroneInputs(myDrones);           // drone id & state my drones
    Input::getDroneInputs(foDrones);           // drone id & state fo drones
    Input::getDroneScanInputs(myDrones, foDrones);     // scans my + fo drones
    Input::getVisibleCreatureInputs(fishes, monsters); // visible creatures
    Input::getRadarBlipInputs(myDrones); // drone radar for my drones

    // Post Input Processing
    PostProcessing::updateFishes(fishes, mySavedScans, foSavedScans, myDrones,
                                 foDrones);
    PostProcessing::updateDrones(fishes, myDrones);
  }

  void calculateMove() {
    std::unordered_map<std::string, std::unordered_map<std::string, int>>
        params = {
          {"StrategySeperateDrones", {
            {"distanceThreshold", 250}
          }},
          {"StrategyScareAway", {
            {"distanceThreshold", 1500},
            {"distanceBuffer", 800}
          }},
          {"StrategyCashIn", {
            {"cacheThreshold4", 250},
            {"cacheThreshold6", 250}
          }},
          {"StrategySeekDenseFish", {
            {"monsterPenalty", 2},
            {"otherDronePenalty", 2},
            {"fishValueMultiplier", 0.2}
          }},
          {"StrategyNavigateMonsters", {
            {"anglesToSimulation", 6},
            {"vetoThreshold", 551},
            {"monsterDistanceThreshold", 601}
          }}
        };

    for (auto &drone_id : myDrones.droneOrder) {
      Drone &drone = myDrones.droneMap.at(drone_id);
      std::vector<Strategy *> strategies;
      strategies.push_back(new StrategySeperateDrones(
        myDrones, drone,
        params["StrategySeperateDrones"]["distanceThreshold"]
      ));
      strategies.push_back(new StrategyScareAway(
        drone, fishes, monsters, frame,
        params["StrategyScareAway"]["distanceThreshold"],
        params["StrategyScareAway"]["distanceBuffer"]
      ));
      strategies.push_back(new StrategyCashIn(
        drone, myDrones, fishes, monsters,
        params["StrategyCashIn"]["cacheThreshold4"],
        params["StrategyCashIn"]["cacheThreshold6"]
      ));
      strategies.push_back(new StrategyGoHome(
        drone, fishes
      ));
      strategies.push_back(new StrategySeekDenseFish(
        myDrones, drone, fishes, monsters, frame,
        params["StrategySeekDenseFish"]["monsterPenalty"],
        params["StrategySeekDenseFish"]["otherDronePenalty"],
        params["StrategySeekDenseFish"]["fishValueMultiplier"]
      ));
      strategies.push_back(new StrategySeekFoFish(
        drone, fishes
      ));
      strategies.push_back(new StrategyWait());


      Move bestMove;
      for (const auto &strat : strategies) {
        if (strat->check()) {
          bestMove = strat->getMove();
          break;
        }
      }

      StrategyEmergency strat(drone);
      if (strat.check()) {
        bestMove = strat.getMove();
      } else {
        StrategyNavigateMonsters strat(
            drone, monsters, bestMove,
            params["StrategyNavigateMonsters"]["anglesToSimulation"],
            params["StrategyNavigateMonsters"]["vetoThreshold"],
            params["StrategyNavigateMonsters"]["monsterDistanceThreshold"]);
        if (strat.check()) {
          bestMove = strat.getMove();
        }
      }

      bestMove.print();
      drone.appendMoveHistory(bestMove);
      myDrones.lightOn = bestMove.light;
      addStrategyToStrategyCounter(bestMove.strategy);
    }
  }
};

int main() {
  srand(0);
  Game game;
  game.initialiseGame();
  while (true) {
    game.initiliseFrame();
    game.calculateMove();
    game.printMoveStrategyCounter();
  }
  return 0;
};