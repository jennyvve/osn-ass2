import matplotlib.pyplot as plt
import pandas as pd

# 1. Data Setup
data = {
    "Trace File": [
        "cxx.txt.gz",
        "gcc.txt.gz",
        "ls.txt.gz",
        "make.txt.gz",
        "small-reclaim.txt.gz",
        "small.txt.gz",
    ],
    "TLB4": [91.07, 91.94, 91.63, 89.67, 92.02, 92.02],
    "TLB64": [99.87, 99.87, 99.80, 99.84, 99.80, 99.81],
}
df = pd.DataFrame(data)

# 2. Calculate Gains
df["Gain_4_to_64"] = df["TLB64"] - df["TLB4"]

# 3. Plotting
fig, ax = plt.subplots(figsize=(10, 5))

# Creating two boxplots to compare the scale of improvement
plot_data = [df["Gain_4_to_64"]]
labels = ["From TLB=4 to 64"]

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
plt.savefig("pg_4_64.png")
