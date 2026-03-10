import matplotlib.pyplot as plt

with open("output.txt", "r", encoding="utf-8") as output:
    speedup_20000 = [float(output.readline().strip()) for _ in range(8)] 
    speedup_40000 = [float(output.readline().strip()) for _ in range(8)]

threads = [1, 2, 4, 7, 8, 16, 20, 40]
ideal = threads

plt.figure(figsize=(10, 6))
plt.plot(threads, ideal, 'k--', label='Идеальное ускорение')
plt.plot(threads, list(map(lambda x: speedup_20000[0] / x, speedup_20000)), 'bo-', label='20000×20000')
plt.plot(threads, list(map(lambda x: speedup_40000[0] / x, speedup_40000)), 'rs-', label='40000×40000')

plt.xlabel('Количество потоков')
plt.ylabel('Ускорение')
plt.title('Масштабируемость программы умножения матрицы на вектор')
plt.grid(True, alpha=0.3)
plt.legend()
# plt.savefig('speedup_graph.png', dpi=300)
plt.show()