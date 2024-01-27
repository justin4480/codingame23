import sys
import math
import numpy as np
import pandas as pd
from time import perf_counter

pd.set_option('display.max_columns', None)
def p(message):
    print(message, file=sys.stderr, flush=True)

def time(t=None, message=''):
    if t:
        p(f'{np.round(t - perf_counter(), 4)} {message}')
    return perf_counter()

px = 10000
# counter = 0
creature_count = int(input())

df_init = pd.DataFrame(columns=['color', 'type', 'me', 'foe', 'scan_x', 'scan_y', 'radar', 'radar_x', 'radar_y'])
for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]
    df_init.loc[creature_id, ['color', 'type']] = color, _type
    df_init.loc[:, 'me'] = 0

while True:
    start = perf_counter()
    t = time()
    drones = []
    best_id = -1
    df = df_init.copy()

    my_score = int(input())
    foe_score = int(input())
    
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
        drones.append({'drone_id': drone_id, 'drone_x': drone_x, 'drone_y': drone_y, 'emergency': emergency, 'battery': battery})
        # my_x, my_y = drone_x, drone_y
        # if my_y <= 500:
        #     counter = 0
    
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
        df.loc[creature_id, 'radar_x'] = radar_x[radar]
        df.loc[creature_id, 'radar_y'] = radar_y[radar]

    df2 = df.copy()

    for drone in drones:
        p(f"{drone['drone_id']=}")
        t = time(t, 'start loop')
        
        df = df2.copy()
        t = time(t, ' - df copy')

        df_type_agg = (
            df.groupby('type')
            .agg({'type': 'count', 'me': 'sum', 'foe': 'sum'})
            .rename(columns={'type': 't_n', 'me': 't_me', 'foe': 't_foe'})
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
        t = time(t, ' - type agg')

        df_color_agg = (
            df.groupby('color')
            .agg({'color': 'count', 'me': 'sum', 'foe': 'sum'})
            .rename(columns={'color': 'c_n', 'me': 'c_me', 'foe': 'c_foe'})
            .assign(c_points_all=lambda x: 1
                * np.where(x.c_me < x.c_n, 3, 0)
                * np.where(x.c_foe < x.c_n, 1, 2)
            )
            .loc[:, ['c_n', 'c_me', 'c_foe', 'c_points_all']]
        )
        # p(' - color agg')
        t = time(t, ' - color agg')
        
        df = df.merge(df_type_agg, left_on='type', right_index=True)
        df = df.merge(df_color_agg, left_on='color', right_index=True)
        df['radar_x'] = drone['drone_x'] + df['radar_x']
        df['radar_y'] = drone['drone_y'] + df['radar_y']
        df['x'] = np.where(pd.isna(df['scan_x']), df['radar_x'], df['scan_x'])
        df['y'] = np.where(pd.isna(df['scan_y']), df['radar_y'], df['scan_y'])
        df['distance'] = (np.square(df['x'] - drone['drone_x']) + np.square(df['y'] - drone['drone_y']) ** 0.5)
        df['distance_norm'] = df['distance'] / 10000
        df['points_t_one'] = df['t_points_one'] * (1 - df['me'])
        df['points_t_all'] = df['t_points_all'] * (1 - df['me'])
        df['points_c_all'] = df['c_points_all'] * (1 - df['me'])
        df['t_frac'] = (df['t_me'] + 1) / df['t_n']
        df['c_frac'] = (df['c_me'] + 1) / df['c_n']
        df['points'] = (
            (df['points_t_one'] * 1) +
            (df['points_t_all'] * df['t_frac']) +
            (df['points_c_all'] * df['c_frac']) +
            (-1000000 * df['me'])
        )
        t = time(t, ' - assigns')
        
        w = [1, -1]
        X = df.loc[:, ['points', 'distance_norm']]
        df.loc[:, 'score'] = X.dot(w)
        # p(' - X.dot(w)')
        t = time(t, ' - X.dot(w)')

        if best_id > 0:
            df.loc[best_id, 'score'] =- 1000000

        # best_id = df.score.argmax()
        best_id = df.index[df.score == df.score.max()][0]
        best_x, best_y = df.loc[best_id, ['radar_x', 'radar_y']].astype(int).values
        light = 1 if df.distance.min() < 2000 else 0
        # p(' - best')
        t = time(t, ' - best')
        
        caught = df.query('distance <= 2000').index
        if len(caught) > 0:
            df_init.loc[caught, 'me'] = 1
        # p(' - caught')
        t = time(t, ' - caught')

        # logs = df.sort_index().loc[:, [
        #     'me', 'x', 'y',
        #     # 'scan_x', 'scan_y', 'radar_x', 'radar_y',
        #     # 'type', 't_me', 't_n', 't_frac', 'points_t_all',
        #     # 'color', 'c_me', 'c_n', 'c_frac', 'points_c_all',
        #     'distance', 'points', 'score',
        # ]]
        # p(f' ** {perf_counter() - t} **')
        move = (f'MOVE {best_x} {best_y} {light}')
        # print(logs, file=sys.stderr, flush=True)
        # print(f'{best_id=} {best_x=} {best_y=}', file=sys.stderr, flush=True)
        # print(move, file=sys.stderr, flush=True)
        print(move)
    end = perf_counter()
    p(f'Final Timing: {end - start}')
