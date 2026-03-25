"""
train.py - trains the polcity with reinforcement learning 
           via policy gradient (tweaks the network via positive / negative
           rewards to increase / decrease probability of the specific moves it chooses)

1) model plays full game
2) at each frame, get_state() and picks an action
3) after the game ends we look at actions led to good outcomes
4) reinforce or supress those actions
5) repeat
"""

import torch
import torch.optim as optim
import numpy as np
from sim import SimulateGame 
from model import Policy

def train(num_episodes=20000, lr=1e-3, gamma=0.99, print_every=500):
    """
    arguments:
        1) num_episodes: how many full games to simulate
        2) lr: learning rate (how big each weight update step is)
        3) gamma: discount factor (multiplies steps backwards leading up to the reward)
                  which shows the impact of the action it did many frames ago
        4) print_every: prints an update message every 500 epsiodes
    """

    policy = Policy()   # creates MLP
    optimizer = optim.Adam(policy.parameters(), lr=lr)  # Adam optimizer handles learning rate
    
    env = SimulateGame()
    reward_history = []

    for episode in range(num_episodes):
        
        state = env.reset()
        log_probs = []
        rewards = []

        while True:
            # convert np array to tensor
            state_tensor = torch.FloatTensor(state)

            # forward pass
            probs = policy(state_tensor)

            # probability distribution of likely hood of actions (left, stay, right)
            dist = torch.distributions.Categorical(probs)

            # returns 0, 1, 2 
            # if model keeps choosing argmax then it will never try alternative routes (can get stuck in bad pattern)
            action = dist.sample() 

            # used to compute the gradient
            log_probs.append(dist.log_prob(action))

            # takes action in the game (converts to tensor)
            state, reward, done = env.step(action.item())
            rewards.append(reward)

            if done:
                break

            
        returns = []
        G = 0.0
        for r in reversed(rewards):      # walk backwards: last reward first
            G = r + gamma * G            # accumulate discounted return
            returns.insert(0, G)         # prepend so index 0 = first frame

        returns = torch.FloatTensor(returns)

        if len(returns) > 1:
            returns = (returns - returns.mean()) / (returns.std() + 1e-8)

        loss = torch.zeros(1)
        for lp, G_val in zip(log_probs, returns):
            loss -= lp * G_val

        optimizer.zero_grad()   # clear gradients from previous episode
        loss.backward()         # compute gradients (backpropagation)
        optimizer.step()        # update all 59 weights using the gradients

        # ── logging ──
        episode_reward = sum(rewards)
        reward_history.append(episode_reward)

        if (episode + 1) % print_every == 0:
            avg = np.mean(reward_history[-print_every:])
            print(f"Episode {episode+1:>6d}  |  avg reward: {avg:>7.2f}  |  "f"last score: {env.score}  |  last lives left: {env.lives}")


    torch.save(policy.state_dict(), "policy.pt")
    print(f"\nTraining complete. Model saved to policy.pt")
    print(f"Final avg reward (last 500): {np.mean(reward_history[-500:]):.2f}")

    return policy

if __name__ == "__main__":
    trained_policy = train()