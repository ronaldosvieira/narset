#include <math.h>
#include "network.h"
#include "weights.h"

int predict(const double* input) {
    double X[169], Y[3];

    // forward pass through the hidden layer
    for (int i = 0; i < 169; i++) {
        X[i] = hidden_layer_biases[i];

        for (int j = 0; j < 48; j++)
            X[i] += input[j] * hidden_layer_weights[j][i];

        X[i] = X[i] >= 0? X[i] : exp(X[i]) - 1;
    }

    // calculate the policy
    for (int i = 0; i < 3; i++) {
        Y[i] = policy_biases[i];

        for (int j = 0; j < 169; j++)
            Y[i] += X[j] * policy_weights[j][i];
    }

    // extract an action
    int action = 0;
    double best_value = Y[0];

    if (Y[1] > best_value) {
        action = 1; best_value = Y[1];
    }

    if (Y[2] > best_value) action = 2;

    return action;
}