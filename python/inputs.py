import sys
import numpy as np

def p(msg):
    print(msg, file=sys.stderr, flush=True)

creature_count = input()
p(creature_count)
[p(input()) for i in range(int(creature_count))]

while True:
    my_score = input()
    p(my_score)

    foe_score = input()
    p(foe_score)

    my_scan_count = input()
    p(my_scan_count)

    [p(input()) for i in range(int(my_scan_count))]

    foe_scan_count = input()
    p(foe_scan_count)
    [p(input()) for i in range(int(foe_scan_count))]

    my_drone_count = input()
    p(my_drone_count)
    [p(input()) for i in range(int(my_drone_count))]

    foe_drone_count = input()
    p(foe_drone_count)
    [p(input()) for i in range(int(foe_drone_count))]

    drone_scan_count = input()
    p(drone_scan_count)
    [p(input()) for i in range(int(drone_scan_count))]

    visible_creature_count = input()
    p(visible_creature_count)
    [p(input()) for i in range(int(visible_creature_count))]

    radar_blip_count = input()
    p(radar_blip_count)
    [p(input()) for i in range(int(radar_blip_count))]

    for i in range(int(my_drone_count)):
        print(f"WAIT {np.random.randint(0,2)}")
