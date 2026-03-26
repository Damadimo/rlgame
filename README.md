training a RL model to play a simple game of catch deployed on a risc-32 fpga

trained offline in python, inference ran on C

file structure:
    python (offline training):
        sim.py         - game logic cloned from C
        model.py       - defines MLP
        train.py       - rl training loop
        export.py      - converts policy.pt -> weights.h

    C (inference on board):
        main.c         - game loop (AI drives basket when AI_MODE=1)
        game.c/h       - game logic
        graphics.c/h   - VGA double buffered rendering
        input.c/h      - read pushbutton
        ai.c/h         - forward pass (matrix multiplication)
        weights.h      - 59 trained floats

currently:
    - model learned from 3 inputs (fruit_x, fruit_x, basket_x) 
    - 8 hidden neurons layer for computation
    - 3 outputs (0, 1, 2) representing left, stay, right respectively
    - trained over 20000 epsiodes
total parameters: 59 floats

next steps:
    - feed locations (x,y) of all 10 fruits rather than just the closest one (for complex thinking)
    - add PS/2 keyboard rather than using key pushbuttons on DE1-SoC
    - split VGA screen in half: allowing both human and player input

each fruit needs can have at most 5 values (active,x,y,speed,radius) -> 10 fruits = 51 inputs (+1 basket)
with 51 inputs and 64 neurons in hidden layer then:
                (51 * 64 + 64) + (64 * 64 + 64) + (64 * 3 + 3) = 7683 parameters 

how to run:
    python part:
        pip install -r requirements.txt
        python sim.py
        python train.py
        python export.py
    
    C part (inference on board):
        compile on hardware 