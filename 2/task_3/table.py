import matplotlib.pyplot as plt
import numpy as np

def parse_data(filename):
    threads = [1, 2, 4, 7, 8, 16, 20, 40]
    v1_times = []
    v2_times = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if "Variant 1" in line:
                v1_times.append(float(line.split(":")[1].strip().split()[0]))
            elif "Variant 2" in line:
                v2_times.append(float(line.split(":")[1].strip().split()[0]))
    
    return np.array(threads), np.array(v1_times), np.array(v2_times)

# Загрузка данных
# Убедитесь, что файл output.txt лежит в той же папке
try:
    threads, t1, t2 = parse_data('output.txt')
except FileNotFoundError:
    print("Файл output.txt не найден!")
    exit()

# Расчет метрик
s1 = t1[0] / t1  # Ускорение Variant 1
s2 = t2[0] / t2  # Ускорение Variant 2
e1 = s1 / threads # Эффективность Variant 1
e2 = s2 / threads # Эффективность Variant 2

# Построение графиков
fig, ax = plt.subplots(1, 3, figsize=(18, 5))

# 1. График времени
ax[0].plot(threads, t1, 'o-', label='Variant 1 (Multiple parallel)')
ax[0].plot(threads, t2, 's-', label='Variant 2 (Single parallel)')
ax[0].set_title('Execution Time')
ax[0].set_xlabel('Number of Threads')
ax[0].set_ylabel('Time (seconds)')
ax[0].grid(True)
ax[0].legend()

# 2. График ускорения
ax[1].plot(threads, s1, 'o-', label='Variant 1')
ax[1].plot(threads, s2, 's-', label='Variant 2')
ax[1].plot(threads, threads, '--', color='gray', label='Ideal')
ax[1].set_title('Speedup')
ax[1].set_xlabel('Number of Threads')
ax[1].set_ylabel('S(p) = T1 / Tp')
ax[1].grid(True)
ax[1].legend()

# 3. График эффективности
ax[2].plot(threads, e1, 'o-', label='Variant 1')
ax[2].plot(threads, e2, 's-', label='Variant 2')
ax[2].set_ylim(0, 1.1)
ax[2].set_title('Efficiency')
ax[2].set_xlabel('Number of Threads')
ax[2].set_ylabel('E(p) = S(p) / p')
ax[2].grid(True)
ax[2].legend()

plt.tight_layout()
plt.show()