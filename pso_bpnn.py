import numpy as np
import pandas as pd
import pyswarms as ps
import tensorflow as tf
import matplotlib.pyplot as plt

global best_weights

# Getting data

def prepare_data(file_path):

    df = pd.read_csv(file_path)

    # Use filtered RSSI if available
    if "RSSI_filtered" in df.columns:
        rssi = df["RSSI_filtered"].values
        print("Using FILTERED RSSI for training")
    else:
        rssi = df["RSSI_dBm"].values
        print("Using RAW RSSI for training")

    # Fixed normalization range : Normalize [-100, 0] to [0, 1]
    X = (rssi - (-100.0)) / (0.0 - (-100.0))
    X = np.clip(X, 0, 1).reshape(-1, 1)

    y = df['Distance_m'].values.reshape(-1, 1)

    return X, y


# Making predictions based on the particle

def forward_propagation(params, X):

    W1 = params[0:5].reshape((1, 5))
    b1 = params[5:10].reshape((5,))
    W2 = params[10:15].reshape((5, 1))
    b2 = params[15:16].reshape((1,))
 
    # Hidden layer (sigmoid)
    z1 = np.dot(X, W1) + b1

    # Numerical stability improvement
    z1 = np.clip(z1, -50, 50)

    a1 = 1 / (1 + np.exp(-z1))

    y_pred = np.dot(a1, W2) + b2

    print("Prediced", y_pred)

    return y_pred


# 3. Fitness function to score particles

def fitness_function(particles, X, y):

    n_particles = particles.shape[0]
    mse_list = []

    for i in range(n_particles):
        pred = forward_propagation(particles[i], X)
        mse = np.mean((pred - y) ** 2)
        mse_list.append(mse)

    return np.array(mse_list)



# 4. Training execution

def train_model(X, y):

    options = {
        'c1': 1.5,
        'c2': 2.0,
        'w': 0.7
    }

    dimensions = 16

    optimizer = ps.single.GlobalBestPSO(
        n_particles=50,
        dimensions=dimensions,
        options=options
    )

    cost, best_pos = optimizer.optimize(
        fitness_function,
        iters=1000,
        X=X,
        y=y
    )

    return cost, best_pos

try:

    X_train, y_train = prepare_data('rssi_filtered_training_data.csv')

    best_cost, best_weights = train_model(X_train, y_train)

    # print("\n" + "="*30)
    # print("Training complete")
    # print(f"Minimum MSE: {best_cost:.4f}")
    # print("="*30)

    # print(f"float W1[1][5] = {{{', '.join(map(str, best_weights[0:5]))}}};")
    # print(f"float b1[5]    = {{{', '.join(map(str, best_weights[5:10]))}}};")
    # print(f"float W2[5][1] = {{{', '.join(map(str, best_weights[10:15]))}}};")
    # print(f"float b2[1]    = {{{best_weights[15]}}};")


    print(best_weights)

    # defining the architecture 

    model = tf.keras.Sequential([
        tf.keras.layers.Dense(5, activation='tanh', input_shape=(1,), name='hidden'),
        tf.keras.layers.Dense(1, activation='linear', name='output')
    ])

    # extracting the intial parameters from the best particle

    w1 = best_weights[0:5].reshape((1, 5))
    b1 = best_weights[5:10]
    w2 = best_weights[10:15].reshape((5, 1))
    b2 = best_weights[15:16]

    model.layers[0].set_weights([w1, b1]) # Setting hidden Layer
    model.layers[1].set_weights([w2, b2]) # Setting output Layer

    model.compile(optimizer=tf.keras.optimizers.SGD(learning_rate=0.01), loss='mse')

    print("Starting Backpropagation")
    model.fit(X_train, y_train, epochs=200, verbose=0) # Fewer epochs needed now


    # final weights and biases
    final_w1, final_b1 = model.layers[0].get_weights()
    final_w2, final_b2 = model.layers[1].get_weights()


except FileNotFoundError:
    print("Error: file was not found.")