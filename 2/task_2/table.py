import matplotlib.pyplot as plt


with open("output.txt", "r", encoding="utf-8") as output:
    times = [float(output.readline().strip()) for _ in range(8)]

threads = [1, 2, 4, 7, 8, 16, 20, 40]

speedup = [times[0] / t for t in times]

ideal = threads

plt.figure(figsize=(10, 6))

plt.plot(threads, ideal, 'k--', label='Идеальное ускорение')
plt.plot(threads, speedup, 'bo-', label='Реальное ускорение')

plt.xlabel('Количество потоков')
plt.ylabel('Ускорение')
plt.title('Масштабируемость параллельного вычисления интеграла')
plt.grid(True, alpha=0.3)
plt.legend()

plt.savefig('speedup_graph.png', dpi=300)
plt.show()