import matplotlib.pyplot as plt

# Read data from output files
single_threaded_data = []
multithreaded_data = []

with open("single_threaded_output.txt", "r") as f:
    for line in f:
        num_threads, time = line.split()
        single_threaded_data.append((int(num_threads), float(time)))

with open("multithreaded_output.txt", "r") as f:
    for line in f:
        num_threads, time = line.split()
        multithreaded_data.append((int(num_threads), float(time)))

# Sort data by number of threads
single_threaded_data.sort()
multithreaded_data.sort()

# Extract data for plotting
threads = [x[0] for x in single_threaded_data]
single_threaded_times = [x[1] for x in single_threaded_data]
multithreaded_times = [x[1] for x in multithreaded_data]

# Plot the graph
plt.figure(figsize=(10, 6))
plt.plot(threads, single_threaded_times, label="Single-threaded", marker='o')
plt.plot(threads, multithreaded_times, label="Multithreaded", marker='o')
plt.xlabel("Number of Threads")
plt.ylabel("Execution Time (seconds)")
plt.title("Execution Time vs. Number of Threads")
plt.legend()
plt.grid(True)
plt.savefig("execution_time_vs_threads.png")
plt.show()