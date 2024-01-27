import sys
import math
import pandas as pd
import numpy as np


def euclidean_distance(point1, point2):
    return int(np.sqrt((point1[0] - point2[0])**2 + (point1[1] - point2[1])**2))

    
class Drone:
    def __init__(self, path):
        self.path_backup = path
        self.path = path
        self.detour_x = None
        self.detour_y = None
    
    def update_xy(self, x, y):
        self.x = x
        self.y = y
        self.update_path()
    
    def next_point(self):
        if len(self.path) > 0:
            self.path.pop(0)
        else:
            self.path = self.path_backup
    
    def update_path(self):
        if self.x == self.path[0][0] and self.y == self.path[0][1]:
            self.next_point()

    def calculate_move(self, creatures):
        self.detour_x = None
        self.detour_y = None
        monsters = creatures.loc[creatures['type'] == -1, ['x', 'y']].dropna()
        monster_distance = [euclidean_distance([self.x, self.y], monster) for monster in np.array(monsters)]
        target_distance = euclidean_distance([self.x, self.y], self.path[0])
        if len(monsters) > 0:
            p(f"{self.x=} {self.y=}\n{self.path[0]=}\n{monster_distance=} {target_distance=}")
            if np.all(np.array(monster_distance) > 2000):
                p('monster not within 2,000')
            else:
                # if target_distance < 2000:
                #     self.next_point()
                #     p('skip to next waypoint')
                p('calculated detour')
                target_vector = np.array(self.path[0], dtype=float) - np.array([self.x, self.y], dtype=float)
                monster_vectors = np.array(monsters, dtype=float) - np.array([self.x, self.y], dtype=float)
                dot_products = np.dot(monster_vectors, target_vector)
                most_aligned_monster_index = np.argmax(dot_products)
                west_or_east = 1 if self.x < 5000 else -1
                detour_vector = np.array([
                    west_or_east * monster_vectors[most_aligned_monster_index, 1],
                    monster_vectors[most_aligned_monster_index, 0]
                ], dtype=float)
                if True:
                    detour_vector /= euclidean_distance([0, 0], detour_vector) / euclidean_distance([0, 0], target_vector)
                else:
                    detour_vector = np.array([target_vector[1], -target_vector[0]], dtype=float)
                    detour_vector /= np.linalg.norm(detour_vector)
                detour_position = np.array([self.x, self.y], dtype=float) + detour_vector
                self.detour_x, self.detour_y = detour_position
                self.detour_x = int(np.clip(self.detour_x, 0, 9999))
                self.detour_y = int(np.clip(self.detour_y, 0, 9999)) 


    def get_move(self):
        if self.detour_x is not None:
            return f'MOVE {self.detour_x} {self.detour_y} 0'
        else:
            x, y = self.path[0]
            return f'MOVE {x} {y} 1'


path1 = [
    (1000, 1500), (1000, 9500), (1000, 9500), (1000, 500),
    (5000, 500),
    (5000, 9500), (5000, 9000), (1000, 9000),    
    (2000, 9500), (2000, 300), (1000, 9500),
    (3000, 9500), (3000, 300), (1000, 9500),
    (4000, 9500), (4000, 300), (1000, 9500),
    (5000, 9500), (5000, 300), (1000, 9500),
]
path2 = [(10000-i, j) for i, j in path1]
drones = [Drone(path1), Drone(path2)]

creatures = pd.DataFrame(columns=['creature_id', 'color', 'type', 'x', 'y'])

def p(msg):
    print(msg, file=sys.stderr, flush=True)

creature_count = int(input())
for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]
    creatures.loc[creature_id, 'color'] = color
    creatures.loc[creature_id, 'type'] = _type

frame = 0
while True:
    frame += 1
    my_score = int(input())
    foe_score = int(input())
    my_scan_count = int(input())
    for i in range(my_scan_count):
        creature_id = int(input())
    foe_scan_count = int(input())
    for i in range(foe_scan_count):
        creature_id = int(input())
    my_drone_count = int(input())
    for i in range(my_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
        drones[i].update_xy(drone_x, drone_y)
    if frame == 1 and drones[0].x > drones[1].x:
        drones = drones[::-1]
    foe_drone_count = int(input())
    for i in range(foe_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
    drone_scan_count = int(input())
    for i in range(drone_scan_count):
        drone_id, creature_id = [int(j) for j in input().split()]
    visible_creature_count = int(input())
    for i in range(visible_creature_count):
        creature_id, creature_x, creature_y, creature_vx, creature_vy = [int(j) for j in input().split()]
        creatures.loc[creature_id, 'x']  = creature_x
        creatures.loc[creature_id, 'y']  = creature_y
    radar_blip_count = int(input())
    for i in range(radar_blip_count):
        inputs = input().split()
        drone_id = int(inputs[0])
        creature_id = int(inputs[1])
        radar = inputs[2]
    # Print monsters
    p(creatures.loc[creatures['type'] == -1, ['x', 'y']].dropna())
    for i in range(my_drone_count):
        drones[i].calculate_move(creatures)
        print(drones[i].get_move())
