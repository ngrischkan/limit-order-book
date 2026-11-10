import pandas as pd
import matplotlib.pyplot as plt

trades = pd.read_csv("trades.csv")
print(trades.head())
print((trades["price"] - trades["mid"]).abs().mean())

plt.plot(trades['price'])
plt.xlabel("Trade #")
plt.ylabel("Price")
plt.plot(trades["mid"], label="true price")
plt.title("Trade prices")
plt.legend()
plt.savefig("images/price_vs_mid.png")
plt.show()
