import sys
import math

class Drone:
    def __init__(self, path):
        self.path = path
    
    def update_xy(self, x, y):
        self.x = x
        self.y = y
        if len(self.path) > 0:
            x, y = self.path[0]
            if self.x == x and self.y == y:
                self.path.pop(0)
    
    def get_move(self):
        if len(self.path) > 0:
            x, y = self.path[0]
            return f'MOVE {x} {y} 1'
        else:
            return f'MOVE 5000 500 1'
path1 = [
    (300, 1500), (300, 9500),
    (5000, 9500), (5000, 9000), (1000, 9000),
    
    (2000, 9500), (2000, 300), (1000, 9500),
    (3000, 9500), (3000, 300), (1000, 9500),
    (4000, 9500), (4000, 300), (1000, 9500),
    (5000, 9500), (5000, 300), (1000, 9500),
]

path2 = [(9999-i, j) for i, j in path1]

drones = [
    Drone(path=path1),
    Drone(path=path2),
]

def p(msg):
    print(msg, file=sys.stderr, flush=True)

creature_count = int(input())
for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]

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
    radar_blip_count = int(input())
    for i in range(radar_blip_count):
        inputs = input().split()
        drone_id = int(inputs[0])
        creature_id = int(inputs[1])
        radar = inputs[2]
    for i in range(my_drone_count):
        print(drones[i].get_move())
