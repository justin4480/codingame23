import sys
import numpy as np

def p(msg):
    print(msg, file=sys.stderr, flush=True)

creature_count = int(input())
p(creature_count)
for i in range(creature_count):
    inputs = input().split()
    p(inputs)
    creature_id, color, _type = [int(j) for j in inputs]

while True:
    my_score = int(input())
    foe_score = int(input())
    my_scan_count = int(input())
    p(my_score)
    p(foe_score)
    p(my_scan_count)
    for i in range(my_scan_count):
        creature_id = int(input())
        p(creature_id)
    foe_scan_count = int(input())
    p(foe_scan_count)
    for i in range(foe_scan_count):
        creature_id = int(input())
        p(creature_id)
    my_drone_count = int(input())
    p(my_drone_count)
    for i in range(my_drone_count):
        inputs = input().split()
        p(inputs)
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in inputs]
    foe_drone_count = int(input())
    p(foe_drone_count)
    for i in range(foe_drone_count):
        inputs = input().split()
        p(inputs)
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in inputs]
    drone_scan_count = int(input())
    p(drone_scan_count)
    for i in range(drone_scan_count):
        inputs = input().split()
        p(inputs)
        drone_id, creature_id = [int(j) for j in inputs]
    visible_creature_count = int(input())
    p(visible_creature_count)
    for i in range(visible_creature_count):
        inputs = input().split()
        p(inputs)
        creature_id, creature_x, creature_y, creature_vx, creature_vy = [int(j) for j in inputs]
    radar_blip_count = int(input())
    p(radar_blip_count)
    for i in range(radar_blip_count):
        inputs = input().split()
        p(inputs)
        drone_id = int(inputs[0])
        creature_id = int(inputs[1])
        radar = inputs[2]
    for i in range(my_drone_count):
        print(f"WAIT {np.random.randint(0,2)}")
