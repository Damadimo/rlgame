import numpy as np

# same global definitons from C verison of catch game
SCREEN_WIDTH   = 320      
SCREEN_HEIGHT  = 240      
BASKET_WIDTH   = 36       
BASKET_HEIGHT  = 10       
BASKET_SPEED   = 3              
MAX_FRUITS     = 10       
SPAWN_INTERVAL = 25       
INITIAL_LIVES  = 5

class Fruit:
    def __init__(self):
        self.x = 0       
        self.y = 0        
        self.r = 0        
        self.dy = 0        # fall speed
        self.active = False 

class SimulateGame: 
    def __init__(self):
        self.basket_x = 0
        self.basket_y = 0 
        self.fruits = [Fruit() for _ in range(MAX_FRUITS)]   # equivalent to Fruit fruits[MAX_FRUITS]
        self.score = 0
        self.lives = 0
        self.frame_counter = 0
        self.running = False
        self.reset()

    def reset(self):
        # reset the game back to initalize state (identical to C version of init_game)
        self.basket_x = (SCREEN_WIDTH - BASKET_WIDTH) // 2
        self.basket_y = SCREEN_HEIGHT - 18

        for fruit in self.fruits:
            fruit.active = False
        
        self.score = 0
        self.lives = INITIAL_LIVES
        self.frame_counter = 0
        self.running = True

        return self.get_state()     # for the AI to get inital state
        
    def get_state(self):
        # for the AI model to read game state variables
        # highest y is closer to the basket
        closest = None
        for fruit in self.fruits:
            if fruit.active:
                # larger y coordinate of fruit means closer to ground than current closest
                if closest is None or fruit.y > closest.y: 
                    closest = fruit

        # found a closest fruit
        if closest is not None:
            # normalize for the reinforcement learning model from [0, 1]
            fruit_x = closest.x / SCREEN_WIDTH
            fruit_y = closest.y / SCREEN_HEIGHT
        else: 
            # default values for neutral state
            fruit_x = 0.5
            fruit_y = 0.0 

        # noramlize [0,1] for basket
        basket_x = self.basket_x / (SCREEN_WIDTH - BASKET_WIDTH)

        # cast signed 32 bit floating point number (default 64 takes up too much memory)
        return np.array([fruit_x, fruit_y, basket_x], dtype=np.float32)

    # updates game by receiving and applying the AI's decision by a frame
    def step(self, action):
        """
        0 left
        1 stay
        2 right
        """        
        self.frame_counter += 1

        if action == 0:
            self.basket_x -= BASKET_SPEED
        elif action == 2:
            self.basket_x += BASKET_SPEED
    
        # clamp basket to ensure it remains within screen bounds
        if self.basket_x < 0:
            self.basket_x = 0
        if self.basket_x > SCREEN_WIDTH - BASKET_WIDTH:
            self.basket_x = SCREEN_WIDTH - BASKET_WIDTH

        # checks if 25 frames have pissed to spawn fruit
        if self.frame_counter % 25 == 0:
            self.spawn_fruit()
        

        # updates fruits at this frame 
        reward = 0.0
        for fruit in self.fruits:
            if not fruit.active:
                continue

            fruit.y += fruit.dy

            if self.fruit_hits_basket(fruit):
                fruit.active = False
                self.score += 1
                reward += 1.0       # for reinforcement learning, float

            # hits the floor (missed the basket)
            elif fruit.y - fruit.r > SCREEN_HEIGHT:
                fruit.active = False
                self.lives -= 1
                reward -= 2.0 

        # reward shaping: small bonus every frame for being close to the closest fruit
        # without this, the model only gets feedback at catch/miss moments (+1/-1)
        # and has no signal during the hundreds of frames in between
        closest = None
        for fruit in self.fruits:
            if fruit.active:
                if closest is None or fruit.y > closest.y:
                    closest = fruit
 
        if closest is not None:
            basket_center = self.basket_x + BASKET_WIDTH / 2
            distance = abs(closest.x - basket_center) / SCREEN_WIDTH
            # 0.05 (0.01 is tiny compared to catch/miss rewards) when perfectly aligned, 0.0 when far away
            reward += 0.05 * (1.0 - distance)
        
        # game over flag
        done = False
        if self.lives <= 0:
            self.running = False
            done = True

        # RL needs next state, feedback to learn, whether game is finished after each frame
        return self.get_state(), reward, done

    def spawn_fruit(self):
        for fruit in self.fruits:
            if not fruit.active:
                fruit.active = True
                fruit.r = 4 + np.random.randint(3)
                fruit.x = 10 + np.random.randint(SCREEN_WIDTH - 20)
                fruit.y = -5
                fruit.dy = 1 + np.random.randint(3)
                return

    # collision handling
    def fruit_hits_basket(self, fruit):
        fruit_left = fruit.x - fruit.r              
        fruit_right = fruit.x + fruit.r                  
        fruit_top  = fruit.y - fruit.r                  
        fruit_bottom = fruit.y + fruit.r              

        basket_left = self.basket_x              
        basket_right = self.basket_x + BASKET_WIDTH   
        basket_top = self.basket_y              
        basket_bottom = self.basket_y + BASKET_HEIGHT  

        # fruit is left of the basket
        if fruit_right < basket_left:
            return False                    
        
        # fruit is right of basket
        if fruit_left > basket_right:
            return False                  
        
        # fruit is above basket (smaller y coordinate means higher up)
        if fruit_bottom < basket_top:
            return False           
                 
        # fruit is below the basket
        if fruit_top > basket_bottom: 
            return False                   

        # otherwise boxes overlap (collision)
        return True 
    

if __name__ == "__main__":
    env = SimulateGame()
    state = env.reset()

    total_reward = 0
    steps = 0

    while True:
        # simple heuristic: move toward the closest fruit
        # this just tests that the sim works — not the trained AI
        fruit_x_norm   = state[0]   # normalized ball x
        basket_x_norm = state[2]   # normalized basket x

        if fruit_x_norm < basket_x_norm - 0.05:
            action = 0    # move left
        elif fruit_x_norm > basket_x_norm + 0.05:
            action = 2    # move right
        else:
            action = 1    # stay

        state, reward, done = env.step(action)
        total_reward += reward
        steps += 1

        if done:
            break

    print(f"Game over after {steps} frames")
    print(f"Score: {env.score}  |  Lives left: {env.lives}  |  Total reward: {total_reward}")