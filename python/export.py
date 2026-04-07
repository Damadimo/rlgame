""" 
exporting 59 parameters to weights.h (offline training, online inference)

"""


import torch
from pathlib import Path

from model import Policy

_SCRIPT_DIR = Path(__file__).resolve().parent
_REPO_ROOT = _SCRIPT_DIR.parent


def export_weights(model_path=None, output_path=None):
    if model_path is None:
        model_path = _SCRIPT_DIR / "policy.pt"
    else:
        model_path = Path(model_path)
    if output_path is None:
        output_path = _REPO_ROOT / "c" / "weights.h"
    else:
        output_path = Path(output_path)
    
    # load trained model
    policy = Policy()
    policy.load_state_dict(torch.load(str(model_path), weights_only=True))
    policy.eval()

    """
    fc1: fully connected layer (connects 3 inputs to 8 hidden neurons)
    fc2: fully connected layer (8 hidden neurons to 3 outputs)

    .detach().tolist(): detachs from the computational graph to notify PyTorch
                       that we're done training and converts tensor to array
    """
    w1 = policy.fc1.weight.detach().numpy()
    b1 = policy.fc1.bias.detach().numpy()
    w2 = policy.fc2.weight.detach().numpy()
    b2 = policy.fc2.bias.detach().numpy()

    # creating weights.h file
    with open(output_path, "w", encoding="utf-8") as f:
        f.write("#ifndef WEIGHTS_H\n")
        f.write("#define WEIGHTS_H\n\n")

        # loops through the numbers, formats into C synthax, converst matrix to 1D array
        write_array(f, "W1", w1.flatten())
        write_array(f, "B1", b1.flatten())
        write_array(f, "W2", w2.flatten())
        write_array(f, "B2", b2.flatten())

        f.write("#endif\n")

    print(f"Exported {w1.size + b1.size + w2.size + b2.size} parameters to {output_path}")

def write_array(f, name, arr):
    """Writes one float array as a C const declaration."""

    f.write(f"const float {name}[{len(arr)}] = {{\n    ")
    for i, val in enumerate(arr):
        f.write(f"{val:.6f}f")
        if i < len(arr) - 1:
            f.write(", ")
        # line break every 6 values for readability
        if (i + 1) % 6 == 0 and i < len(arr) - 1:
            f.write("\n    ")
    f.write("\n};\n\n")

if __name__ == "__main__":
    export_weights()