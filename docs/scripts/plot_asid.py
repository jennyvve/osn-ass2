import matplotlib.pyplot as plt
import pandas as pd

# 1. Data Setup based on updated results
data = {
    "Configuration": ["2 Traces", "3 Traces", "All Traces"],
    "Gain_ASID": [1.00, 0.60, 0.35],  # Calculated from your percentages
}
df = pd.DataFrame(data)

# 2. Plotting
fig, ax = plt.subplots(figsize=(10, 4))

# Using your requested boxplot style
ax.boxplot(
    [df["Gain_ASID"]],
    vert=False,
    patch_artist=True,
    boxprops=dict(facecolor="lightgrey", color="black"),
    medianprops=dict(color="black", linewidth=2),
)

# 3. Styling
ax.set_yticklabels(["ASID Performance Gain"])
ax.set_xlabel("Hit rate increase [%]")
ax.set_title("Distribution of Performance Gains via ASID")
ax.grid(axis="x", linestyle="--", alpha=0.6)

# Set limits to see the 0.35% to 1.00% spread clearly
ax.set_xlim(0, 1.2)

plt.tight_layout()
plt.savefig("pg_asid.png")
