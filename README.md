# ESP32/12E  Indoor Localization System

This project implements a real-time indoor positioning system using a network of **three Anchor nodes** (ESP12E) and **one Receiver node** (ESP32). It features two distinct mathematical approaches to solve the localization problem based on Wi-Fi RSSI (Received Signal Strength Indicator) data.

## System Architecture

The system operates on **ESP-NOW**, a connectionless protocol that allows high-frequency packet exchange with minimal overhead.
### 1. The Anchors (ESP12E)
- Stationary nodes placed at known positions.
- Continuously broadcast packets containing their unique `ANCHOR_ID`.
- Operate on a fixed Wi-Fi channel (Channel 1) for consistency.

### 2. The Receiver (ESP32)
- Collects RSSI data from all three anchors simultaneously.
- Applies a **Kalman Filter** to smooth the noisy raw RSSI signals.
- Processes the filtered data through one of two solutions to estimate its current position.
---

##  Solution 1: ML-Enhanced Trilateration

This is a high-precision approach that converts signal strength into physical distance using a Neural Network.

- **Distance Estimation:** Uses a **BPNN (Backpropagation Neural Network)**.
    
- **Optimization:** The network weights are pre-optimized using **PSO (Particle Swarm Optimization)** to avoid local minima and ensure the RSSI-to-Distance mapping is accurate.
    
- **Trilateration:** Solves the system of equations representing three intersecting circles to find the exact $(x, y)$ coordinate.
    
## Solution 2: Weighted Vector Approach

A robust, computationally efficient alternative that avoids complex distance modeling.
- **Logic:** Instead of calculating distances, it treats each anchor as a "source of gravity."

- **Weighting:** Assigns a weight to each anchor using an exponential function: $w = e^{(RSSI + 90)/10}$.    
- **Calculation:** The position is the **weighted average** of the anchors' coordinates. Stronger signals pull the estimated position closer to that specific anchor.
    

---
##  Technical Stack

| **Component**        | **Technology**                             |
| -------------------- | ------------------------------------------ |
| **Microcontrollers** | ESP32 (Receiver), ESP8266/ESP12E (Anchors) |
| **Communication**    | ESP-NOW (2.4GHz)                           |
| **Filtering**        | Kalman Filter (State-space estimation)     |
| **ML Training**      | Python, TensorFlow, PySwarms (PSO), NumPy  |
| **Geometry**         | Linear Algebra (Trilateration)             |

---

##  Code Implementation Details

### Kalman Filter Parameters

To handle the jitter of Wi-Fi signals, the code uses:
- **Process Noise (Q):** 0.05
- **Measurement Noise (R):** 9.0 (High R reflects the high volatility of RSSI)
    
### PSO-BPNN Training

The Python script provided (`prepare_data`, `train_model`) performs the following:

1. **Normalization:** Scales RSSI from $[-100, 0]$ to $[0, 1]$.
2. **PSO Optimization:** 50 particles iterate 1000 times to find the best weights for a 5-neuron hidden layer.
3. **Refinement:** Uses Stochastic Gradient Descent (SGD) to fine-tune the PSO-discovered weights.
    
---
##  How to Use

1. **Setup Anchors:** Update the `ANCHOR_ID` (1, 2, or 3) in the anchor code and flash three ESP12E modules. Place them at $(0,0)$, $(0, 0.7)$, and $(0.7, 0)$.
    
2. **Calibrate (Optional):** Use the Python script to train the Neural Network if your environment has unique interference patterns.
    
3. **Flash Receiver:** Update the `anchors` MAC addresses in the ESP32 code to match your specific hardware.
    
4. **Monitor:** Open the Serial Monitor (115200 baud) to see real-time coordinate updates.
    
