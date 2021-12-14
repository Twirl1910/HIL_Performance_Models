import matplotlib.pyplot as plt
from read_simulation import results as sr
from read_measured import results as mr
import numpy as np

rows = 4
cols = 4
OFFSET = 6

fig = plt.figure(figsize=(30, 20))
gs = fig.add_gridspec(rows, cols, hspace=0.4, wspace=0.2)
axs = gs.subplots()

for i in range(OFFSET, rows + OFFSET):
    simulated = []
    measured = []
    try:
        measured = mr[f"T{i}-T{i-1}"][1:501]
    except KeyError:
        print(f"Key T{i}-T{i-1} existiert nicht in measured")
    try:
        simulated = sr[f"T{i}-T{i-1}"][1:501]
    except KeyError:
        print(f"Key T{i}-T{i-1} existiert nicht in simulated")

    axs[i-OFFSET, 0].plot(np.arange(len(measured)), measured)
    axs[i-OFFSET, 0].set_title(f"measured latency T{i-1} - T{i}")

    axs[i-OFFSET, 1].plot(np.arange(len(simulated)), simulated)
    axs[i-OFFSET, 1].set_title(f"simulated latency T{i-1} - T{i}")
    axs[i-OFFSET, 1].sharey(axs[i-OFFSET, 0])

    axs[i-OFFSET, 2].boxplot([measured, simulated])
    axs[i-OFFSET, 2].set_title(f"compare latency T{i-1} - T{i}")
    axs[i-OFFSET, 2].set_xticklabels(["measured", "simulated"])
    axs[i-OFFSET, 2].sharey(axs[i-OFFSET, 1])

    try:
        axs[i-OFFSET, 3].plot(np.arange(len(measured)), np.array(measured) - np.array(simulated))
        axs[i-OFFSET, 3].set_title("difference between simulated and measured")
    except ValueError:
        pass

plt.show()
