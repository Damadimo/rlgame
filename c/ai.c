/*
inference (running the game in C using weights trained offline in python)
*/

#include "ai.h"
#include "weights.h"
#include "graphics.h"
#include "game.h"

#define INPUTS 3
#define HIDDEN 8
#define OUTPUTS 3

int ai_get_action(int fruit_x, int fruit_y, int basket_x)
{
    float input[INPUTS];    // fruit x, fruit y, basket_x
    float hidden[HIDDEN];
    float output[OUTPUTS];  // left, stay, right

    // normalize inputs for inference (cast as floats)
    input[0] = (float)fruit_x / (float)SCREEN_WIDTH;               
    input[1] = (float)fruit_y / (float)SCREEN_HEIGHT;                  
    input[2] = (float)basket_x / (float)(SCREEN_WIDTH - BASKET_WIDTH);
    
    /*
    Layer 1: hidden = ReLU(W1 * input + B1)

    for each hidden neuron (8), compute the dot product for each input (3)
    which is the core math (output = input * weights + bias)

    note: we use "row-major" math to represent a 2D matrix in C as W1 is a 1D array
          (i.e: neuron 1 at input 2 -> 1 * 3 + 2 = index 5)

    after neuron summed its weights and biases, we pass the resultant number 
    into the activation function (ReLU) for non-linearity (allows it to learn complex patterns)
    note: ReLU equivalent in C is making negativite numbers to 0
    */
    int i, j;
    for (i = 0; i < HIDDEN; i++) {
        hidden[i] = B1[i];                              // grab the bias at start          
        for (j = 0; j < INPUTS; j++) {  
            hidden[i] += W1[i * INPUTS + j] * input[j]; // dot product
        }
        if (hidden[i] < 0.0f)                           // activation function
            hidden[i] = 0.0f;          
    }

    /*
    Layer 2: output = W2 * hidden + B2
     
    connects 8 hidden neurons to 3 outupts 
    
    same logic as before, still using row-major math, 

    no softmax required during inference: converting to percentages that up to 100% was needed during training
    to have accurate probabilites, instead we choose the largest value (argmax) and order of the numbers is the same    
    */
    for (i = 0; i < OUTPUTS; i++) {
        output[i] = B2[i];                                 // grab the bias at start
        for (j = 0; j < HIDDEN; j++) {
            output[i] += W2[i * HIDDEN + j] * hidden[j];   // dot product
        }
    }

    /*
    argmax to pick action with largest value (0 left, 1 stay, 2 right)
    */
    int best = 0;

    if (output[1] > output[best]) 
        best = 1;

    if (output[2] > output[best])
        best = 2;

    return best;
}