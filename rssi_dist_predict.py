import numpy as np

# ---- trained weights ----

W1 = np.array([[
    4.246597907730787,
    -8.213340121512017,
    0.439348204154549,
    0.9229701655205426,
    -12.41035435258382
]])

b1 = np.array([
    -5.9747660086978485,
    3.3450393750223535,
    -0.7674117247494263,
    -3.0098816220105555,
    2.2622246243718647
])

W2 = np.array([
    [12.761656843882118],
    [4.67526594532535],
    [-2.8485643158576623],
    [-13.641019321269322],
    [-1.8993355204610496]
])

b2 = np.array([0.20820478751424418])


def sigmoid(x):
    return 1 / (1 + np.exp(-x))


def predict_distance(rssi_value):

    # SAME NORMALIZATION AS TRAINING
    x_norm = (rssi_value - (-100.0)) / (0.0 - (-100.0))
    x_norm = np.clip(x_norm, 0, 1)

    X = np.array([[x_norm]])

    # forward pass
    z1 = np.dot(X, W1) + b1
    z1 = np.clip(z1, -50, 50)   # stability
    a1 = sigmoid(z1)

    z2 = np.dot(a1, W2) + b2

    return float(z2[0][0])


# test

for i in range(60, 70):
    print(predict_distance(-i))