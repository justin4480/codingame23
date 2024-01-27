import sys
import math
import numpy as np
import pandas as pd
from sklearn.preprocessing import MinMaxScaler 

creature_count = int(input())

a = np.zeros((creature_count, 9))

for i in range(creature_count):
    creature_id, color, _type = [int(j) for j in input().split()]
    a[i, 0:3] = creature_id, color,_type

while True:
    my_score = int(input())
    foe_score = int(input())
    
    my_scan_count = int(input())
    for i in range(my_scan_count):
        creature_id = int(input())
        a[a[:, 0] == creature_id, 3] = 1

    foe_scan_count = int(input())
    for i in range(foe_scan_count):
        creature_id = int(input())
        a[a[:, 0] == creature_id, 4] = 1
    
    my_drone_count = int(input())
    for i in range(my_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
        my_x = drone_x
        my_y = drone_y
    
    foe_drone_count = int(input())
    for i in range(foe_drone_count):
        drone_id, drone_x, drone_y, emergency, battery = [int(j) for j in input().split()]
    
    drone_scan_count = int(input())
    for i in range(drone_scan_count):
        drone_id, creature_id = [int(j) for j in input().split()]
    
    visible_creature_count = int(input())
    for i in range(visible_creature_count):
        creature_id, creature_x, creature_y, creature_vx, creature_vy = [int(j) for j in input().split()]
        a[a[:, 0] == creature_id, 5:9] = creature_x, creature_y, creature_vx, creature_vy
    
    radar_blip_count = int(input())
    for i in range(radar_blip_count):
        inputs = input().split()
        drone_id = int(inputs[0])
        creature_id = int(inputs[1])
        radar = inputs[2]

    df = pd.DataFrame(
        a.astype(int),
        columns=['id', 'color', 'type', 'me', 'foe', 'x', 'y', 'vx', 'vy'],
    )
    
    df_agg = (
        df.groupby('type')
        .agg({'me': 'sum', 'foe': 'sum'})
        .assign(type_me=lambda x: x.me > 0)
        .assign(type_foe=lambda x: x.foe > 0)
        .drop(['me', 'foe'], axis=1)
    )
    df = df.merge(df_agg, left_on='type', right_index=True)
    df = (
        df
        .assign(const=1)
        .assign(distance=lambda x: ((x.x-my_x)**2 + (x.y-my_y)**2) ** 0.5)
        .assign(value=lambda x: 1 - x.type_me)
    )
    X = df.loc[df['value'] > 0, ['id', 'const', 'value', 'distance']]
    X['distance'] = MinMaxScaler().fit_transform(X[['distance']])
    w = [1, -100, -10, -1]
    X['y'] = X.dot(w)

    best_id = X.loc[X.y.idxmax(), 'id']

    best_x, best_y = df.loc[df.id == best_id, ['x', 'y']].values[0]
    print(X, file=sys.stderr, flush=True)
    
    for i in range(my_drone_count):
        print(f'MOVE {best_x} {best_y} 0')
