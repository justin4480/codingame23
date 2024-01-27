import sys
import numpy as np
import pandas as pd
import time

pd.set_option('display.max_columns', None)

frame = 0
data = {'c': {'arr': None,
              'col': ['id', 'color', 'type', 'x', 'y', 'active',
                      'got_cid', 'got_color', 'got_type',
                      'p1', 'p2', 'p3', 'pt']},
        'd': {'arr': None,
              'col': ['drone_id', 'drone_x', 'drone_y', 'emergency', 'battery', 'most_east', 'most_south']},
        'r': {'arr': None,
              'col': ['drone_id', 'creature_id', 'radar_east', 'radar_south']}}

radar_east = {'TL': 0, 'TR': 1, 'BL': 0, "BR": 1}
radar_south = {'TL': 0, 'TR': 0, 'BL': 1, "BR": 1}

def p(message):
    print(message, file=sys.stderr, flush=True)

# ---------------------------------
# Get init inputs
# ---------------------------------

creature_count = int(input())
data['c']['arr'] = np.zeros((creature_count, len(data['c']['col'])), dtype=np.int16)
for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]
    data['c']['arr'][i, 0] = creature_id
    data['c']['arr'][i, 1] = color
    data['c']['arr'][i, 2] = _type

while True:
    frame += 1
    # ---------------------------------
    # Get round inputs
    # ---------------------------------

    start = time.perf_counter()
    my_score = int(input())
    foe_score = int(input())
    
    my_scan_count = int(input())
    for i in range(my_scan_count):
        creature_id = int(input())
    
    foe_scan_count = int(input())
    for i in range(foe_scan_count):
        creature_id = int(input())
    
    my_drone_count = int(input())
    data['d']['arr'] = np.zeros((my_drone_count, len(data['d']['col'])), dtype=np.int16)
    for i in range(my_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
        data['d']['arr'][i, 0] = drone_id
        data['d']['arr'][i, 1] = drone_x
        data['d']['arr'][i, 2] = drone_y
        data['d']['arr'][i, 3] = emergency
        data['d']['arr'][i, 4] = battery
    
    foe_drone_count = int(input())
    for i in range(foe_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
    
    drone_scan_count = int(input())
    for i in range(drone_scan_count):
        drone_id, creature_id = [int(j) for j in input().split()]
    
    visible_creature_count = int(input())
    for i in range(visible_creature_count):
        creature_id, creature_x, creature_y, creature_vx, creature_vy = [int(j) for j in input().split()]
        data['c']['arr'][data['c']['arr'][:, 0] == creature_id, 3] = creature_x
        data['c']['arr'][data['c']['arr'][:, 0] == creature_id, 4] = creature_y
    
    radar_blip_count = int(input())
    data['r']['arr'] = np.zeros((radar_blip_count, len(data['r']['col'])), dtype=np.int16)
    for i in range(radar_blip_count):
        inputs = input().split()
        drone_id, creature_id, radar = int(inputs[0]), int(inputs[1]), inputs[2]
        data['r']['arr'][i, 0] = drone_id
        data['r']['arr'][i, 1] = creature_id
        data['r']['arr'][i, 2] = radar_east[radar]
        data['r']['arr'][i, 3] = radar_south[radar]

    # ---------------------------------
    # Drone Transformations
    # ---------------------------------

    data['d']['arr'][0, 5] = np.where(data['d']['arr'][0, 1] >= data['d']['arr'][1, 1], 1, 0)
    data['d']['arr'][0, 6] = np.where(data['d']['arr'][0, 2] >= data['d']['arr'][1, 2], 1, 0)
    data['d']['arr'][1, 5] = 1 - data['d']['arr'][0, 5]
    data['d']['arr'][1, 6] = 1 - data['d']['arr'][0, 6]

    # ---------------------------------
    # Creature Transformations
    # ---------------------------------

    # active
    # lost = np.setdiff1d(data['c']['arr'][:, 0], data['r']['arr'][:, 1])
    data['c']['arr'][:, 5] = np.isin(data['c']['arr'][:, 0], data['r']['arr'][:, 1]).astype(np.int8)
    # got_cid
    data['c']['arr'][:, 6] = np.where(data['c']['arr'][:, 3] == 0, 0, 1)
    # got_color
    data['c']['arr'][:, 7] = pd.Series(data['c']['arr'][:, 2]).map(
        pd.DataFrame(data['c']['arr'][:, [2, 5]]).groupby(0).sum()[1].to_dict())
    # got_type
    data['c']['arr'][:, 8] = pd.Series(data['c']['arr'][:, 2]).map(
        pd.DataFrame(data['c']['arr'][:, [2, 5]]).groupby(0).sum()[1].to_dict())
    # p1 / p2 / p3 / pt
    data['c']['arr'][:, 9] = (data['c']['arr'][:, 2] + 1) * (1 - data['c']['arr'][:, 6]) * data['c']['arr'][:, 5]
    data['c']['arr'][:, 10] = 3 * (data['c']['arr'][:, 7] == 0) * data['c']['arr'][:, 5]
    data['c']['arr'][:, 11] = 4 * (data['c']['arr'][:, 8] == 0) * data['c']['arr'][:, 5]
    data['c']['arr'][:, 12] = np.sum(data['c']['arr'][:, 9:12], axis=1)
    # sort array descending based on pt
    # data['c']['arr'] = data['c']['arr'][np.argsort(data['c']['arr'][:, 12])]
    # sort array descending based on type
    data['c']['arr'] = data['c']['arr'][np.lexsort((
        +data['c']['arr'][:, 12],
        # -data['c']['arr'][:, 2],
        -data['c']['arr'][:, 6],
        +data['c']['arr'][:, 5],
    ))]

    # ---------------------------------
    # Verbose
    # ---------------------------------

    if True:
        # p(pd.DataFrame(data['d']['arr'], columns=data['d']['col']))
        p(pd.DataFrame(data['c']['arr'], columns=data['c']['col']).drop(['p1', 'p2', 'p3'], axis=1))
        # p(pd.DataFrame(data['r']['arr'], columns=data['r']['col']))
    
    # ---------------------------------
    # Get targets and allocate
    # ---------------------------------
    
    targets = data['c']['arr'][-2:, 0]
    drones = data['d']['arr'][:, 0]
    p(f'{targets=}')

    target1_east, target1_south = data['r']['arr'][1==1
        & (data['r']['arr'][:, 1] == targets[0])
        & (data['r']['arr'][:, 0] == drones[0]), -2:][0]
    
    target2_east, target2_south = data['r']['arr'][1==1
        & (data['r']['arr'][:, 1] == targets[1])
        & (data['r']['arr'][:, 0] == drones[1]), -2:][0]

    # p(f'{target1_east=} {target1_south=}')
    drone1_x, drone1_y = data['d']['arr'][0, 1:3]
    # p(f'{drone1_x=} {drone1_y=}')
    drone1_x += +1000 if target1_east else -1000
    drone1_y += +1000 if target1_south else -1000
    # p(f'{drone1_x=} {drone1_y=}')

    # p(f'{target2_east=} {target2_south=}')
    drone2_x, drone2_y = data['d']['arr'][1, 1:3]
    # p(f'{drone2_x=} {drone2_y=}')
    drone2_x += +1000 if target2_east else -1000
    drone2_y += +1000 if target2_south else -1000
    # p(f'{drone2_x=} {drone2_y=}')

    instruction1 = f'MOVE {drone1_x} {drone1_y} {int(frame + 3 % 5 == 0)}'
    instruction2 = f'MOVE {drone2_x} {drone2_y} {int(frame + 0 % 5 == 0)}'

    p(instruction1)
    p(instruction2)

    print(instruction1)
    print(instruction2)

    # ---------------------------------
    # Performance
    # ---------------------------------
    
    end = time.perf_counter()
    p(f'time: {np.round(end - start, 4)}')
    # Performance
    # ---------------------------------
    
    end = time.perf_counter()
    p(f'time: {np.round(end - start, 4)}')
