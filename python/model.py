"""
model.py - MLP policy (exported as arrays in C)

3 inputs -> 8 hidden layers (neurons) (ReLU) -> 3 outoputs (softmax)

inputs (normalized for network): [fruit_x, fruit_y, basket_x]
outputs (probabilites summed to 1): [P(left), P(stay), P(right)]

total parameters: 59 parameters (floats)
"""

import torch
import torch.nn as nn

class Policy(nn.Module):
    def __init__(self, n_inputs=3, n_hidden=8, n_outputs=3):
        """
        nn.Module = base class for PyTorch neural nets    
        nn.Linear(in, out): creates connected layer where output = input * weights^T + bias

        fc1: 3 inputs and 8 hidden neurons
        weights: 8 rows x 3 cols = 24 floats
        biases: 8 floats
        total: 32 params

        fc2: 8 hidden neurons and 3 outputs
        weights: 8 rows x 3 cols = 24 floast
        biases: 3 floats
        total: 27 params
        """
        super().__init__()
        self.fc1 = nn.Linear(n_inputs, n_hidden)
        self.fc2 = nn.Linear(n_hidden, n_outputs)

    def forward(self, x):
        """
        layer 1: linear transformation then ReLU (keeps positive values)
            - activation function letting network learn nonlinear patterns
        
        layer 2: linear transformation then softmax (converts raw scores to probabilites that sum to 1)  
            - exploration: during training we sample probabilites called 
            - exploitation: during inference on board we pick argmax

        x is a tensor (fruit_x, fruit_y, basket_x
        """

        x = torch.relu(self.fc1(x))
        return torch.softmax(self.fc2(x), dim=-1)