import sys
import math
import numpy as np
import pandas as pd
from time import perf_counter

pd.set_option('display.max_columns', None)
def p(message):
    print(message, file=sys.stderr, flush=True)

VERBOSE = 0

VERBOSE and print(f'Start Game', file=sys.stderr, flush=True)
px = 10000
counter = 0
creature_count = int(input())

df_init = pd.DataFrame(columns=['color', 'type', 'me', 'foe', 'scan_x', 'scan_y', 'radar', 'radar_x', 'radar_y'])
for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]
    df_init.loc[creature_id, ['color', 'type']] = f'c{color}', f't{_type}'
    df_init.loc[:, 'me'] = 0

while True:
    counter = counter + 1
    print(f'{counter=}', file=sys.stderr, flush=True)
    t = perf_counter()
    df = df_init.copy()

    my_score = int(input())
    foe_score = int(input())
    VERBOSE and print(f'Start Loop', file=sys.stderr, flush=True)
    
    my_scan_count = int(input())
    # df.loc[:, 'me'] = 0
    for i in range(my_scan_count):
        creature_id = int(input())
        df.loc[creature_id, 'me'] = 1
    
    foe_scan_count = int(input())
    df.loc[:, 'foe'] = 0
    for i in range(foe_scan_count):
        creature_id = int(input())
        df.loc[creature_id, 'foe'] = 1
    
    my_drone_count = int(input())
    for i in range(my_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
        my_x, my_y = drone_x, drone_y
        if my_y <= 500:
            counter = 0
    
    foe_drone_count = int(input())
    for i in range(foe_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
    
    drone_scan_count = int(input())
    for i in range(drone_scan_count):
        drone_id, creature_id = [int(j) for j in input().split()]
    
    visible_creature_count = int(input())
    for i in range(visible_creature_count):
        creature_id, creature_x, creature_y, creature_vx, creature_vy = [int(j) for j in input().split()]
        # df.loc[:, ['x', 'y', 'vx', 'vy']] = creature_x, creature_y, creature_vx, creature_vy
        df.loc[creature_id, ['scan_x', 'scan_y']] = creature_x, creature_y
    
    radar_blip_count = int(input())
    radar_x = {'TL': -5000, 'TR': +5000, 'BL': -5000, 'BR': +5000}
    radar_y = {'TL': -5000, 'TR': -5000, 'BL': +5000, 'BR': +5000}
    for i in range(radar_blip_count):
        inputs = input().split()
        drone_id = int(inputs[0])
        creature_id = int(inputs[1])
        radar = inputs[2]

        df.loc[creature_id, 'radar'] = radar
        df.loc[creature_id, 'radar_x'] = max(0, min(9999, my_x + radar_x[radar]))
        df.loc[creature_id, 'radar_y'] = max(0, min(9999, my_y + radar_y[radar]))
        VERBOSE and print(f'{creature_id} {my_x=} {radar_x[radar]=} {my_y=} {radar_y[radar]=}', file=sys.stderr, flush=True)
    
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} inputs', file=sys.stderr, flush=True)

    df_type_agg = (
        df.groupby('type')
        .agg({'type': 'count', 'me': 'sum', 'foe': 'sum'})
        .assign(t_n=lambda x: x.type)
        .assign(t_me=lambda x: x.me)
        .assign(t_foe=lambda x: x.foe)
        .assign(t_points_all=lambda x: 1
            * np.where(x.t_me < x.t_n, 4, 0)
            * np.where(x.t_foe < x.t_n, 1, 2)
        )
        .assign(t_points_one=lambda x: 1
            * x.index.map({'t0':1, 't1':2, 't2':3})
            * np.where(x.t_me < x.t_n, 1, 0)
            * np.where(x.t_foe < x.t_n, 1, 2)
        )
        .loc[:, ['t_n', 't_me', 't_foe', 't_points_one', 't_points_all']]
    )
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} df_type_agg', file=sys.stderr, flush=True)

    df_color_agg = (
        df.groupby('color')
        .agg({'color': 'count', 'me': 'sum', 'foe': 'sum'})
        .assign(c_n=lambda x: x.color)
        .assign(c_me=lambda x: x.me)
        .assign(c_foe=lambda x: x.foe)
        .assign(c_points_all=lambda x: 1
            * np.where(x.c_me < x.c_n, 3, 0)
            * np.where(x.c_foe < x.c_n, 1, 2)
        )
        .loc[:, ['c_n', 'c_me', 'c_foe', 'c_points_all']]
    )
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} df_color_agg', file=sys.stderr, flush=True)

    df = (
        df
        .merge(df_type_agg, left_on='type', right_index=True)
        .merge(df_color_agg, left_on='color', right_index=True)
        .assign(x=lambda x: np.where(pd.isna(x.scan_x), x.radar_x, x.scan_x))
        .assign(y=lambda x: np.where(pd.isna(x.scan_y), x.radar_y, x.scan_y))
        .assign(distance=lambda x: ((x.x - my_x)**2 + (x.y - my_y)**2) ** 0.5)
        .assign(distance_norm=lambda x: x.distance / 10000)
        .assign(points_t_one=lambda x: x.t_points_one * (1-x.me))
        .assign(points_t_all=lambda x: x.t_points_all * (1-x.me))
        .assign(points_c_all=lambda x: x.c_points_all * (1-x.me))
        .assign(t_frac=lambda x: (x.t_me + 1) / x.t_n)
        .assign(c_frac=lambda x: (x.c_me + 1) / x.c_n)
        .assign(points=lambda x: 0
            + (x.points_t_one * 1)
            + (x.points_t_all * x.t_frac)
            + (x.points_c_all * x.c_frac)
            + (-1000000 * x.me)
        )
    )
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} df assign', file=sys.stderr, flush=True)
    
    caught = df.query('distance <= 2000').index
    p(caught)
    if len(caught) > 0:
        p('added')
        df_init.loc[caught, 'me'] = 1

    w = [1, -1]
    X = df.loc[:, ['points', 'distance']]
    df.loc[:, 'score'] = X.dot(w)
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} X.dot(w)', file=sys.stderr, flush=True)

    # best_id = df.score.argmax()
    best_id = df.index[df.score == df.score.max()][0]
    best_x, best_y = df.loc[best_id, ['radar_x', 'radar_y']].astype(int).values
    light = 1 if df.distance.min() < 2000 else 0
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} get best', file=sys.stderr, flush=True)

    for i in range(my_drone_count):
        logs = df.sort_index().loc[:, [
            'me', 'x', 'y', 'scan_x', 'scan_y', 'radar_x', 'radar_y',
            # 'type', 't_me', 't_n', 't_frac', 'points_t_all',
            # 'color', 'c_me', 'c_n', 'c_frac', 'points_c_all',
            'distance', 'points', 'score',
        ]]
        print(logs, file=sys.stderr, flush=True)
        print(f'{best_id=} {best_x=} {best_y=}', file=sys.stderr, flush=True)
        if False:
            move = 'MOVE 4000 4000 0'
        elif counter > 50:
            move = (f'MOVE {drone_x} 500 0')
        else:
            move = (f'MOVE {best_x} {best_y} {light}')
        print(move, file=sys.stderr, flush=True)
        print(move)
    VERBOSE and print(f'{np.round(perf_counter() - t, 4)} End loop', file=sys.stderr, flush=True)
