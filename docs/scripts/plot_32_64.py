import matplotlib.pyplot as plt
import pandas as pd

# 1. Data Setup
data = {
    "Trace": ["cxx", "gcc", "ls", "make", "small-rec", "small"],
    "TLB32": [99.66, 99.62, 99.17, 99.06, 99.51, 99.52],
    "TLB64": [99.87, 99.87, 99.80, 99.84, 99.80, 99.81],
}
df = pd.DataFrame(data)

# 2. Calculate Gains
df["Gain_32_to_64"] = df["TLB64"] - df["TLB32"]

# 3. Plotting
fig, ax = plt.subplots(figsize=(10, 5))

# Creating two boxplots to compare the scale of improvement
plot_data = [df["Gain_32_to_64"]]
labels = ["From TLB=32 to 64"]

ax.boxplot(
    plot_data,
    vert=False,
    patch_artist=True,
    boxprops=dict(facecolor="lightgrey", color="black"),
    medianprops=dict(color="black", linewidth=2),
)

# 4. Styling
ax.set_yticklabels(labels)
ax.set_xlabel("Hit rate increase [%]")
ax.set_title("")
ax.grid(axis="x", linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("pg_32_64.png")
